<?php
/**
 * Plugin Name: See64You
 * Description: Small REST publishing adapter for the See64You C64U editor.
 * Version: 0.1.0
 * Requires at least: 6.0
 * Requires PHP: 8.0
 * Author: See64You
 */

if (!defined('ABSPATH')) {
    exit;
}

require_once __DIR__ . '/lib/protocol.php';

const SEE64YOU_META_SOURCE = '_see64you_source';
const SEE64YOU_META_MANAGED = '_see64you_managed';

add_action('init', static function (): void {
    register_post_meta('', SEE64YOU_META_SOURCE, [
        'type' => 'string',
        'single' => true,
        'show_in_rest' => false,
        'sanitize_callback' => static fn($value): string => (string)$value,
        'auth_callback' => static function (): bool {
            return current_user_can('edit_posts') || current_user_can('edit_pages');
        },
    ]);

    register_post_meta('', SEE64YOU_META_MANAGED, [
        'type' => 'boolean',
        'single' => true,
        'show_in_rest' => false,
        'auth_callback' => static function (): bool {
            return current_user_can('edit_posts') || current_user_can('edit_pages');
        },
    ]);
});

add_action('rest_api_init', static function (): void {
    register_rest_route('see64you/v1', '/status', [
        'methods' => WP_REST_Server::READABLE,
        'callback' => static fn(): WP_REST_Response => new WP_REST_Response([
            'ok' => true,
            'protocol' => 'see64you/1',
            'backend' => 'wordpress',
        ]),
        'permission_callback' => '__return_true',
    ]);

    register_rest_route('see64you/v1', '/connection', [
        'methods' => WP_REST_Server::READABLE,
        'callback' => static fn(): WP_REST_Response => new WP_REST_Response([
            'ok' => true,
            'protocol' => 'see64you/1',
            'backend' => 'wordpress',
            'can_edit_posts' => current_user_can('edit_posts'),
            'can_edit_pages' => current_user_can('edit_pages'),
        ]),
        'permission_callback' => static fn(): bool =>
            current_user_can('edit_posts') || current_user_can('edit_pages'),
    ]);

    register_rest_route('see64you/v1', '/documents', [
        [
            'methods' => WP_REST_Server::READABLE,
            'callback' => 'see64you_list_documents',
            'permission_callback' => 'see64you_can_list_documents',
        ],
        [
            'methods' => WP_REST_Server::CREATABLE,
            'callback' => 'see64you_save_document',
            'permission_callback' => 'see64you_can_create_document',
        ],
    ]);

    register_rest_route('see64you/v1', '/documents/(?P<id>\d+)', [
        [
            'methods' => WP_REST_Server::READABLE,
            'callback' => 'see64you_get_document',
            'permission_callback' => 'see64you_can_edit_document',
        ],
        [
            'methods' => WP_REST_Server::EDITABLE,
            'callback' => 'see64you_save_document',
            'permission_callback' => 'see64you_can_edit_document',
        ],
        [
            'methods' => WP_REST_Server::DELETABLE,
            'callback' => 'see64you_delete_document',
            'permission_callback' => 'see64you_can_delete_document',
        ],
    ]);
});

function see64you_type(WP_REST_Request $request): ?string
{
    $type = sanitize_key((string)$request->get_param('type'));
    return in_array($type, ['page', 'post'], true) ? $type : null;
}

function see64you_request_input(WP_REST_Request $request): array|WP_Error
{
    $contentType = strtolower((string)$request->get_header('content-type'));

    if (str_contains($contentType, SEE64YOU_WIRE_MIME)) {
        try {
            return see64you_wire_parse($request->get_body());
        } catch (InvalidArgumentException) {
            return new WP_Error(
                'see64you_invalid_wire',
                'Invalid See64You wire document.',
                ['status' => 400]
            );
        }
    }

    return [
        'type' => (string)$request->get_param('type'),
        'slug' => (string)$request->get_param('slug'),
        'title' => (string)$request->get_param('title'),
        'status' => (string)$request->get_param('status'),
        'date' => (string)$request->get_param('date'),
        'order' => (int)$request->get_param('order'),
        'body' => (string)$request->get_param('body'),
    ];
}

