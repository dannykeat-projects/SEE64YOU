<?php
declare(strict_types=1);

require_once dirname(__DIR__) . '/package-site.php';

function package_check(bool $condition, string $message): void
{
    if (!$condition) {
        fwrite(STDERR, "FAIL: {$message}\n");
        exit(1);
    }
}

$root = sys_get_temp_dir() . '/see64you-package-test-' . bin2hex(random_bytes(4));
$repo = $root . '/repo';
$site = $repo . '/site';
$output = $root . '/out';

mkdir($site . '/lib', 0775, true);
mkdir($site . '/tests', 0775, true);
mkdir($site . '/pages', 0775, true);
mkdir($site . '/posts', 0775, true);

file_put_contents($site . '/index.php', '<?php echo "ok";');
file_put_contents($site . '/lib/App.php', '<?php');
file_put_contents($site . '/pages/home.s64.php', '<?php');
file_put_contents($site . '/posts/welcome.s64.php', '<?php');
file_put_contents($site . '/config.example.php', '<?php return [];');
file_put_contents($site . '/config.php', '<?php return ["secret" => true];');
file_put_contents($site . '/ENABLE_SETUP', '');
file_put_contents($site . '/tests/run.php', '<?php');
file_put_contents($site . '/stale.tmp', 'temporary');

try {
    s64_package_site($repo, $output);

    package_check(is_file($output . '/index.php'), 'runtime entry point copied');
    package_check(is_file($output . '/lib/App.php'), 'runtime library copied');
    package_check(is_file($output . '/pages/home.s64.php'), 'starter page copied');
    package_check(is_file($output . '/posts/welcome.s64.php'), 'starter post copied');
    package_check(is_file($output . '/config.example.php'), 'config example copied');
    package_check(is_file($output . '/ENABLE_SETUP'), 'fresh package enables one-time setup');
    package_check(!file_exists($output . '/config.php'), 'local config excluded');
    package_check(!is_dir($output . '/tests'), 'test suite excluded');
    package_check(!file_exists($output . '/stale.tmp'), 'temporary files excluded');

    file_put_contents($output . '/old.txt', 'old');
    s64_package_site($repo, $output);
    package_check(!file_exists($output . '/old.txt'), 'existing output is replaced cleanly');

    echo "site package tests passed\n";
} finally {
    s64_remove_tree($root);
}
