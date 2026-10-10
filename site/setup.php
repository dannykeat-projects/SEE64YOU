<?php
declare(strict_types=1);

$configPath = __DIR__ . '/config.php';
$setupMarker = __DIR__ . '/ENABLE_SETUP';

if (is_file($configPath) || !is_file($setupMarker)) {
    http_response_code(404);
    echo '<!doctype html><html><body><h1>See64You</h1><p>Setup is not available.</p></body></html>';
    exit;
}

$error = '';
$complete = false;

if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'POST') {
    $siteTitle = trim((string)($_POST['site_title'] ?? ''));
    $theme = (string)($_POST['theme'] ?? 'plain');
    $password = (string)($_POST['password'] ?? '');
    $confirm = (string)($_POST['confirm'] ?? '');

    if ($siteTitle === '') {
        $error = 'Site title is required.';
    } elseif (!in_array($theme, ['plain', 'simple', 'c64'], true)) {
        $error = 'Invalid theme.';
    } elseif (strlen($password) < 8) {
        $error = 'Password must be at least 8 characters.';
    } elseif (!hash_equals($password, $confirm)) {
        $error = 'Passwords do not match.';
    } else {
        $config = [
            'site_title' => $siteTitle,
            'theme' => $theme,
            'text_color' => '#b0b0ff',
            'background_color' => '#3e31a2',
            'link_color' => '#ffffff',
            'visited_color' => '#e49aff',
            'password_hash' => password_hash($password, PASSWORD_DEFAULT),
            'token_secret' => bin2hex(random_bytes(32)),
            'direct_secret' => bin2hex(random_bytes(32)),
            'token_ttl' => 43200,
        ];

        $contents = "<?php\ndeclare(strict_types=1);\n\nreturn "
            . var_export($config, true)
            . ";\n";

        $temporary = $configPath . '.tmp.' . bin2hex(random_bytes(4));
        if (file_put_contents($temporary, $contents, LOCK_EX) === false || !rename($temporary, $configPath)) {
            @unlink($temporary);
            $error = 'Could not write config.php. Check folder permissions.';
        } else {
            @unlink($setupMarker);
            $complete = true;
        }
    }
}

function h(string $value): string
{
    return htmlspecialchars($value, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

?><!doctype html>
<html>
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>See64You setup</title></head>
<body>
<h1>See64You setup</h1>
<?php if ($complete): ?>
<p>Setup complete. Your site is ready.</p>
<p><a href="start.php?step=3">Continue: pair your C64U and publish your first page or post</a></p>
<p><a href="index.php">Open the site</a></p>
<?php else: ?>
<p><a href="start.php?step=2">First page or post walkthrough</a></p>
<?php if ($error !== ''): ?><p><strong><?= h($error) ?></strong></p><?php endif; ?>
<form method="post">
<p><label>Site title<br><input name="site_title" value="<?= h((string)($_POST['site_title'] ?? 'My See64You Site')) ?>"></label></p>
<p><label>Theme<br>
<select name="theme">
<?php foreach (['plain' => 'Plain HTML', 'simple' => 'Simple', 'c64' => 'C64'] as $value => $label): ?>
<option value="<?= h($value) ?>"<?= (string)($_POST['theme'] ?? 'plain') === $value ? ' selected' : '' ?>><?= h($label) ?></option>
<?php endforeach; ?>
</select></label></p>
<p><label>Publishing password<br><input type="password" name="password" autocomplete="new-password"></label></p>
<p><label>Confirm password<br><input type="password" name="confirm" autocomplete="new-password"></label></p>
<p><button type="submit">Create site configuration</button></p>
</form>
<?php endif; ?>
</body>
</html>
