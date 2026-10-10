<?php
declare(strict_types=1);

// This guide records no credentials and changes no site or C64U state.
$step = filter_input(INPUT_GET, 'step', FILTER_VALIDATE_INT);
$step = is_int($step) && $step >= 1 && $step <= 6 ? $step : 1;
$kind = ($_GET['kind'] ?? '') === 'post' ? 'post' : 'page';
$titles = [1 => 'Prepare your C64U', 'Create your website', 'Pair and save settings',
    'Write and save your source', 'Publish your first ' . $kind, 'Check your published ' . $kind];
?><!doctype html>
<html lang="en">
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>First page or post - See64You</title></head>
<body>
<h1>Publish your first page or post</h1>
<p>Step <?= $step ?> of 6: <?= $titles[$step] ?></p>
<p>Keep this guide open while using your C64U. Next means you have completed the step yourself; it does not test your machine. You can go back or resume any step.</p>
<form method="get">
<input type="hidden" name="step" value="<?= $step ?>">
<label>I want to publish <select name="kind"><option value="page"<?= $kind === 'page' ? ' selected' : '' ?>>a page (website navigation)</option><option value="post"<?= $kind === 'post' ? ' selected' : '' ?>>a post (blog archive)</option></select></label>
<button type="submit">Choose</button>
</form>
<?php if ($step === 1): ?>
<h2>Prepare your C64U</h2>
<ol>
<li>Back up an existing installation, configuration and source files. Copy every file from the independent client package into <code>/sd/SEE64YOU/</code>, keeping matching assets together.</li>
<li>In the Ultimate menu, enable the Ultimate Command Interface. Under C64 and Cartridge Settings, set RAM Expansion Unit to Enabled and REU Size to 16 MB.</li>
<li>Connect the C64U to your network. The website must be reachable from the C64U; localhost refers to the machine opening it, so use a reachable hostname or computer IP address.</li>
<li>Launch <code>SEE64YOU.PRG</code> for Editor, <code>S64CFG.PRG</code> for Settings and <code>S64PUB.PRG</code> for Publisher separately. Other PRGs/BIN files are internal assets. Each program returns to BASIC.</li>
</ol>
<p>You can write and save offline, then complete publishing setup later. If the Editor prerequisite screen appears, follow its instruction and press R to recheck.</p>
<?php elseif ($step === 2): ?>
<h2>Create your website</h2>
<p>Upload the standalone site package to PHP 8.1+ hosting. For a fresh installation only, enable setup with an empty <code>ENABLE_SETUP</code> file and open <a href="setup.php">Website setup</a>. Choose a site title, theme and a publishing password of at least eight characters. Setup removes the marker when successful.</p>
<p>If this website is already configured, keep its configuration and skip creating it again. Open <a href="index.php">your site</a> to check it works. Starter pages/posts may already be present.</p>
<p>Keep your publishing password for browser administration. C64U Settings uses a separate publishing key.</p>
<?php elseif ($step === 3): ?>
<h2>Pair and save settings</h2>
<ol>
<li>Open this website's <a href="pair.php">Pair C64U</a> page over HTTPS. Enter the browser publishing password to reveal your dedicated key. Pairing over HTTP is refused.</li>
<li>Save only the 64 hexadecimal key characters in <code>/sd/SEE64YOU/SEE64KEY.TXT</code>, or prepare to type them on the C64U. Keep the key private and remove the import file when finished.</li>
<li>Launch <code>S64CFG.PRG</code>. The setup wizard appears for missing site/key settings; press W from Basic Settings to run it again. Step 1: press 1, enter the site folder URL (without <code>api.php</code>), Return, then N. Step 2: press 1 to import the key or 2 to type it, then N.</li>
<li>Firmware 1.1 direct publishing uses an explicit <code>http://</code> site URL and transmits document text unencrypted. At step 3, press 1 and Y only if you consent. HTTPS URLs are never silently downgraded. For an HTTPS-required destination, use the separately configured relay; Advanced Settings remains available.</li>
<li>Press S. Continue only after “SETTINGS SAVED AND VERIFIED”, then Return to BASIC. Reopen Settings to check the site and configured-key indicator; STOP cancels without saving. Connection authentication happens in Publisher.</li>
</ol>
<p>STOP in the wizard cancels changes and returns to BASIC. You can edit offline and set up later.</p>
<?php elseif ($step === 4): ?>
<h2>Write and save your source</h2>
<ol>
<li>Launch <code>SEE64YOU.PRG</code>. Choose a new document (Ctrl+N), resolving any unsaved/recovered content first.</li>
<li>Type a short first <?= $kind ?>. Press Ctrl+S, enter a new name such as <code>FIRST.S64</code>, and Return. Wait for saving to finish. Local source files live at the SD root. The first filename supplies the initial title and slug.</li>
<li>Press RUN/STOP to return to BASIC. If prompted, S saves, D discards, STOP cancels exit. Open the saved file with Ctrl+O if you want to check it before publishing.</li>
</ol>
<p>The <code>.s64</code> file is your editable original. Keep it backed up. No live document transfers between Editor and Publisher.</p>
<?php elseif ($step === 5): ?>
<h2>Publish your first <?= $kind ?></h2>
<ol>
<li>Launch <code>S64PUB.PRG</code> fresh. Press L, type <code>FIRST.S64</code>, Return. Wait for Loading to finish and check the displayed title and slug.</li>
<li>Press 2. Wait for Connecting and a successful connection result. On failure, check Settings, the reachable site URL and key before trying again.</li>
<li>Check Type. Press T only if needed to select <strong><?= $kind ?></strong>. Check State. Press 4 only if needed to select <strong>published</strong>; drafts do not appear publicly.</li>
<li>Press 3 once. Wait for Publishing and “Document published”. Do not keep pressing keys while the operation is working. On an error, retain your local source and correct the reported problem.</li>
<li>Press RUN/STOP to return to BASIC.</li>
</ol>
<p>Changing type/state in Publisher does not rewrite the local source file. Check them again when loading that original later.</p>
<?php else: ?>
<h2>Check your published <?= $kind ?></h2>
<p>Open <a href="<?= $kind === 'post' ? 'posts.php' : 'index.php' ?>"><?= $kind === 'post' ? 'the post archive' : 'the website' ?></a>, follow your document's link and confirm the text matches. “Document published” plus the visible matching page completes this walkthrough. Next/step navigation alone is not proof of publication.</p>
<p>To edit later: fresh Publisher, 2 connect, <?= $kind === 'post' ? '7' : '6' ?> browse <?= $kind ?>s, Space for Next, O to open and Y if replacement is requested. Wait for “Remote document opened”. Then D, enter a <strong>new</strong> filename such as <code>FIRST2.S64</code>, and wait for “Saved .S64; reopen in Editor”. Existing download files are refused.</p>
<p>Exit to BASIC, open that download in Editor, edit/save, then launch Publisher and load it to update the same remote document. Keep its title, slug and type if you want an update rather than a different document.</p>
<p>If your page is missing, check that Publisher's State was published and that you are looking at the configured website, then check the displayed slug. Do not delete your source or recovery files to retry.</p>
<?php endif; ?>
<nav aria-label="Wizard steps">
<?php if ($step > 1): ?><a href="start.php?step=<?= $step - 1 ?>&amp;kind=<?= $kind ?>">Back</a><?php endif; ?>
<?php if ($step < 6): ?> <a href="start.php?step=<?= $step + 1 ?>&amp;kind=<?= $kind ?>">I completed this step — Next</a><?php endif; ?>
<p><?php foreach ($titles as $number => $title): ?><a href="start.php?step=<?= $number ?>&amp;kind=<?= $kind ?>"><?= $number ?>. <?= $title ?></a><br><?php endforeach; ?></p>
</nav>
<p><a href="index.php">Return to the website</a></p>
</body>
</html>
