<?php
declare(strict_types=1);

header('Cache-Control: no-store');
header('X-Content-Type-Options: nosniff');
header('Content-Type: text/plain; charset=us-ascii');
if (!in_array($_SERVER['REQUEST_METHOD'] ?? 'GET', ['GET', 'HEAD'], true)) {
    header('Allow: GET, HEAD');
    http_response_code(405);
    exit;
}

require __DIR__ . '/lib/bootstrap.php';

try {
    $type = $_GET['type'] ?? 'page';
    $slug = $_GET['slug'] ?? '';
    $document = is_string($type) && is_string($slug) ? $documents->get($type, $slug) : null;
    if ($document === null || $document['status'] !== 'published' || !isset($document['raw8tml'])) {
        http_response_code(404);
        exit;
    }
} catch (InvalidArgumentException | RuntimeException) {
    http_response_code(404);
    exit;
}

// Serve the live guarded record, never a static copy that could outlive unpublish.
header('Content-Type: text/8tml; charset=us-ascii');
header('Content-Length: ' . strlen($document['raw8tml']));
if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'GET') echo $document['raw8tml'];
