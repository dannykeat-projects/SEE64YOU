<?php
declare(strict_types=1);

require_once __DIR__ . '/Themes.php';
require_once __DIR__ . '/Auth.php';


/* Source links remain portable. Only known local .8tml destinations are
 * mapped to HTML views; unknown schemes/paths remain escaped source text. */
function s64_link_href(string $href, string $type): ?string
{
    if (preg_match('~^https?://~i', $href)) {
        return filter_var($href, FILTER_VALIDATE_URL) !== false ? $href : null;
    }
    if ($href === '/') return 'index.php';
    if (preg_match('~^(?:/|\./)?(?:(pages|posts)/)?([a-z0-9-]+)\.8tml(#[a-z0-9_-]+)?$~i', $href, $match)) {
        $kind = ($match[1] ?? '') !== '' ? strtolower($match[1])
            : (str_starts_with($href, '/') ? 'pages' : ($type === 'post' ? 'posts' : 'pages'));
        return ($kind === 'posts' ? 'post.php?slug=' : 'page.php?slug=')
            . rawurlencode(strtolower($match[2])) . ($match[3] ?? '');
    }
    if (preg_match('~^\.\./([a-z0-9-]+)\.8tml(#[a-z0-9_-]+)?$~i', $href, $match)) {
        return 'page.php?slug=' . rawurlencode(strtolower($match[1])) . ($match[2] ?? '');
    }
    return null;
}

function s64_inline_markup(string $text, string $type = 'page'): string
{
    $safe = htmlspecialchars($text, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');

    $safe = preg_replace('/\*\*([^*]+)\*\*/', '<strong>$1</strong>', $safe) ?? $safe;
    $safe = preg_replace('/\*([^*]+)\*/', '<em>$1</em>', $safe) ?? $safe;

    $safe = preg_replace_callback(
        '~\[([^\]]+)\]\(([^)\s]+)\)~i',
        static function (array $matches) use ($type): string {
            $url = html_entity_decode($matches[2], ENT_QUOTES | ENT_HTML5, 'UTF-8');
            $href = s64_link_href($url, $type);
            if ($href === null) return $matches[0];
            return '<a href="' . htmlspecialchars($href, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8') . '">'
                . $matches[1] . '</a>';
        },
        $safe
    ) ?? $safe;

    return $safe;
}

function s64_markup(string $source, string $type = 'page'): string
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
            $html[] = '<li>' . s64_inline_markup(substr($line, 2), $type) . '</li>';
            continue;
        }

        if ($listOpen) {
            $html[] = '</ul>';
            $listOpen = false;
        }

        if (str_starts_with($line, '## ')) {
            $html[] = '<h2>' . s64_inline_markup(substr($line, 3), $type) . '</h2>';
        } elseif (str_starts_with($line, '# ')) {
            $html[] = '<h1>' . s64_inline_markup(substr($line, 2), $type) . '</h1>';
        } elseif (trim($line) === '') {
            $html[] = '';
        } else {
            $html[] = '<p>' . s64_inline_markup($line, $type) . '</p>';
        }
    }

    if ($listOpen) {
        $html[] = '</ul>';
    }

    return implode("\n", $html);
}

function s64_draft_preview_headers(): void
{
    header('Cache-Control: private, no-store');
    header('X-Robots-Tag: noindex, nofollow');
    header('Referrer-Policy: no-referrer');
}

function s64_draft_preview_banner(): string
{
    return '<p><strong>Draft preview</strong></p>';
}

function s64_safe_color(mixed $value, string $fallback): string
{
    $value = (string)$value;
    return preg_match('/^#[0-9a-f]{6}$/i', $value) ? s64_theme_nearest_colour($value) : $fallback;
}

function s64_theme_css(array $config): string
{
    $theme = (string)($config['theme'] ?? 'plain');
    if ($theme === 'plain') {
        return '';
    }
    if (isset($config['custom_themes'][$theme]['values']) && is_array($config['custom_themes'][$theme]['values'])) {
        return s64_custom_theme_css($config['custom_themes'][$theme]['values']);
    }

    $text = s64_safe_color($config['text_color'] ?? '', '#ffffff');
    $background = s64_safe_color($config['background_color'] ?? '', '#2e2c9b');
    $link = s64_safe_color($config['link_color'] ?? '', '#edf171');
    $visited = s64_safe_color($config['visited_color'] ?? '', '#a9ff9f');

    // Old built-in C64 selections use Simple styling without modifying saved configuration.
    return 'body{max-width:48em;margin:2em auto;padding:0 1em;font-family:system-ui,sans-serif;line-height:1.5;'
        . "color:$text;background:$background}"
        . "a{color:$link;text-decoration:underline}a:visited{color:$visited}"
        . "a:focus-visible{outline:2px solid currentColor;outline-offset:2px}"
        . "nav{margin:1em 0}";
}

