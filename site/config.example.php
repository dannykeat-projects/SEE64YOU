<?php
declare(strict_types=1);

/*
 * Copy this file to config.php.
 *
 * Create a password hash:
 *   php -r "echo password_hash('your-password', PASSWORD_DEFAULT), PHP_EOL;"
 *
 * Create token/direct secrets:
 *   php -r "echo bin2hex(random_bytes(32)), PHP_EOL;"
 */
return [
    'site_title' => 'My See64You Site',
    'theme' => 'plain', // plain, simple, c64
    'text_color' => '#b0b0ff',
    'background_color' => '#3e31a2',
    'link_color' => '#ffffff',
    'visited_color' => '#e49aff',

    // Leave empty to disable publishing API login until configured.
    'password_hash' => '',
    'token_secret' => '',
    'direct_secret' => '', // 32 random bytes as 64 hex chars; used by direct C64U auth
    'token_ttl' => 43200, // 12 hours
];
