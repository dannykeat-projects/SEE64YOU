<?php
declare(strict_types=1);
require dirname(__DIR__) . '/lib/Browser.php';
require_once dirname(__DIR__) . '/lib/Render.php';
function require_browser(bool $valid, string $message): void {
    if (!$valid) throw new RuntimeException($message);
}
ob_start();
s64_browser_render(
    ['site_title' => 'Sample Site'],
    [['slug' => 'home', 'status' => 'published', 'title' => 'Home'],
     ['slug' => 'about', 'status' => 'published', 'title' => 'About'],
     ['slug' => 'private', 'status' => 'draft', 'title' => 'Private']],
    'Welcome',
    '<h1>Hello</h1><p>A <strong>small</strong> site.</p>'
    . '<p>Visit <a href="https://example.org/path?x=1&amp;y=2">Other site</a>.</p>'
);
$output = ob_get_clean();
require_browser(str_starts_with($output, "SEE64YOU-BROWSE/1\n"), 'wire header');
require_browser(str_contains($output, "L\tpage.php?slug=about\tAbout\n"), 'published nav');
require_browser(!str_contains($output, 'private'), 'draft not in public nav');
require_browser(str_contains($output, "T\tA small site.\n"), 'inline formatting');
require_browser(str_contains($output, "L\thttps://example.org/path?x=1&y=2\tOther site\n"), 'entity decode');
require_browser(!str_contains($output, '<strong>'), 'HTML not emitted');
require_browser(s64_browser_ascii("one\ntwo\tthree") === 'one two three', 'whitespace remains readable');
$_SERVER['HTTP_ACCEPT'] = 'application/x-see64you-browser;q=0, text/html';
ob_start();
s64_render_layout(['site_title' => 'Sample Site', 'theme' => 'plain'], [], 'Welcome', '<p>Hello</p>');
$rejected = ob_get_clean();
require_browser(!str_starts_with($rejected, "SEE64YOU-BROWSE/1\n"), 'explicitly rejected browser format uses HTML');
unset($_SERVER['HTTP_ACCEPT']);
require_browser(s64_browser_requested('text/html, Application/X-See64you-Browser; q=0.5'), 'exact media type is case insensitive');
require_browser(!s64_browser_requested('application/x-see64you-browser-extra'), 'substring media type rejected');
require_browser(!s64_browser_requested('application/x-see64you-browser;q=invalid'), 'invalid quality rejected');
echo "browser wire: all tests passed\n";