function see64you_input_type(array $input): ?string
{
    $type = sanitize_key((string)($input['type'] ?? ''));
    return in_array($type, ['page', 'post'], true) ? $type : null;
}

function see64you_c64_metadata_compatible(array $document): bool
{
    return strlen((string)($document['slug'] ?? '')) <= 64
        && preg_match('/^[a-z0-9]+(?:-[a-z0-9]+)*$/', (string)($document['slug'] ?? '')) === 1
        && strlen((string)($document['title'] ?? '')) <= 80
        && strlen((string)($document['date'] ?? '')) <= 40
        && strlen((string)($document['body'] ?? '')) <= 8192;
}

function see64you_can_list_documents(WP_REST_Request $request): bool|WP_Error
{
    $type = see64you_type($request);
    if ($type === null) {
        return new WP_Error('see64you_invalid_type', 'Invalid document type.', ['status' => 400]);
    }

    return $type === 'page'
        ? current_user_can('edit_pages')
        : current_user_can('edit_posts');
}

function see64you_can_create_document(WP_REST_Request $request): bool|WP_Error
{
    $input = see64you_request_input($request);
    if (is_wp_error($input)) {
        return $input;
    }

    $type = see64you_input_type($input);
    $status = see64you_status_to_wordpress(sanitize_key((string)($input['status'] ?? '')));
    if ($type === null) {
        return new WP_Error('see64you_invalid_type', 'Invalid document type.', ['status' => 400]);
    }
    if ($status === null) {
        return new WP_Error('see64you_invalid_status', 'Invalid document status.', ['status' => 400]);
    }

    if ($type === 'page') {
        return current_user_can('edit_pages')
            && ($status !== 'publish' || current_user_can('publish_pages'));
    }

    return current_user_can('edit_posts')
        && ($status !== 'publish' || current_user_can('publish_posts'));
}

function see64you_can_edit_document(WP_REST_Request $request): bool|WP_Error
{
    $id = (int)$request['id'];
    if ($id <= 0 || !current_user_can('edit_post', $id)) {
        return false;
    }

    if ($request->get_method() === 'GET') {
        return true;
    }

    $input = see64you_request_input($request);
    if (is_wp_error($input)) {
        return $input;
    }

    $status = see64you_status_to_wordpress(sanitize_key((string)($input['status'] ?? '')));
    if ($status === null) {
        return new WP_Error('see64you_invalid_status', 'Invalid document status.', ['status' => 400]);
    }
    if ($status !== 'publish') {
        return true;
    }

    $post = get_post($id);
    if (!$post instanceof WP_Post) {
        return false;
    }

    return $post->post_type === 'page'
        ? current_user_can('publish_pages')
        : current_user_can('publish_posts');
}

function see64you_can_delete_document(WP_REST_Request $request): bool
{
    $id = (int)$request['id'];
    return $id > 0 && current_user_can('delete_post', $id);
}

function see64you_inline_markup(string $text): string
{
    $safe = esc_html($text);
    $safe = preg_replace('/\*\*([^*]+)\*\*/', '<strong>$1</strong>', $safe) ?? $safe;
    $safe = preg_replace('/\*([^*]+)\*/', '<em>$1</em>', $safe) ?? $safe;

    return preg_replace_callback(
        '~\[([^\]]+)\]\((https?://[^)\s]+)\)~i',
        static function (array $matches): string {
            $url = esc_url($matches[2], ['http', 'https']);
            return $url === ''
                ? $matches[0]
                : '<a href="' . esc_attr($url) . '">' . $matches[1] . '</a>';
        },
        $safe
    ) ?? $safe;
}

