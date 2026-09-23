<?php
declare(strict_types=1);

require __DIR__ . '/lib/bootstrap.php';

$pages = $documents->list('page', true);
$posts = $documents->list('post', true);
$html = '<h2>Posts</h2>';

if ($posts === []) {
    $html .= '<p>No published posts yet.</p>';
} else {
    $html .= '<ul>';
    foreach ($posts as $post) {
        $date = htmlspecialchars(substr((string)$post['date'], 0, 10), ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
        $title = htmlspecialchars((string)$post['title'], ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
        $slug = rawurlencode((string)$post['slug']);
        $html .= '<li>' . $date . ' - <a href="post.php?slug=' . $slug . '">' . $title . '</a></li>';
    }
    $html .= '</ul>';
}

s64_render_layout($config, $pages, 'Posts', $html);
