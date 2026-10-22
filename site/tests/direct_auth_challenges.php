<?php
declare(strict_types=1);
require_once dirname(__DIR__) . '/lib/DirectAuth.php';

$directory = sys_get_temp_dir() . '/see64you-direct-auth-' . bin2hex(random_bytes(5));
$auth = new S64DirectAuth(['direct_secret' => str_repeat('a', 64)], $directory);
try {
    $first = $auth->challenge();
    for ($i = 1; $i < 128; ++$i) $auth->challenge();

    try {
        $auth->challenge();
        throw new RuntimeException('unbounded challenge issuance');
    } catch (RuntimeException $error) {
        if ($error->getMessage() !== 'direct_auth_busy') throw $error;
    }

    $uri = '/api.php?action=status';
    $signature = S64DirectAuth::signature(str_repeat('a', 64),
        $first['challenge'], 'GET', $uri, '');
    if (!$auth->verify($first['challenge'], $signature, 'GET', $uri, '')
        || $auth->verify($first['challenge'], $signature, 'GET', $uri, '')) {
        throw new RuntimeException('challenge consumption/replay');
    }

    $replacement = $auth->challenge();
    $expired = $directory . '/challenge-' . $replacement['challenge'];
    file_put_contents($expired, (string)(time() - 1));
    $auth->challenge();
    if (file_exists($expired)
        || count(glob($directory . '/challenge-*') ?: []) !== 128) {
        throw new RuntimeException('expired challenges not reclaimed');
    }
    echo "Direct-auth challenge expiry, cap and single-use tests passed\n";
} finally {
    foreach (glob($directory . '/*') ?: [] as $file) {
        if (is_file($file)) unlink($file);
    }
    if (is_file($directory . '/.challenge-issue.lock')) {
        unlink($directory . '/.challenge-issue.lock');
    }
    if (is_dir($directory)) rmdir($directory);
}
