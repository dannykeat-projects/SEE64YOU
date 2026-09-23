<?php
declare(strict_types=1);

require __DIR__ . '/lib/bootstrap.php';

$pages = $documents->list('page', true);
$home = $documents->get('page', 'home');

if ($home === null || $home['status'] !== 'published') {
    $home = null;
    foreach ($pages as $candidate) {
        $home = $documents->get('page', (string)$candidate['slug']);
        if ($home !== null) {
            break;
        }
    }
}

$title = $home['title'] ?? '';
$content = $home === null
    ? '<p>No published home page yet.</p>'
    : s64_markup((string)$home['body']);

s64_render_layout($config, $pages, (string)$title, $content);
