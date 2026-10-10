<?php
declare(strict_types=1);
require dirname(__DIR__) . '/lib/Browser.php';
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
echo "browser wire: all tests passed\n";
