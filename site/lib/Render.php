<?php
declare(strict_types=1);

function s64_inline_markup(string $text): string
{
    $safe = htmlspecialchars($text, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');

    $safe = preg_replace('/\*\*([^*]+)\*\*/', '<strong>$1</strong>', $safe) ?? $safe;
    $safe = preg_replace('/\*([^*]+)\*/', '<em>$1</em>', $safe) ?? $safe;

    $safe = preg_replace_callback(
        '~\[([^\]]+)\]\((https?://[^)\s]+)\)~i',
        static function (array $matches): string {
            $url = html_entity_decode($matches[2], ENT_QUOTES | ENT_HTML5, 'UTF-8');
            if (filter_var($url, FILTER_VALIDATE_URL) === false) {
                return $matches[0];
            }
            return '<a href="' . htmlspecialchars($url, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8') . '">'
                . $matches[1] . '</a>';
        },
        $safe
    ) ?? $safe;

    return $safe;
}

function s64_markup(string $source): string
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
            $html[] = '<li>' . s64_inline_markup(substr($line, 2)) . '</li>';
            continue;
        }

        if ($listOpen) {
            $html[] = '</ul>';
            $listOpen = false;
        }

        if (str_starts_with($line, '## ')) {
            $html[] = '<h2>' . s64_inline_markup(substr($line, 3)) . '</h2>';
        } elseif (str_starts_with($line, '# ')) {
            $html[] = '<h1>' . s64_inline_markup(substr($line, 2)) . '</h1>';
        } elseif (trim($line) === '') {
            $html[] = '';
        } else {
            $html[] = '<p>' . s64_inline_markup($line) . '</p>';
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
    return preg_match('/^#[0-9a-f]{6}$/i', $value) ? $value : $fallback;
}

function s64_theme_css(array $config): string
{
    $theme = (string)($config['theme'] ?? 'plain');
    if ($theme === 'plain') {
        return '';
    }

    $text = s64_safe_color($config['text_color'] ?? '', '#222222');
    $background = s64_safe_color($config['background_color'] ?? '', '#ffffff');
    $link = s64_safe_color($config['link_color'] ?? '', '#0000ee');
    $visited = s64_safe_color($config['visited_color'] ?? '', '#551a8b');

    if ($theme === 'c64') {
        return 'body{max-width:40em;margin:2em auto;padding:1em;font-family:monospace;'
            . "color:$text;background:$background;line-height:1.5}"
            . "a{color:$link;text-decoration:underline}a:visited{color:$visited}"
            . "a:focus-visible{outline:2px solid currentColor;outline-offset:2px}"
            . "nav{margin:1em 0}hr{border:0;border-top:1px solid $text}";
    }

    return 'body{max-width:48em;margin:2em auto;padding:0 1em;font-family:system-ui,sans-serif;line-height:1.5;'
        . "color:$text;background:$background}"
        . "a{color:$link;text-decoration:underline}a:visited{color:$visited}"
        . "a:focus-visible{outline:2px solid currentColor;outline-offset:2px}"
        . "nav{margin:1em 0}";
}

function s64_render_layout(array $config, array $pages, string $title, string $content): void
{
    $siteTitle = htmlspecialchars((string)$config['site_title'], ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
    $pageTitle = htmlspecialchars($title, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
    $css = s64_theme_css($config);

    echo "<!doctype html>\n<html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    echo '<title>' . ($title === '' ? $siteTitle : $pageTitle . ' - ' . $siteTitle) . '</title>';
    if ($css !== '') {
        echo '<style>' . $css . '</style>';
    }
    echo "</head><body>\n";
    echo '<header><h1>' . $siteTitle . '</h1><nav>';
    echo '<a href="index.php">Home</a>';

    foreach ($pages as $page) {
        if ($page['slug'] === 'home') {
            continue;
        }
        echo ' | <a href="page.php?slug='
            . rawurlencode((string)$page['slug']) . '">'
            . htmlspecialchars((string)$page['title'], ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8')
            . '</a>';
    }

    echo ' | <a href="posts.php">Posts</a>';
    echo "</nav><hr></header>\n<main>";
    echo $content;
    echo "</main>\n</body></html>";
}
