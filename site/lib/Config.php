<?php
declare(strict_types=1);

function s64_default_config(): array
{
    return [
        'site_title' => 'See64You',
        'theme' => 'plain',
        'text_color' => '#b0b0ff',
        'background_color' => '#3e31a2',
        'link_color' => '#ffffff',
        'visited_color' => '#e49aff',
        'password_hash' => '',
        'token_secret' => '',
        'token_ttl' => 43200,
    ];
}

function s64_config(): array
{
    static $config = null;

    if ($config !== null) {
        return $config;
    }

    $config = s64_default_config();
    $path = dirname(__DIR__) . '/config.php';

    if (is_file($path)) {
        $loaded = require $path;
        if (is_array($loaded)) {
            $config = array_replace($config, $loaded);
        }
    }

    return $config;
}
