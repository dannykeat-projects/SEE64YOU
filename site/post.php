<?php
declare(strict_types=1);

require __DIR__ . '/lib/bootstrap.php';

$slug = (string)($_GET['slug'] ?? '');
$previewToken = (string)($_GET['preview'] ?? '');
try {
    $post = $documents->get('post', $slug);
} catch (InvalidArgumentException) {
    $post = null;
}

$pages = $documents->list('page', true);
$isPreview = $post !== null
    && $post['status'] === 'draft'
    && $auth->validPreviewToken($previewToken, 'post', $slug);

if ($post === null || ($post['status'] !== 'published' && !$isPreview)) {
    http_response_code(404);
    s64_render_layout($config, $pages, 'Not found', '<h2>Not found</h2><p>The requested post was not found.</p>');
    exit;
}

if ($isPreview) {
    s64_draft_preview_headers();
}

$date = htmlspecialchars(substr((string)$post['date'], 0, 10), ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
$content = ($isPreview ? s64_draft_preview_banner() : '')
    . '<article><h2>'
    . htmlspecialchars((string)$post['title'], ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8')
    . '</h2><p><small>' . $date . '</small></p>'
    . s64_markup((string)$post['body'])
    . '</article>';

s64_render_layout($config, $pages, (string)$post['title'], $content);
