<?php
declare(strict_types=1);

require_once __DIR__ . '/Themes.php';

final class S64Settings
{
    public function __construct(
        private readonly string $configPath,
        private readonly array $config
    ) {
    }

    public function publicValues(?array $source = null): array
    {
        $source ??= $this->config;

        return [
            'site_title' => (string)$source['site_title'],
            'theme' => (string)$source['theme'],
            'text_color' => (string)$source['text_color'],
            'background_color' => (string)$source['background_color'],
            'link_color' => (string)$source['link_color'],
            'visited_color' => (string)$source['visited_color'],
        ];
    }

    public function update(array $input): array
    {
        $updated = $this->config;

        if (array_key_exists('site_title', $input)) {
            $title = trim((string)$input['site_title']);
            if ($title === '' || strlen($title) > 120) {
                throw new InvalidArgumentException('invalid_site_title');
            }
            $updated['site_title'] = $title;
        }

        if (array_key_exists('theme', $input)) {
            $theme = (string)$input['theme'];
            if (!in_array($theme, ['plain', 'simple'], true)
                && !isset($updated['custom_themes'][$theme])) {
                throw new InvalidArgumentException('invalid_theme');
            }
            $updated['theme'] = $theme;
        }

        foreach (['text_color', 'background_color', 'link_color', 'visited_color'] as $key) {
            if (!array_key_exists($key, $input)) {
                continue;
            }

            $value = strtolower((string)$input[$key]);
            if (!isset(s64_theme_palette()[$value])) {
                throw new InvalidArgumentException('invalid_color');
            }
            $updated[$key] = $value;
        }

        $this->validateContrast($updated);
        $this->save($updated);
        return $this->publicValues($updated);
    }

    public function themeAction(string $action, string $id, array $input = []): array
    {
        $updated = $this->config;
        $themes = $updated['custom_themes'] ?? [];
        $builtIn = in_array($id, ['plain', 'simple'], true);
        if (!$builtIn && !isset($themes[$id])) throw new InvalidArgumentException('invalid_theme');
        if ($action === 'select') {
            $updated['theme'] = $id;
        } elseif ($action === 'delete') {
            if ($builtIn) throw new InvalidArgumentException('built_in_theme');
            unset($themes[$id]);
            if ($updated['theme'] === $id) $updated['theme'] = 'plain';
        } elseif (in_array($action, ['create', 'duplicate', 'save', 'preview'], true)) {
            if ($builtIn && $action === 'save') throw new InvalidArgumentException('built_in_theme');
            $name = trim(is_string($input['name'] ?? null) ? $input['name'] : '');
            if ($name === '' || strlen($name) > 80 || preg_match('/[\x00-\x1f\x7f]/', $name)) throw new InvalidArgumentException('invalid_theme_name');
            $base = $builtIn ? s64_theme_defaults($updated) : s64_theme_palette_values($themes[$id]['values']);
            if ($id === 'simple') { $base['font'] = 'sans'; $base['content_width'] = '48'; }
            $values = s64_validate_theme($action === 'duplicate' ? [] : $input, $base);
            if ($action === 'preview') return array_replace($updated, ['theme' => 'preview', 'custom_themes' => ['preview' => ['name' => $name, 'values' => $values]]]);
            if ($action !== 'save') {
                if (count($themes) >= 32) throw new InvalidArgumentException('theme_limit');
                $id = 'custom-' . bin2hex(random_bytes(8));
            }
            $themes[$id] = ['name' => $name, 'values' => $values];
        } else {
            throw new InvalidArgumentException('invalid_action');
        }
        $updated['custom_themes'] = $themes;
        $this->save($updated);
        return $updated;
    }

    private function save(array $updated): void
    {

        $contents = "<?php\ndeclare(strict_types=1);\n\nreturn "
            . var_export($updated, true)
            . ";\n";

        $temporary = $this->configPath . '.tmp.' . bin2hex(random_bytes(4));
        if (file_put_contents($temporary, $contents, LOCK_EX) === false) {
            throw new RuntimeException('settings_write_failed');
        }

        if (!rename($temporary, $this->configPath)) {
            @unlink($temporary);
            throw new RuntimeException('settings_write_failed');
        }

    }

    private function validateContrast(array $config): void
    {
        if ((string)($config['theme'] ?? 'plain') === 'plain') {
            return;
        }
        if (isset($config['custom_themes'][$config['theme']])) {
            s64_validate_theme(s64_theme_palette_values($config['custom_themes'][$config['theme']]['values']));
            return;
        }

        $background = (string)$config['background_color'];
        foreach (['text_color', 'link_color', 'visited_color'] as $key) {
            if (s64_contrast_ratio((string)$config[$key], $background) < 4.5) {
                throw new InvalidArgumentException('insufficient_contrast');
            }
        }
    }

}
