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

        $this->validateContrast($updated);

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

    private function validateContrast(array $config): void
    {
        if ((string)($config['theme'] ?? 'plain') === 'plain') {
            return;
        }

        $background = (string)$config['background_color'];
        foreach (['text_color', 'link_color', 'visited_color'] as $key) {
            if ($this->contrastRatio((string)$config[$key], $background) < 4.5) {
                throw new InvalidArgumentException('insufficient_contrast');
            }
        }
    }

    private function contrastRatio(string $foreground, string $background): float
    {
        $first = $this->relativeLuminance($foreground);
        $second = $this->relativeLuminance($background);
        $lighter = max($first, $second);
        $darker = min($first, $second);

        return ($lighter + 0.05) / ($darker + 0.05);
    }

    private function relativeLuminance(string $color): float
    {
        $channels = [
            hexdec(substr($color, 1, 2)) / 255,
            hexdec(substr($color, 3, 2)) / 255,
            hexdec(substr($color, 5, 2)) / 255,
        ];

        foreach ($channels as &$channel) {
            $channel = $channel <= 0.04045
                ? $channel / 12.92
                : (($channel + 0.055) / 1.055) ** 2.4;
        }
        unset($channel);

        return 0.2126 * $channels[0]
            + 0.7152 * $channels[1]
            + 0.0722 * $channels[2];
    }
}
