<?php
declare(strict_types=1);

require __DIR__ . '/lib/bootstrap.php';

$slug = (string)($_GET['slug'] ?? '');
$previewToken = (string)($_GET['preview'] ?? '');
try {
    $page = $documents->get('page', $slug);
} catch (InvalidArgumentException) {
    $page = null;
}

$isPreview = $page !== null
    && $page['status'] === 'draft'
    && $auth->validPreviewToken($previewToken, 'page', $slug);

if ($page === null || ($page['status'] !== 'published' && !$isPreview)) {
    http_response_code(404);
    $pages = $documents->list('page', true);
    s64_render_layout($config, $pages, 'Not found', '<h2>Not found</h2><p>The requested page was not found.</p>');
    exit;
}

if ($isPreview) {
    s64_draft_preview_headers();
}

$pages = $documents->list('page', true);
$content = ($isPreview ? s64_draft_preview_banner() : '')
    . s64_markup((string)$page['body']);
s64_render_layout($config, $pages, (string)$page['title'], $content);
