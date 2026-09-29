<?php
declare(strict_types=1);

require __DIR__ . '/lib/bootstrap.php';

header('Cache-Control: no-store');
header('Referrer-Policy: no-referrer');

$isHttps = strtolower((string)($_SERVER['HTTPS'] ?? '')) === 'on'
    || strtolower((string)($_SERVER['HTTP_X_FORWARDED_PROTO'] ?? '')) === 'https';
if (!$isHttps) {
    http_response_code(400);
    echo '<!doctype html><html><body><h1>See64You</h1><p>C64U pairing is available over HTTPS only.</p></body></html>';
    exit;
}

$error = '';
$key = '';

if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'POST') {
    $password = (string)($_POST['password'] ?? '');

    if (!$auth->passwordValid($password)) {
        $error = 'Invalid publishing password.';
    } else {
        $directSecret = strtolower((string)($config['direct_secret'] ?? ''));

        if (preg_match('/^[a-f0-9]{64}$/', $directSecret) !== 1) {
            $updated = $config;
            $updated['direct_secret'] = bin2hex(random_bytes(32));
            $contents = "<?php\ndeclare(strict_types=1);\n\nreturn "
                . var_export($updated, true)
                . ";\n";
            $configPath = __DIR__ . '/config.php';
            $temporary = $configPath . '.tmp.' . bin2hex(random_bytes(4));

            if (file_put_contents($temporary, $contents, LOCK_EX) === false
                || !rename($temporary, $configPath)) {
                @unlink($temporary);
                $error = 'Could not update config.php.';
            } else {
                $directSecret = (string)$updated['direct_secret'];
            }
        }

        if ($error === '') {
            $key = strtoupper($directSecret);
        }
    }
}

function h(string $value): string
{
    return htmlspecialchars($value, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}
?><!doctype html>
<html>
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Pair C64U - See64You</title></head>
<body>
<h1>Pair C64U</h1>
<?php if ($key !== ''): ?>
<p>Enter this publishing key in See64You on the C64U:</p>
<p><code style="overflow-wrap:anywhere"><?= h($key) ?></code></p>
<p>This key authenticates C64U publishing requests. The publishing password itself is never sent over the C64U HTTP connection.</p>
<?php else: ?>
<?php if ($error !== ''): ?><p><strong><?= h($error) ?></strong></p><?php endif; ?>
<form method="post">
<p><label>Publishing password<br><input type="password" name="password" autocomplete="current-password"></label></p>
<p><button type="submit">Show C64U publishing key</button></p>
</form>
<?php endif; ?>
</body>
</html>
