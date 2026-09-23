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

function see64you_type(WP_REST_Request $request): string
{
    $type = sanitize_key((string)$request->get_param('type'));
    return $type === 'page' ? 'page' : 'post';
}

function see64you_can_list_documents(WP_REST_Request $request): bool
{
    return see64you_type($request) === 'page'
        ? current_user_can('edit_pages')
        : current_user_can('edit_posts');
}

function see64you_can_create_document(WP_REST_Request $request): bool
{
    $type = see64you_type($request);
    $status = sanitize_key((string)$request->get_param('status'));

    if ($type === 'page') {
        return current_user_can('edit_pages')
            && ($status !== 'publish' || current_user_can('publish_pages'));
    }

    return current_user_can('edit_posts')
        && ($status !== 'publish' || current_user_can('publish_posts'));
}

function see64you_can_edit_document(WP_REST_Request $request): bool
{
    $id = (int)$request['id'];
    if ($id <= 0 || !current_user_can('edit_post', $id)) {
        return false;
    }

    $status = sanitize_key((string)$request->get_param('status'));
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
        'type' => $post->post_type === 'page' ? 'page' : 'post',
        'slug' => $post->post_name,
        'title' => get_the_title($post),
        'status' => $post->post_status,
        'date' => get_post_time(DATE_ATOM, false, $post),
        'order' => (int)$post->menu_order,
        'body' => $source,
        'managed' => $managed,
    ];
}

function see64you_list_documents(WP_REST_Request $request): WP_REST_Response
{
    $type = see64you_type($request);
    $posts = get_posts([
        'post_type' => $type,
        'post_status' => ['publish', 'draft', 'pending', 'future', 'private'],
        'posts_per_page' => 100,
        'orderby' => $type === 'page' ? ['menu_order' => 'ASC', 'title' => 'ASC'] : ['date' => 'DESC'],
    ]);

    $documents = array_map(static function (WP_Post $post): array {
        $item = see64you_document_response($post);
        unset($item['body']);
        return $item;
    }, $posts);

    return new WP_REST_Response(['ok' => true, 'documents' => $documents]);
}

function see64you_get_document(WP_REST_Request $request): WP_REST_Response|WP_Error
{
    $post = get_post((int)$request['id']);
    if (!$post || !in_array($post->post_type, ['post', 'page'], true)) {
        return new WP_Error('see64you_not_found', 'Document not found.', ['status' => 404]);
    }

    return new WP_REST_Response(['ok' => true, 'document' => see64you_document_response($post)]);
}

function see64you_save_document(WP_REST_Request $request): WP_REST_Response|WP_Error
{
    $id = isset($request['id']) ? (int)$request['id'] : 0;
    $existing = $id > 0 ? get_post($id) : null;
    $type = $existing instanceof WP_Post ? $existing->post_type : see64you_type($request);

    if (!in_array($type, ['post', 'page'], true)) {
        return new WP_Error('see64you_invalid_type', 'Invalid document type.', ['status' => 400]);
    }

    $title = sanitize_text_field((string)$request->get_param('title'));
    $source = str_replace(["\r\n", "\r"], "\n", (string)$request->get_param('body'));
    $status = sanitize_key((string)$request->get_param('status'));
    $slug = sanitize_title((string)$request->get_param('slug'));

    if ($title === '') {
        return new WP_Error('see64you_invalid_title', 'Title is required.', ['status' => 400]);
    }
    if (!in_array($status, ['draft', 'publish', 'pending', 'private', 'future'], true)) {
        $status = 'draft';
    }

    $postData = [
        'ID' => $id,
        'post_type' => $type,
        'post_title' => $title,
        'post_content' => see64you_render_markup($source),
        'post_status' => $status,
        'menu_order' => (int)$request->get_param('order'),
    ];

    if ($slug !== '') {
        $postData['post_name'] = $slug;
    }

    $date = trim((string)$request->get_param('date'));
    if ($date !== '') {
        $timestamp = strtotime($date);
        if ($timestamp !== false) {
            $postData['post_date'] = wp_date('Y-m-d H:i:s', $timestamp);
        }
    }

    $savedId = wp_insert_post(wp_slash($postData), true);
    if (is_wp_error($savedId)) {
        return $savedId;
    }

    update_post_meta($savedId, SEE64YOU_META_SOURCE, $source);
    update_post_meta($savedId, SEE64YOU_META_MANAGED, true);

    $post = get_post($savedId);
    return new WP_REST_Response(['ok' => true, 'document' => see64you_document_response($post)], $id > 0 ? 200 : 201);
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

    return new WP_REST_Response(['ok' => true, 'deleted' => true, 'id' => $id]);
}