function see64you_render_markup(string $source): string
{
    $lines = preg_split('/\R/', $source) ?: [];
    $html = [];
    $listOpen = false;

    foreach ($lines as $line) {
        if (str_starts_with($line, '- ')) {
            if (!$listOpen) {
                $html[] = '<ul>';
                $listOpen = true;
            }
            $html[] = '<li>' . see64you_inline_markup(substr($line, 2)) . '</li>';
            continue;
        }

        if ($listOpen) {
            $html[] = '</ul>';
            $listOpen = false;
        }

        if (str_starts_with($line, '## ')) {
            $html[] = '<h2>' . see64you_inline_markup(substr($line, 3)) . '</h2>';
        } elseif (str_starts_with($line, '# ')) {
            $html[] = '<h1>' . see64you_inline_markup(substr($line, 2)) . '</h1>';
        } elseif (trim($line) !== '') {
            $html[] = '<p>' . see64you_inline_markup($line) . '</p>';
        }
    }

    if ($listOpen) {
        $html[] = '</ul>';
    }

    return implode("\n", $html);
}

function see64you_document_response(WP_Post $post): array
{
    $source = (string)get_post_meta($post->ID, SEE64YOU_META_SOURCE, true);
    $managed = (bool)get_post_meta($post->ID, SEE64YOU_META_MANAGED, true);

    if (!$managed) {
        $source = html_entity_decode(
            wp_strip_all_tags($post->post_content, true),
            ENT_QUOTES | ENT_HTML5,
            get_bloginfo('charset') ?: 'UTF-8'
        );
    }

    return [
        'id' => $post->ID,
        'id_text' => (string)$post->ID,
        'type' => $post->post_type === 'page' ? 'page' : 'post',
        'slug' => $post->post_name,
        'title' => get_the_title($post),
        'status' => see64you_status_from_wordpress($post->post_status),
        'wordpress_status' => $post->post_status,
        'date' => get_post_time(DATE_ATOM, false, $post),
        'order' => (int)$post->menu_order,
        'body' => $source,
        'managed' => $managed,
    ];
}

function see64you_list_documents(WP_REST_Request $request): WP_REST_Response|WP_Error
{
    $type = see64you_type($request);
    if ($type === null) {
        return new WP_Error('see64you_invalid_type', 'Invalid document type.', ['status' => 400]);
    }

    $args = [
        'post_type' => $type,
        'post_status' => ['publish', 'draft', 'pending', 'future', 'private'],
        'posts_per_page' => 100,
        'orderby' => $type === 'page' ? ['menu_order' => 'ASC', 'title' => 'ASC'] : ['date' => 'DESC'],
    ];

    if ((string)$request->get_param('managed') === '1') {
        $args['meta_query'] = [[
            'key' => SEE64YOU_META_MANAGED,
            'value' => '1',
        ]];
    }

    $posts = get_posts($args);
    $documents = array_map(static function (WP_Post $post): array {
        $item = see64you_document_response($post);
        unset($item['body']);
        return $item;
    }, $posts);

    return new WP_REST_Response([
        'ok' => true,
        'count' => count($documents),
        'documents' => $documents,
    ]);
}

function see64you_get_document(WP_REST_Request $request): WP_REST_Response|WP_Error
{
    $post = get_post((int)$request['id']);
    if (!$post || !in_array($post->post_type, ['post', 'page'], true)) {
        return new WP_Error('see64you_not_found', 'Document not found.', ['status' => 404]);
    }

    $document = see64you_document_response($post);
    if ((string)$request->get_param('chunks') === '1') {
        if (!see64you_c64_metadata_compatible($document)) {
            return new WP_Error(
                'see64you_incompatible_document',
                'Document exceeds the current C64 client limits.',
                ['status' => 422]
            );
        }

        $source = (string)$document['body'];
        $chunks = see64you_source_chunks($source);
        unset($document['body']);
        $document['source_encoding'] = 'base64-chunks';
        $document['source_length'] = strlen($source);
        $document['source_count'] = count($chunks);
        $document['source_chunks'] = $chunks;
    }

    return new WP_REST_Response(['ok' => true, 'document' => $document]);
}

