<?php
declare(strict_types=1);

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
            if (!in_array($theme, ['plain', 'simple', 'c64'], true)) {
                throw new InvalidArgumentException('invalid_theme');
            }
            $updated['theme'] = $theme;
        }

        foreach (['text_color', 'background_color', 'link_color', 'visited_color'] as $key) {
            if (!array_key_exists($key, $input)) {
                continue;
            }

            $value = strtolower((string)$input[$key]);
            if (!preg_match('/^#[0-9a-f]{6}$/', $value)) {
                throw new InvalidArgumentException('invalid_color');
            }
            $updated[$key] = $value;
        }

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

        return $this->publicValues($updated);
    }
}
