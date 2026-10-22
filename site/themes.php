<?php
declare(strict_types=1);
require __DIR__ . '/lib/bootstrap.php';

header('Cache-Control: private, no-store');
header('Referrer-Policy: no-referrer');
header('X-Robots-Tag: noindex, nofollow');
$https = strtolower((string)($_SERVER['HTTPS'] ?? '')) === 'on'
    || strtolower((string)($_SERVER['HTTP_X_FORWARDED_PROTO'] ?? '')) === 'https';
$local = in_array($_SERVER['REMOTE_ADDR'] ?? '', ['127.0.0.1', '::1'], true)
    && in_array(explode(':', (string)($_SERVER['HTTP_HOST'] ?? ''))[0], ['localhost', '127.0.0.1'], true);
if (!$https && !$local) {
    http_response_code(400);
    echo '<p>Theme login requires HTTPS. Use the HTTPS address of this site.</p>';
    exit;
}
session_name('s64_theme_session');
session_set_cookie_params(['httponly' => true, 'secure' => $https, 'samesite' => 'Strict', 'path' => rtrim(str_replace('\\', '/', dirname($_SERVER['SCRIPT_NAME'])), '/') . '/']);
session_start();
function theme_h(string $value): string { return htmlspecialchars($value, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8'); }
function theme_post(string $key): string { return is_string($_POST[$key] ?? null) ? $_POST[$key] : ''; }
$error = ''; $message = ''; $preview = null;
$action = theme_post('action');
$loggedIn = $auth->valid((string)($_SESSION['theme_token'] ?? ''));
$_SESSION['theme_csrf'] ??= bin2hex(random_bytes(32));
if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'POST') {
    if (!hash_equals($_SESSION['theme_csrf'], theme_post('csrf'))) {
        http_response_code(403); $error = 'The form expired. Please try again.';
    } else {
        try {
            if ($action === 'login') {
                $login = $auth->login(theme_post('password'));
                session_regenerate_id(true);
                $_SESSION['theme_token'] = $login['token'];
                $loggedIn = true;
            } elseif ($action === 'logout') {
                unset($_SESSION['theme_token']); $loggedIn = false;
            } elseif (!$loggedIn) {
                http_response_code(403); $error = 'Please log in first.';
            } else {
                if ($action === 'delete' && theme_post('confirm_delete') !== 'yes') throw new InvalidArgumentException('confirm_delete');
                $next = $settings->themeAction($action, theme_post('id'), $_POST);
                if ($action === 'preview') $preview = $next;
                else {
                    $created = array_diff_key($next['custom_themes'] ?? [], $config['custom_themes'] ?? []);
                    if ($created !== []) $_GET['id'] = array_key_first($created);
                    $config = $next; $message = 'Theme changes saved.';
                }
            }
        } catch (InvalidArgumentException $exception) {
            $error = 'Could not apply theme: ' . str_replace('_', ' ', $exception->getMessage()) . '. Colours must have readable contrast (4.5:1).';
        } catch (RuntimeException) {
            $error = $action === 'login' ? 'Invalid password or site not configured.' : 'Could not save the theme. Please try again.';
        }
    }
}
$csrf = '<input type="hidden" name="csrf" value="' . theme_h($_SESSION['theme_csrf']) . '">';
ob_start();
?>
<h2>Website themes</h2>
<?php if ($error !== ''): ?><p><strong><?= theme_h($error) ?></strong></p><?php endif; ?>
<?php if ($message !== ''): ?><p><?= theme_h($message) ?></p><?php endif; ?>
<?php if (!$loggedIn): ?>
<form method="post" action="themes.php"><?= $csrf ?><input type="hidden" name="action" value="login">
<p><label>Site publishing password<br><input type="password" name="password" size="20"></label></p>
<p><button type="submit">Log in</button></p></form>
<?php else:
    $choices = ['plain' => 'Plain (no CSS)', 'simple' => 'Simple'];
    foreach ($config['custom_themes'] ?? [] as $id => $theme) $choices[$id] = $theme['name'];
    $id = is_string($_GET['id'] ?? null) ? $_GET['id'] : 'simple';
    if (!isset($created) || $created === []) {
        if (isset($choices[theme_post('id')])) $id = theme_post('id');
    }
    if (!isset($choices[$id])) $id = 'simple';
    $custom = isset($config['custom_themes'][$id]);
    $values = $custom ? $config['custom_themes'][$id]['values'] : s64_theme_defaults($config);
    if ($id === 'simple') { $values['font'] = 'sans'; $values['content_width'] = '48'; }
    $name = $custom ? $choices[$id] : $choices[$id] . ' custom';
    if ($error !== '' || $preview !== null) {
        $name = theme_post('name') ?: $name;
        foreach (s64_theme_fields() as $key => $field) if (is_string($_POST[$key] ?? null)) $values[$key] = $_POST[$key];
    }
?>
<p>Active theme: <strong><?= theme_h($choices[$config['theme']] ?? (($config['theme'] ?? '') === 'c64' ? 'Simple' : 'Plain')) ?></strong>. Changes apply to this whole site. Built-in themes remain available.</p>
<form method="get" action="themes.php"><p><label>Theme <select name="id">
<?php foreach ($choices as $key => $label): ?><option value="<?= theme_h($key) ?>"<?= $key === $id ? ' selected' : '' ?>><?= theme_h($label) ?></option><?php endforeach; ?>
</select></label> <button type="submit">Open</button></p></form>
<form method="post" action="themes.php"><?= $csrf ?><input type="hidden" name="id" value="<?= theme_h($id) ?>"><input type="hidden" name="action" value="select"><p><button type="submit">Select this theme for the site</button></p></form>
<form method="post" action="themes.php"><?= $csrf ?><input type="hidden" name="id" value="<?= theme_h($id) ?>">
<fieldset><legend><strong>Theme</strong></legend>
<p><label>Theme name<br><input name="name" type="text" size="24" maxlength="80" value="<?= theme_h($name) ?>"></label></p>
<p>Start with the page layout and typography. Colours and detailed spacing are below. All controls work without JavaScript.</p>
</fieldset>
<?php
    $fields = s64_theme_fields();
    $choiceLabels = [
        'monospace' => 'Monospace', 'c64' => 'Commodore 64', 'sans' => 'Sans serif', 'serif' => 'Serif',
        'arial' => 'Arial', 'verdana' => 'Verdana', 'trebuchet' => 'Trebuchet MS', 'tahoma' => 'Tahoma',
        'georgia' => 'Georgia', 'times' => 'Times New Roman', 'courier' => 'Courier New',
        'yes' => 'Yes', 'no' => 'No', 'panel' => 'Boxed panel', 'open' => 'Open page',
        'inline' => 'Across the page', 'stacked' => 'One link per line',
    ];
?>
<?php foreach (s64_theme_field_groups() as $group => $keys): ?>
<fieldset><legend><strong><?= theme_h($group) ?></strong></legend>
<?php if ($group === 'Colours'): ?><p>Choose from the 16 Commodore 64 colours. Readability is checked before saving.</p><?php endif; ?>
<?php foreach ($keys as $key): $field = $fields[$key]; ?>
<p><label><?= theme_h($field[1]) ?><br>
<?php if ($field[0] === 'color'): ?><select name="<?= theme_h($key) ?>">
<?php foreach (s64_theme_palette() as $hex => $label): ?><option value="<?= $hex ?>"<?= s64_theme_nearest_colour((string)$values[$key]) === $hex ? ' selected' : '' ?>><?= theme_h($label) ?> (<?= $hex ?>)</option><?php endforeach; ?></select>
<?php elseif ($field[0] === 'choice'): ?><select name="<?= theme_h($key) ?>">
<?php foreach ($field[2] as $choice): ?><option value="<?= theme_h($choice) ?>"<?= $values[$key] === $choice ? ' selected' : '' ?>><?= theme_h($choiceLabels[$choice] ?? ucfirst($choice)) ?></option><?php endforeach; ?></select>
<?php else: ?><input type="text" size="<?= $field[0] === 'text' ? '28' : '12' ?>" name="<?= theme_h($key) ?>" value="<?= theme_h((string)$values[$key]) ?>"<?= $field[0] === 'text' ? ' maxlength="' . (int)$field[2] . '"' : '' ?>> <?= $field[0] === 'number' ? '(' . $field[2] . '–' . $field[3] . ')' : '' ?><?php endif; ?>
</label></p>
<?php endforeach; ?>
</fieldset>
<?php endforeach; ?>
<fieldset><legend><strong>Preview and save</strong></legend>
<p><label>Action <select name="action"><option value="preview">Preview without saving</option><option value="<?= $custom ? 'save' : 'create' ?>"><?= $custom ? 'Save changes' : 'Create custom theme' ?></option><option value="duplicate">Duplicate as a new theme</option></select></label> <input type="submit" value="Apply"></p>
<p>Preview does not change the live site. Built-in themes cannot be overwritten; editing one creates a custom theme.</p>
</fieldset></form>
<?php if ($custom): ?>
<form method="post" action="themes.php"><?= $csrf ?><input type="hidden" name="id" value="<?= theme_h($id) ?>"><input type="hidden" name="action" value="delete"><p><label><input type="checkbox" name="confirm_delete" value="yes"> Delete this custom theme permanently (active theme returns to Plain)</label> <button type="submit">Delete</button></p></form>
<?php endif; ?>
<form method="post" action="themes.php"><?= $csrf ?><input type="hidden" name="action" value="logout"><p><button type="submit">Log out</button></p></form>
<?php endif; ?>
<?php
$content = (string)ob_get_clean();
if ($preview !== null) {
    $sample = "**** COMMODORE 64 BASIC V2 ****\n64K RAM SYSTEM  38911 BASIC BYTES FREE\nREADY.\nABCDEFGHIJKLMNOPQRSTUVWXYZ\n0123456789 !? +-*/";
    $content = '<p><strong>Preview only — nothing saved.</strong></p><h2>Page heading</h2><p>This is ordinary page text with a <a href="index.php">sample link</a>.</p><h3>Screen sample</h3><pre>' . theme_h($sample)
        . '</pre><p><input aria-label="Sample input" value="Sample input" size="12"> <button type="button">Sample button</button></p>' . $content;
}
s64_render_layout($preview ?? $config, $documents->list('page', true), 'Themes', $content);