function s64_theme_admin_visible(array $config): bool
{
    if (session_status() === PHP_SESSION_NONE) {
        if (!isset($_COOKIE['s64_theme_session'])) {
            return false;
        }
        session_name('s64_theme_session');
        session_cache_limiter('');
        if (!session_start(['read_and_close' => true])) {
            return false;
        }
    }

    return (new S64Auth($config))->valid((string)($_SESSION['theme_token'] ?? ''));
}

function s64_active_theme_values(array $config): ?array
{
    $theme = (string)($config['theme'] ?? 'plain');
    if (!isset($config['custom_themes'][$theme]['values']) || !is_array($config['custom_themes'][$theme]['values'])) {
        return null;
    }
    try {
        return s64_validate_theme(s64_theme_palette_values($config['custom_themes'][$theme]['values']));
    } catch (InvalidArgumentException) {
        return null;
    }
}

function s64_browser_requested(string $accept): bool
{
    foreach (explode(',', $accept) as $entry) {
        $parts = explode(';', $entry);
        if (strtolower(trim(array_shift($parts))) !== 'application/x-see64you-browser') continue;
        $quality = 1.0;
        foreach ($parts as $part) {
            if (!preg_match('/^q\s*=/i', trim($part))) continue;
            if (!preg_match('/^q\s*=\s*([01](?:\.\d{0,3})?)$/i', trim($part), $match)) return false;
            $quality = (float)$match[1];
        }
        return $quality > 0.0 && $quality <= 1.0;
    }
    return false;
}

function s64_render_layout(array $config, array $pages, string $title, string $content): void
{
    if (s64_browser_requested((string)($_SERVER['HTTP_ACCEPT'] ?? ''))) {
        require_once __DIR__ . '/Browser.php';
        s64_browser_render($config, $pages, $title, $content);
        return;
    }
    $themeValues = s64_active_theme_values($config);
    $rawSiteTitle = $themeValues !== null && $themeValues['header_title'] !== ''
        ? $themeValues['header_title']
        : (string)$config['site_title'];
    $siteTitle = htmlspecialchars($rawSiteTitle, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
    $pageTitle = htmlspecialchars($title, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
    $css = s64_theme_css($config);

    echo "<!doctype html>\n<html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    echo '<title>' . ($title === '' ? $siteTitle : $pageTitle . ' - ' . $siteTitle) . '</title>';
    if ($css !== '') {
        echo '<style>' . $css . '</style>';
    }
    echo "</head><body>\n<div class=\"s64-screen\">";
    echo '<header>';
    if ($themeValues === null || $themeValues['show_title'] === 'yes') {
        echo '<h1>' . $siteTitle . '</h1>';
    }
    if ($themeValues === null || $themeValues['show_menu'] === 'yes') {
        echo '<nav><div class="s64-menu">';
        echo '<a href="index.php">Home</a>';

        foreach ($pages as $page) {
            if ($page['slug'] === 'home') {
                continue;
            }
            echo '<span class="s64-menu-separator"> | </span><a href="page.php?slug='
                . rawurlencode((string)$page['slug']) . '">'
                . htmlspecialchars((string)$page['title'], ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8')
                . '</a>';
        }

        echo '<span class="s64-menu-separator"> | </span><a href="posts.php">Posts</a>';
        if (s64_theme_admin_visible($config)) {
            echo '<span class="s64-menu-separator"> | </span><a href="themes.php">Themes</a>';
        }
        echo '</div></nav>';
    }
    if ($themeValues === null || $themeValues['show_separator'] === 'yes') {
        echo '<hr>';
    }
    echo "</header>\n<main><div class=\"s64-panel\">";
    echo $content;
    echo "</div></main></div>\n</body></html>";
}
