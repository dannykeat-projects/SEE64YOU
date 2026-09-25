<?php
declare(strict_types=1);

function s64_remove_tree(string $path): void
{
    if (!file_exists($path)) {
        return;
    }

    if (is_file($path) || is_link($path)) {
        if (!unlink($path)) {
            throw new RuntimeException('Unable to remove file: ' . $path);
        }
        return;
    }

    $items = scandir($path);
    if ($items === false) {
        throw new RuntimeException('Unable to read directory: ' . $path);
    }

    foreach ($items as $item) {
        if ($item === '.' || $item === '..') {
            continue;
        }
        s64_remove_tree($path . DIRECTORY_SEPARATOR . $item);
    }

    if (!rmdir($path)) {
        throw new RuntimeException('Unable to remove directory: ' . $path);
    }
}

function s64_should_skip_package_path(string $relative): bool
{
    $relative = str_replace('\\', '/', $relative);

    if ($relative === 'tests' || str_starts_with($relative, 'tests/')) {
        return true;
    }

    return in_array($relative, [
        'config.php',
        'ENABLE_SETUP',
    ], true)
        || str_ends_with($relative, '.tmp')
        || str_contains($relative, '/.DS_Store')
        || str_ends_with($relative, '/Thumbs.db');
}

function s64_copy_site_tree(string $source, string $destination, string $relative = ''): void
{
    if ($relative !== '' && s64_should_skip_package_path($relative)) {
        return;
    }

    if (is_dir($source)) {
        if (!is_dir($destination)
            && !mkdir($destination, 0775, true)
            && !is_dir($destination)) {
            throw new RuntimeException('Unable to create directory: ' . $destination);
        }

        $items = scandir($source);
        if ($items === false) {
            throw new RuntimeException('Unable to read directory: ' . $source);
        }

        foreach ($items as $item) {
            if ($item === '.' || $item === '..') {
                continue;
            }

            $childRelative = $relative === '' ? $item : $relative . '/' . $item;
            s64_copy_site_tree(
                $source . DIRECTORY_SEPARATOR . $item,
                $destination . DIRECTORY_SEPARATOR . $item,
                $childRelative
            );
        }
        return;
    }

    if (!copy($source, $destination)) {
        throw new RuntimeException('Unable to copy file: ' . $source);
    }
}

function s64_package_site(string $repositoryRoot, string $outputDirectory): void
{
    $siteRoot = $repositoryRoot . DIRECTORY_SEPARATOR . 'site';
    if (!is_dir($siteRoot)) {
        throw new RuntimeException('Site directory not found: ' . $siteRoot);
    }

    if (file_exists($outputDirectory)) {
        s64_remove_tree($outputDirectory);
    }

    s64_copy_site_tree($siteRoot, $outputDirectory);

    $setupMarker = $outputDirectory . DIRECTORY_SEPARATOR . 'ENABLE_SETUP';
    if (file_put_contents($setupMarker, '') === false) {
        throw new RuntimeException('Unable to create setup marker');
    }

    if (file_exists($outputDirectory . DIRECTORY_SEPARATOR . 'config.php')) {
        throw new RuntimeException('Refusing to package local config.php');
    }
    if (is_dir($outputDirectory . DIRECTORY_SEPARATOR . 'tests')) {
        throw new RuntimeException('Refusing to package site tests');
    }
}

function s64_package_default_output(string $repositoryRoot): string
{
    return $repositoryRoot
        . DIRECTORY_SEPARATOR . 'dist'
        . DIRECTORY_SEPARATOR . 'see64you-site';
}

if (realpath((string)($_SERVER['SCRIPT_FILENAME'] ?? '')) === __FILE__) {
    $repositoryRoot = dirname(__DIR__);
    $output = $argv[1] ?? s64_package_default_output($repositoryRoot);

    try {
        s64_package_site($repositoryRoot, $output);
        fwrite(STDOUT, "Standalone upload package created: {$output}\n");
    } catch (Throwable $exception) {
        fwrite(STDERR, "Packaging failed: {$exception->getMessage()}\n");
        exit(1);
    }
}
