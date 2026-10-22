<?php
declare(strict_types=1);
/** Create only a NEW disposable public site, with no publishing secrets. */
if ($argc !== 2 || file_exists($argv[1])) {
    fwrite(STDERR, "Usage: php tools/create_browser_test_site.php NEW_DIRECTORY\n");
    exit(1);
}
$source = dirname(__DIR__) . '/site';
$destination = $argv[1];
if (!mkdir($destination, 0777, true)) throw new RuntimeException('create_fixture_failed');
foreach (['index.php', 'page.php', 'post.php', 'posts.php'] as $name) {
    if (!copy($source . '/' . $name, $destination . '/' . $name)) {
        throw new RuntimeException('copy_route_failed');
    }
}
mkdir($destination . '/lib');
foreach (glob($source . '/lib/*.php') ?: [] as $path) {
    if (!copy($path, $destination . '/lib/' . basename($path))) {
        throw new RuntimeException('copy_library_failed');
    }
}
require_once $source . '/lib/Documents.php';
$documents = new S64Documents($destination);
foreach ([['page', 'home', 'Home fixture', 'Home body fixture.'],
    ['page', 'about', 'Page fixture', "Page body fixture.\n\n" . str_repeat("Long paragraph for paging.\n\n", 35)],
    ['post', 'first', 'Post fixture', 'Post body fixture.']] as [$type, $slug, $title, $body]) {
    $documents->save(compact('type', 'slug', 'title', 'body') + ['status' => 'published']);
}
$slow = <<<'PHP'
<?php
header('Content-Type: application/x-see64you-browser; charset=us-ascii');
sleep(4);
echo "SEE64YOU-BROWSE/1\nT\tSlow body fixture.\n";
PHP;
if (file_put_contents($destination . '/slow.php', $slow) === false) {
    throw new RuntimeException('write_slow_fixture_failed');
}
echo "Created disposable browser site: $destination\n";
