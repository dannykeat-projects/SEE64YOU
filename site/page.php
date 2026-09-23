<?php
declare(strict_types=1);

require __DIR__ . '/lib/bootstrap.php';

$slug = (string)($_GET['slug'] ?? '');
try {
    $page = $documents->get('page', $slug);
} catch (InvalidArgumentException) {
    $page = null;
}

if ($page === null || $page['status'] !== 'published') {
    http_response_code(404);
    $pages = $documents->list('page', true);
    s64_render_layout($config, $pages, 'Not found', '<h2>Not found</h2><p>The requested page was not found.</p>');
    exit;
}

$pages = $documents->list('page', true);
s64_render_layout($config, $pages, (string)$page['title'], s64_markup((string)$page['body']));
