<?php
declare(strict_types=1);

require __DIR__ . '/lib/bootstrap.php';

$pages = $documents->list('page', true);
$posts = $documents->list('post', true);
$years = S64PostArchive::years($posts);

$yearText = (string)($_GET['year'] ?? '');
$year = S64PostArchive::parseYear($yearText);
$page = S64PostArchive::parsePage((string)($_GET['page'] ?? ''));

if (($yearText !== '' && $year === null) || $page === 0) {
    http_response_code(404);
    s64_render_layout(
        $config,
        $pages,
        'Not found',
        '<h2>Not found</h2><p>The requested archive page was not found.</p>'
    );
    exit;
}

$filtered = S64PostArchive::filterByYear($posts, $year);
$listing = S64PostArchive::paginate($filtered, $page);
if (!$listing['valid']) {
    http_response_code(404);
    s64_render_layout(
        $config,
        $pages,
        'Not found',
        '<h2>Not found</h2><p>The requested archive page was not found.</p>'
    );
    exit;
}

$title = $year === null ? 'Posts' : 'Posts - ' . $year;
$html = '<h2>' . htmlspecialchars($title, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8') . '</h2>';

if (count($years) > 1) {
    $html .= '<p>Archive: <a href="posts.php">All</a>';
    foreach ($years as $archiveYear) {
        $html .= ' | <a href="posts.php?year='
            . rawurlencode($archiveYear)
            . '">' . htmlspecialchars($archiveYear, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8')
            . '</a>';
    }
    $html .= '</p>';
}

if ($listing['items'] === []) {
    $html .= '<p>No published posts yet.</p>';
} else {
    $html .= '<ul>';
    foreach ($listing['items'] as $post) {
        $date = htmlspecialchars(substr((string)$post['date'], 0, 10), ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
        $postTitle = htmlspecialchars((string)$post['title'], ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
        $slug = rawurlencode((string)$post['slug']);
        $html .= '<li>' . $date . ' - <a href="post.php?slug=' . $slug . '">' . $postTitle . '</a></li>';
    }
    $html .= '</ul>';
}

if ($listing['pages'] > 1) {
    $html .= '<p>';
    if ($listing['page'] > 1) {
        $previous = $listing['page'] - 1;
        $html .= '<a href="posts.php?'
            . ($year === null ? '' : 'year=' . rawurlencode($year) . '&amp;')
            . 'page=' . $previous
            . '">Previous</a>';
    }

    if ($listing['page'] > 1 && $listing['page'] < $listing['pages']) {
        $html .= ' | ';
    }

    if ($listing['page'] < $listing['pages']) {
        $next = $listing['page'] + 1;
        $html .= '<a href="posts.php?'
            . ($year === null ? '' : 'year=' . rawurlencode($year) . '&amp;')
            . 'page=' . $next
            . '">Next</a>';
    }
    $html .= '</p>';
}

s64_render_layout($config, $pages, $title, $html);