function see64you_save_document(WP_REST_Request $request): WP_REST_Response|WP_Error
{
    $id = isset($request['id']) ? (int)$request['id'] : 0;
    $existing = $id > 0 ? get_post($id) : null;
    $input = see64you_request_input($request);
    if (is_wp_error($input)) {
        return $input;
    }

    $requestedType = see64you_input_type($input);
    $type = $existing instanceof WP_Post ? $existing->post_type : $requestedType;
    if (!in_array($type, ['post', 'page'], true)
        || ($existing instanceof WP_Post && $requestedType !== null && $requestedType !== $type)) {
        return new WP_Error('see64you_invalid_type', 'Invalid document type.', ['status' => 400]);
    }

    $title = sanitize_text_field((string)($input['title'] ?? ''));
    $source = str_replace(["\r\n", "\r"], "\n", (string)($input['body'] ?? ''));
    $status = see64you_status_to_wordpress(sanitize_key((string)($input['status'] ?? '')));
    $slug = trim((string)($input['slug'] ?? ''));
    $date = trim((string)($input['date'] ?? ''));
    $order = (int)($input['order'] ?? 0);

    if ($title === '' || strlen($title) > 80) {
        return new WP_Error('see64you_invalid_title', 'Title is invalid.', ['status' => 400]);
    }
    if (strlen($source) > 8192) {
        return new WP_Error('see64you_document_too_large', 'Document is too large.', ['status' => 413]);
    }
    if ($status === null) {
        return new WP_Error('see64you_invalid_status', 'Invalid document status.', ['status' => 400]);
    }
    if (strlen($slug) > 64
        || preg_match('/^[a-z0-9]+(?:-[a-z0-9]+)*$/', $slug) !== 1) {
        return new WP_Error('see64you_invalid_slug', 'Slug is invalid.', ['status' => 400]);
    }
    if (strlen($date) > 40) {
        return new WP_Error('see64you_invalid_date', 'Date is invalid.', ['status' => 400]);
    }

    $postData = [
        'ID' => $id,
        'post_type' => $type,
        'post_title' => $title,
        'post_content' => see64you_render_markup($source),
        'post_status' => $status,
        'post_name' => $slug,
        'menu_order' => $order,
    ];

    if ($date !== '') {
        $timestamp = strtotime($date);
        if ($timestamp === false) {
            return new WP_Error('see64you_invalid_date', 'Date is invalid.', ['status' => 400]);
        }
        $postData['post_date'] = wp_date('Y-m-d H:i:s', $timestamp);
    }

    $savedId = wp_insert_post(wp_slash($postData), true);
    if (is_wp_error($savedId)) {
        return $savedId;
    }

    update_post_meta($savedId, SEE64YOU_META_SOURCE, wp_slash($source));
    update_post_meta($savedId, SEE64YOU_META_MANAGED, true);

    $post = get_post($savedId);
    if (!$post instanceof WP_Post) {
        return new WP_Error('see64you_save_failed', 'Saved document could not be loaded.', ['status' => 500]);
    }

    $document = see64you_document_response($post);
    if ((string)$request->get_param('ack') === '1') {
        return new WP_REST_Response([
            'ok' => true,
            'id' => $savedId,
            'id_text' => (string)$savedId,
            'slug' => $document['slug'],
            'status' => $document['status'],
        ], $id > 0 ? 200 : 201);
    }

    return new WP_REST_Response(
        ['ok' => true, 'document' => $document],
        $id > 0 ? 200 : 201
    );
}

function see64you_delete_document(WP_REST_Request $request): WP_REST_Response|WP_Error
{
    $id = (int)$request['id'];
    $post = get_post($id);
    if (!$post || !in_array($post->post_type, ['post', 'page'], true)) {
        return new WP_Error('see64you_not_found', 'Document not found.', ['status' => 404]);
    }

    $deleted = wp_trash_post($id);
    if (!$deleted) {
        return new WP_Error('see64you_delete_failed', 'Could not move document to Trash.', ['status' => 500]);
    }

    return new WP_REST_Response([
        'ok' => true,
        'deleted' => true,
        'deleted_int' => 1,
        'id' => $id,
        'id_text' => (string)$id,
    ]);
}
