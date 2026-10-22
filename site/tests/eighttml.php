<?php
declare(strict_types=1);
require_once dirname(__DIR__) . '/lib/Documents.php';
function check8(bool $ok, string $message): void { if (!$ok) throw new RuntimeException($message); }
function rejects8(callable $action, string $error): void {
    try { $action(); } catch (InvalidArgumentException | RuntimeException $e) {
        check8($e->getMessage() === $error, "expected $error; got " . $e->getMessage()); return;
    }
    throw new RuntimeException("expected rejection $error");
}
$root = sys_get_temp_dir() . '/see64you-8tml-' . bin2hex(random_bytes(5));
mkdir($root . '/pages', 0775, true); mkdir($root . '/posts', 0775, true);
$documents = new S64Documents($root);
$source = "---\n# Keep comment\n8TmL: 1\ntitle: Source title\nX-Future:  Keep : CASE  \n---\n<unknown>Literal</unknown>\n:::future\nopaque\ttext\n:::\n";
$input = ['type' => 'page', 'slug' => 'sample', 'status' => 'published', 'date' => '2026-10-21',
    'order' => '-12', 'raw8tml' => $source, 'title' => 'Ignored title', 'body' => 'Ignored body'];
try {
    $saved = $documents->save($input);
    check8($saved['raw8tml'] === $source && $saved['title'] === 'Source title', 'source and derived title');
    check8($saved['order'] === -12 && $saved['status'] === 'published' && $saved['date'] === '2026-10-21', 'service metadata');
    $path = $root . '/pages/sample.8tml.php'; $prior = file_get_contents($path);
    check8(str_starts_with($prior, '<?php http_response_code(404); exit; ?>' . "\n{"), 'guarded JSON record');
    check8($documents->get('page', 'sample') === $saved, 'exact reopen');
    check8(!isset($documents->list('page', true)[0]['raw8tml']), 'summary excludes source');
    check8(!isset($documents->list('page', true)[0]['body']), 'summary excludes body');
    $metadataInput = ['type' => 'post', 'slug' => 'metadata', 'status' => 'draft', 'raw8tml' => $source];
    $created = $documents->save($metadataInput);
    check8($created['date'] !== '' && $created['order'] === 0, 'new source gets service date/order defaults');
    $explicit = $documents->save($metadataInput + ['date' => '2001-02-03', 'order' => 12]);
    $preserved = $documents->save(array_replace($metadataInput, ['status' => 'published', 'raw8tml' => $source . "update\n"]));
    check8($preserved['date'] === '2001-02-03' && $preserved['order'] === 12 && $preserved['status'] === 'published', 'omitted date/order preserved on source update');
    $changedMetadata = $documents->save($metadataInput + ['date' => '2002-03-04', 'order' => -7]);
    check8($changedMetadata['date'] === '2002-03-04' && $changedMetadata['order'] === -7, 'explicit date/order still update');
    $documents->save(array_replace($metadataInput, ['slug' => 'earlier', 'date' => '2000-01-01']));
    file_put_contents($root . '/posts/excluded.s64.php', 'not a valid source record');
    $filtered = $documents->list('post', false, true);
    check8(array_column($filtered, 'slug') === ['metadata', 'earlier'], 'source-only list excludes old paths before parsing and retains sort order');
    check8($documents->list('post', true, true) === [], 'source-only published filtering');
    unlink($root . '/posts/excluded.s64.php');
    foreach (["", "---\n---\n", "---\ntitle: Wrong\n8tml: 1\n---\n", "---\n8tml: 2\n---\n",
        "---\n8tml: 1\n8TmL: 2\n---\n", "---\n8tml: 1\n", "---\n8tml: 1\n---\nbad\x80", "---\r\n8tml: 1\r\n---\r\n",
        "---\n8tml: 1\n---\n\0"] as $bad) {
        rejects8(fn() => $documents->save(array_replace($input, ['raw8tml' => $bad])), 'invalid_8tml');
        check8(file_get_contents($path) === $prior, 'invalid save preserves prior record');
    }
    $phpSource = "---\n8tml: 1\n---\n<?php echo 'should never execute'; ?>\n";
    $documents->save(array_replace($input, ['slug' => 'php-source', 'raw8tml' => $phpSource]));
    check8($documents->get('page', 'php-source')['raw8tml'] === $phpSource, 'PHP-like source roundtrips exactly');
    $protected = file_get_contents($root . '/pages/php-source.8tml.php');
    check8(substr_count($protected, '<?php') === 1 && str_contains($protected, '\u003C?php'), 'source PHP tags escaped in guarded record');
    $empty = "---\n8tml: 1\n---\n";
    $withoutTitle = $documents->save(array_replace($input, ['slug' => 'empty', 'raw8tml' => $empty]));
    check8($withoutTitle['title'] === 'empty' && $withoutTitle['body'] === '', 'empty body and fallback title');
    $longTitle = str_repeat('T', 120);
    $longSource = "---\n8tml: 1\ntitle: $longTitle\n---\n";
    $long = $documents->save(array_replace($input, ['slug' => 'long', 'raw8tml' => $longSource]));
    check8(strlen($long['title']) === 80 && $long['raw8tml'] === $longSource, 'display title bounded without source truncation');
    $full = $empty . str_repeat('x', S64EightTml::MAX_BYTES - strlen($empty));
    check8(strlen($documents->save(array_replace($input, ['slug' => 'full', 'raw8tml' => $full]))['raw8tml']) === 61440, 'capacity boundary');
    rejects8(fn() => $documents->save(array_replace($input, ['raw8tml' => $full . 'x'])), 'document_too_large');
    $legacy = ['type' => 'page', 'slug' => 'old', 'title' => 'Old', 'body' => 'Legacy'];
    $documents->save($legacy); $oldPath = $root . '/pages/old.s64.php'; $oldBytes = file_get_contents($oldPath);
    rejects8(fn() => $documents->save(array_replace($input, ['slug' => 'old'])), 'format_conflict');
    check8(file_get_contents($oldPath) === $oldBytes, 'old source untouched');
    rejects8(fn() => $documents->save(array_replace($legacy, ['slug' => 'sample'])), 'format_conflict');
    check8(file_get_contents($path) === $prior, 'new source untouched by old save');
    file_put_contents($path, "corrupt");
    rejects8(fn() => $documents->get('page', 'sample'), 'invalid_record');
    file_put_contents($path, '<?php http_response_code(404); exit; ?>' . "\n{}");
    rejects8(fn() => $documents->get('page', 'sample'), 'invalid_record');
    file_put_contents($path, $prior);
    mkdir($root . '/pages/blocked.8tml.php');
    set_error_handler(static fn() => true);
    try { rejects8(fn() => $documents->save(array_replace($input, ['slug' => 'blocked'])), 'write_failed'); }
    finally { restore_error_handler(); }
    check8(glob($root . '/pages/blocked.8tml.php.tmp.*') === [], 'rename failure removes temporary file');
    rmdir($root . '/pages/blocked.8tml.php');
    /* Hold the production lock, then start a competing new-format writer. Its
     * conflict check must run after the prior owner commits and releases it. */
    $lockPath = $root . '/pages/.documents.lock';
    $lock = fopen($lockPath, 'c');
    check8($lock !== false && flock($lock, LOCK_EX), 'hold directory write lock');
    $childCode = 'require $argv[1]; file_put_contents($argv[3], "ready"); '
        . 'try {(new S64Documents($argv[2]))->save(["type"=>"page","slug"=>"race","raw8tml"=>"---\n8tml: 1\n---\n"]); echo "unexpected";} '
        . 'catch (InvalidArgumentException $e) {echo $e->getMessage();}';
    $marker = $root . '/child-ready';
    $child = proc_open([PHP_BINARY, '-r', $childCode, dirname(__DIR__) . '/lib/Documents.php', $root, $marker],
        [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']], $pipes);
    check8(is_resource($child), 'start competing writer');
    fclose($pipes[0]);
    try {
        $deadline = microtime(true) + 5;
        while (!is_file($marker) && microtime(true) < $deadline) usleep(10000);
        usleep(100000);
        check8(is_file($marker) && proc_get_status($child)['running'], 'competing writer waits');
        check8(!is_file($root . '/pages/race.8tml.php'), 'blocked writer cannot create source');
        $racePath = $root . '/pages/race.s64.php';
        file_put_contents($racePath, $oldBytes);
    } finally {
        flock($lock, LOCK_UN); fclose($lock);
    }
    $childOutput = stream_get_contents($pipes[1]); $childError = stream_get_contents($pipes[2]);
    fclose($pipes[1]); fclose($pipes[2]);
    check8(proc_close($child) === 0 && $childOutput === 'format_conflict', 'serialized writer detects prior format: ' . $childError);
    check8(file_get_contents($racePath) === $oldBytes && !is_file($root . '/pages/race.8tml.php'), 'competing save preserves prior source');
    unlink($marker);
    /* A waiting source update must not restore metadata from a record deleted
     * by the previous lock owner. With no record left it creates fresh defaults. */
    $childCode = 'require $argv[1]; file_put_contents($argv[3], "ready"); '
        . '$saved=(new S64Documents($argv[2]))->save(["type"=>"post","slug"=>"metadata","status"=>"draft","raw8tml"=>"---\n8tml: 1\n---\n"]); echo json_encode($saved);';
    $postLock = fopen($root . '/posts/.documents.lock', 'c');
    check8($postLock !== false && flock($postLock, LOCK_EX), 'hold post write lock');
    $child = proc_open([PHP_BINARY, '-r', $childCode, dirname(__DIR__) . '/lib/Documents.php', $root, $marker],
        [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']], $pipes);
    check8(is_resource($child), 'start source writer before deletion');
    fclose($pipes[0]);
    try {
        $deadline = microtime(true) + 5;
        while (!is_file($marker) && microtime(true) < $deadline) usleep(10000);
        usleep(100000);
        check8(is_file($marker) && proc_get_status($child)['running'], 'source writer waits for metadata lock');
        unlink($root . '/posts/metadata.8tml.php');
    } finally {
        flock($postLock, LOCK_UN); fclose($postLock);
    }
    $childOutput = stream_get_contents($pipes[1]); $childError = stream_get_contents($pipes[2]);
    fclose($pipes[1]); fclose($pipes[2]);
    check8(proc_close($child) === 0, 'source writer after deletion succeeds: ' . $childError);
    $recreated = json_decode($childOutput, true, flags: JSON_THROW_ON_ERROR);
    check8($recreated['order'] === 0 && $recreated['date'] !== '2002-03-04', 'deleted source metadata is not resurrected');
    unlink($marker);
    $publication = array_replace($input, ['slug' => 'publication']);
    $documents->publish($publication, 'create', '');
    $publicationPath = $root . '/pages/publication.8tml.php';
    $publicationBytes = file_get_contents($publicationPath);
    rejects8(fn() => $documents->publish($publication, 'create', ''), 'already_exists');
    rejects8(fn() => $documents->publish($publication, 'update', ''), 'identity_required');
    rejects8(fn() => $documents->publish($publication, 'update', 'different'), 'identity_required');
    rejects8(fn() => $documents->publish($publication, 'upsert', 'publication'), 'invalid_intent');
    check8(file_get_contents($publicationPath) === $publicationBytes, 'rejected publication preserves exact bytes');
    $documents->publish(array_replace($publication, ['status' => 'draft']), 'update', 'publication');
    check8($documents->get('page', 'publication')['status'] === 'draft', 'explicit identity update');
    $documents->delete('page', 'publication');
    rejects8(fn() => $documents->publish($publication, 'update', 'publication'), 'not_found');
    check8(!is_file($publicationPath), 'missing update never recreates deleted content');
    /* Both creators start before either can acquire the lock. Exactly one wins. */
    $lock = fopen($lockPath, 'c');
    check8($lock !== false && flock($lock, LOCK_EX), 'hold publication lock');
    $workers = [];
    $childCode = 'require $argv[1]; file_put_contents($argv[3], "ready"); '
        . 'try {(new S64Documents($argv[2]))->publish(["type"=>"page","slug"=>"concurrent","raw8tml"=>"---\\n8tml: 1\\n---\\n"], "create", ""); echo "created";} '
        . 'catch (InvalidArgumentException $e) {echo $e->getMessage();}';
    try {
        for ($i = 0; $i < 2; ++$i) {
            $ready = $root . '/ready-' . $i;
            $process = proc_open([PHP_BINARY, '-r', $childCode, dirname(__DIR__) . '/lib/Documents.php', $root, $ready],
                [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']], $channels);
            check8(is_resource($process), 'start concurrent publication');
            fclose($channels[0]);
            $workers[] = [$process, $channels, $ready];
        }
        $deadline = microtime(true) + 5;
        while ((!is_file($workers[0][2]) || !is_file($workers[1][2])) && microtime(true) < $deadline) usleep(10000);
        check8(is_file($workers[0][2]) && is_file($workers[1][2]), 'both creators started');
        check8(!is_file($root . '/pages/concurrent.8tml.php'), 'creators wait for lock');
    } finally { flock($lock, LOCK_UN); fclose($lock); }
    $outcomes = [];
    foreach ($workers as [$process, $channels, $ready]) {
        $outcomes[] = stream_get_contents($channels[1]);
        $error = stream_get_contents($channels[2]);
        fclose($channels[1]); fclose($channels[2]);
        check8(proc_close($process) === 0, 'concurrent publication completed: ' . $error);
        unlink($ready);
    }
    sort($outcomes);
    check8($outcomes === ['already_exists', 'created'], 'exactly one simultaneous creator succeeds');
    unlink($lockPath); mkdir($lockPath);
    set_error_handler(static fn() => true);
    try {
        rejects8(fn() => $documents->save($input), 'storage_unavailable');
        rejects8(fn() => $documents->delete('page', 'sample'), 'storage_unavailable');
    } finally { restore_error_handler(); }
    check8(file_get_contents($path) === $prior, 'lock-open failure preserves source');
    check8(glob($root . '/pages/sample.8tml.php.tmp.*') === [], 'lock failure creates no source temp');
    rmdir($lockPath);
    check8($documents->delete('page', 'sample') && $documents->get('page', 'sample') === null, 'new record deletes');
    check8(file_get_contents($oldPath) === $oldBytes, 'delete preserves old records');
    echo "8TML service tests passed\n";
} finally {
    foreach (glob($root . '/*/*') ?: [] as $file) { if (is_dir($file)) rmdir($file); else unlink($file); }
    foreach (['pages', 'posts'] as $directory) {
        $lockPath = $root . '/' . $directory . '/.documents.lock';
        if (is_file($lockPath)) unlink($lockPath);
        elseif (is_dir($lockPath)) rmdir($lockPath);
    }
    if (is_file($root . '/child-ready')) unlink($root . '/child-ready');
    rmdir($root . '/pages'); rmdir($root . '/posts'); rmdir($root);
}
