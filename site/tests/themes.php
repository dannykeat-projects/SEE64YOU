<?php
declare(strict_types=1);
require_once dirname(__DIR__) . '/lib/Config.php';
require_once dirname(__DIR__) . '/lib/Settings.php';
require_once dirname(__DIR__) . '/lib/Render.php';
function theme_check(bool $value, string $message): void {
    if (!$value) throw new RuntimeException($message);
}
$path = sys_get_temp_dir() . '/s64-themes-' . bin2hex(random_bytes(8)) . '.php';
$config = s64_default_config();
$config['direct_secret'] = 'preserve';
try {
    $settings = new S64Settings($path, $config);
    $preview = $settings->themeAction('preview', 'c64', ['name' => 'Preview']);
    theme_check(!file_exists($path), 'preview must not write');
    theme_check(str_contains(s64_theme_css($preview), 'assets/fonts/c64.ttf'), 'local C64 font');
    $config = $settings->themeAction('create', 'c64', ['name' => '<C64>']);
    $id = array_key_first($config['custom_themes']);
    theme_check((require $path)['direct_secret'] === 'preserve', 'secret preserved');
    $config = (new S64Settings($path, $config))->themeAction('select', $id);
    theme_check($config['theme'] === $id, 'custom selected');
    $css = s64_theme_css($config);
    theme_check(str_contains($css, 'Courier,monospace') && !str_contains($css, 'var('), 'legacy font/CSS fallback');
    foreach ([['font' => 'evil;url(x)'], ['padding' => 'expression(x)'], ['padding' => '-1'], ['padding' => '999'], ['text_color' => '#fff'], ['text_color' => '#ffffff;}</style>'], ['font' => []], ['text_color' => '#3e31a2']] as $bad) {
        $before = file_get_contents($path);
        try {
            (new S64Settings($path, $config))->themeAction('save', $id, ['name' => 'Bad'] + $bad);
            throw new RuntimeException('bad input accepted');
        } catch (InvalidArgumentException) {}
        theme_check(file_get_contents($path) === $before, 'rejected input must not write');
    }
    $config = (new S64Settings($path, $config))->themeAction('save', $id, ['name' => 'Renamed', 'padding' => '8', 'menu_layout' => 'stacked']);
    theme_check($config['custom_themes'][$id]['name'] === 'Renamed', 'rename');
    $config = (new S64Settings($path, $config))->themeAction('duplicate', $id, ['name' => 'Copy']);
    theme_check(count($config['custom_themes']) === 2, 'duplicate');
    $copy = array_key_last($config['custom_themes']);
    theme_check($config['custom_themes'][$copy]['values'] === $config['custom_themes'][$id]['values'], 'duplicate preserves saved values');
    $config = (new S64Settings($path, $config))->themeAction('delete', $id);
    theme_check($config['theme'] === 'plain' && !isset($config['custom_themes'][$id]), 'active deletion returns to plain');
    foreach (['save', 'delete'] as $action) {
        try { (new S64Settings($path, $config))->themeAction($action, 'c64', ['name' => 'Bad']); throw new RuntimeException('built-in altered'); }
        catch (InvalidArgumentException) {}
    }
    theme_check(s64_theme_css(['theme' => 'plain']) === '', 'plain has no CSS');
    theme_check(!str_contains(s64_custom_theme_css(['font' => '</style>']), '</style>'), 'render validation');
    echo "theme tests passed\n";
} finally { @unlink($path); }
