<?php
declare(strict_types=1);

/** Public, deliberately small text/link representation of published site pages. */
function s64_browser_ascii(string $value): string
{
    $value = html_entity_decode($value, ENT_QUOTES | ENT_HTML5, 'UTF-8');
    if (function_exists('iconv')) {
        $converted = iconv('UTF-8', 'ASCII//TRANSLIT//IGNORE', $value);
        if ($converted !== false) $value = $converted;
    }
    $value = preg_replace('/\s+/', ' ', $value) ?? '';
    return preg_replace('/[^\x20-\x7E]/', '?', $value) ?? '';
}

function s64_browser_text(string $value): void
{
    $value = trim(s64_browser_ascii(strip_tags($value)));
    while ($value !== '') {
        if (strlen($value) <= 100) {
            echo "T\t", $value, "\n";
            return;
        }
        $split = strrpos(substr($value, 0, 101), ' ');
        if ($split === false || $split < 20) $split = 100;
        echo "T\t", substr($value, 0, $split), "\n";
        $value = ltrim(substr($value, $split));
    }
}

function s64_browser_link(string $href, string $label): void
{
    $href = trim(s64_browser_ascii($href));
    $label = trim(s64_browser_ascii(strip_tags($label)));
    if ($href === '' || $label === '' || strlen($href) > 190
        || str_contains($href, ' ') || str_contains($href, '|')
        || !preg_match('~^(https?://[^/]+(?:/[^ ]*)?|/?[a-zA-Z0-9_./?&=%+;:-]+)$~i', $href)) return;
    echo "L\t", $href, "\t", substr($label, 0, 58), "\n";
}

function s64_browser_markup(string $content): void
{
    preg_match_all('~<a\s+href="([^"]+)"[^>]*>(.*?)</a>|<[^>]*>|[^<]+~is',
        $content, $matches, PREG_SET_ORDER);
    $text = '';
    foreach ($matches as $match) {
        $token = $match[0];
        if (isset($match[1]) && $match[1] !== '') {
            s64_browser_text($text);
            $text = '';
            s64_browser_link($match[1], $match[2]);
            continue;
        }
        if ($token[0] !== '<') {
            $text .= $token;
            continue;
        }
        if (preg_match('~^</?(?:p|li|h[1-6]|div|ul|article|br)\b~i', $token)) {
            s64_browser_text($text);
            $text = '';
        }
    }
    s64_browser_text($text);
}

function s64_browser_render(array $config, array $pages, string $title, string $content): void
{
    header('Content-Type: application/x-see64you-browser; charset=us-ascii');
    header('Cache-Control: no-store');
    header('X-Content-Type-Options: nosniff');
    echo "SEE64YOU-BROWSE/1\n";
    s64_browser_text((string)($config['site_title'] ?? 'SEE64YOU'));
    s64_browser_link('index.php', 'Home');
    foreach ($pages as $page) {
        if (($page['status'] ?? '') !== 'published' || ($page['slug'] ?? '') === 'home') continue;
        s64_browser_link('page.php?slug=' . rawurlencode((string)$page['slug']), (string)$page['title']);
    }
    s64_browser_link('posts.php', 'Posts');
    s64_browser_text($title);
    s64_browser_markup($content);
}
