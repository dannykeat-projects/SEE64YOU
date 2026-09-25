<?php
declare(strict_types=1);

require_once dirname(__DIR__) . '/lib/Documents.php';
require_once dirname(__DIR__) . '/lib/Auth.php';
require_once dirname(__DIR__) . '/lib/Settings.php';
require_once dirname(__DIR__) . '/lib/Wire.php';
require_once dirname(__DIR__) . '/lib/Render.php';

function check(bool $condition, string $message): void
{
    if (!$condition) {
        fwrite(STDERR, "FAIL: $message\n");
        exit(1);
    }
}

$root = sys_get_temp_dir() . '/see64you-test-' . bin2hex(random_bytes(4));
mkdir($root . '/pages', 0775, true);
mkdir($root . '/posts', 0775, true);

try {
    $documents = new S64Documents($root);

    $saved = $documents->save([
        'type' => 'page',
        'slug' => 'hello-world',
        'title' => 'Hello World',
        'status' => 'draft',
        'date' => '2026-09-23T19:00:00+10:00',
        'order' => 10,
        'body' => "Hello **world**\n<script>alert(1)</script>",
    ]);

    check($saved['slug'] === 'hello-world', 'saved slug');
    check(is_file($root . '/pages/hello-world.s64.php'), 'guarded file created');

    $raw = (string)file_get_contents($root . '/pages/hello-world.s64.php');
    check(str_starts_with($raw, '<?php http_response_code(404); exit; ?>'), 'document has direct-access guard');

    $loaded = $documents->get('page', 'hello-world');
    check($loaded !== null, 'saved document loads');
    check($loaded['body'] === "Hello **world**\n<script>alert(1)</script>", 'body round trips exactly');

    check(count($documents->list('page', false)) === 1, 'draft appears in authenticated list');
    check(count($documents->list('page', true)) === 0, 'draft hidden publicly');

    $documents->save(array_replace($saved, ['status' => 'published']));
    check(count($documents->list('page', true)) === 1, 'published page appears publicly');

    $rendered = s64_markup((string)$loaded['body']);
    check(str_contains($rendered, '<strong>world</strong>'), 'bold markup renders');
    check(!str_contains($rendered, '<script>'), 'raw HTML is escaped');
    check(str_contains($rendered, '&lt;script&gt;'), 'escaped HTML remains visible');

    $formatRendered = s64_markup(
        "# Heading one\n"
        . "## Heading two\n"
        . "- list **bold**\n"
        . "- list *italic*\n"
        . "[OpenAI](https://openai.com)"
    );
    check(str_contains($formatRendered, '<h1>Heading one</h1>'), 'heading one renders');
    check(str_contains($formatRendered, '<h2>Heading two</h2>'), 'heading two renders');
    check(str_contains($formatRendered, '<li>list <strong>bold</strong></li>'), 'bold list item renders');
    check(str_contains($formatRendered, '<li>list <em>italic</em></li>'), 'italic list item renders');
    check(
        str_contains($formatRendered, '<a href="https://openai.com">OpenAI</a>'),
        'link markup renders'
    );
    check(
        substr_count($formatRendered, '<ul>') === 1
            && substr_count($formatRendered, '</ul>') === 1,
        'formatting subset renders consistently'
    );

    $unsupportedSource = "### Heading three\n"
        . "`inline code`\n"
        . "~~strikethrough~~\n"
        . "[FTP link](ftp://example.com/file)\n"
        . "**unterminated";
    $unsupportedRendered = s64_markup($unsupportedSource);
    check(
        str_contains($unsupportedRendered, '<p>### Heading three</p>'),
        'unsupported heading level remains text'
    );
    check(
        str_contains($unsupportedRendered, '<p>`inline code`</p>'),
        'unsupported code markup remains text'
    );
    check(
        str_contains($unsupportedRendered, '<p>~~strikethrough~~</p>'),
        'unsupported strikethrough remains text'
    );
    check(
        str_contains($unsupportedRendered, '[FTP link](ftp://example.com/file)'),
        'unsupported link scheme remains source text'
    );
    check(
        str_contains($unsupportedRendered, '<p>**unterminated</p>'),
        'incomplete supported markup remains text'
    );

    $wire = S64Wire::serialize($loaded);
    $wireRoundTrip = S64Wire::parse($wire);
    check($wireRoundTrip['slug'] === 'hello-world', 'wire slug round trips');
    check($wireRoundTrip['title'] === 'Hello World', 'wire title round trips');
    check($wireRoundTrip['body'] === $loaded['body'], 'wire body round trips exactly');

    $password = 'test-password';
    $auth = new S64Auth([
        'password_hash' => password_hash($password, PASSWORD_DEFAULT),
        'token_secret' => str_repeat('a', 64),
        'token_ttl' => 600,
    ]);

    check($auth->configured(), 'auth configured');
    $session = $auth->login($password);
    check($auth->valid($session['token']), 'issued token validates');
    check(!$auth->valid($session['token'] . 'x'), 'tampered token rejected');

    $unconfigured = new S64Auth([
        'password_hash' => '',
        'token_secret' => '',
        'token_ttl' => 600,
    ]);
    $forgedExpiry = time() + 600;
    $forgedNonce = str_repeat('a', 16);
    $forgedPayload = 'v1|' . $forgedExpiry . '|' . $forgedNonce;
    $forgedSignature = hash_hmac('sha256', $forgedPayload, '');
    $forgedToken = 'v1.' . $forgedExpiry . '.' . $forgedNonce . '.' . $forgedSignature;
    check(!$unconfigured->valid($forgedToken), 'unconfigured auth rejects empty-secret forged token');

    $settingsConfig = [
        'site_title' => 'Old title',
        'theme' => 'plain',
        'text_color' => '#111111',
        'background_color' => '#ffffff',
        'link_color' => '#0000ee',
        'visited_color' => '#551a8b',
        'password_hash' => 'keep-this-hash',
        'token_secret' => str_repeat('b', 64),
        'token_ttl' => 600,
    ];
    $configPath = $root . '/config.php';
    $settings = new S64Settings($configPath, $settingsConfig);
    $visible = $settings->update([
        'site_title' => 'New title',
        'theme' => 'c64',
        'text_color' => '#abcdef',
        'background_color' => '#123456',
    ]);
    check($visible['site_title'] === 'New title', 'settings title updates');
    check($visible['theme'] === 'c64', 'settings theme updates');
    $writtenConfig = require $configPath;
    check($writtenConfig['password_hash'] === 'keep-this-hash', 'settings preserve password hash');
    check($writtenConfig['token_secret'] === str_repeat('b', 64), 'settings preserve token secret');

    check($documents->delete('page', 'hello-world'), 'document deletes');
    check($documents->get('page', 'hello-world') === null, 'deleted document is gone');

    $invalidRejected = false;
    try {
        $documents->get('page', '../secret');
    } catch (InvalidArgumentException) {
        $invalidRejected = true;
    }
    check($invalidRejected, 'path traversal slug rejected');

    $metadataRejected = false;
    try {
        $documents->save([
            'type' => 'post',
            'slug' => str_repeat('a', 65),
            'title' => 'Too long slug',
            'status' => 'draft',
            'body' => '',
        ]);
    } catch (InvalidArgumentException $exception) {
        $metadataRejected = $exception->getMessage() === 'invalid_slug';
    }
    check($metadataRejected, 'slug longer than C64 metadata capacity rejected');

    $metadataRejected = false;
    try {
        $documents->save([
            'type' => 'post',
            'slug' => 'long-title',
            'title' => str_repeat('T', 81),
            'status' => 'draft',
            'body' => '',
        ]);
    } catch (InvalidArgumentException $exception) {
        $metadataRejected = $exception->getMessage() === 'invalid_title';
    }
    check($metadataRejected, 'title longer than C64 metadata capacity rejected');

    $metadataRejected = false;
    try {
        $documents->save([
            'type' => 'post',
            'slug' => 'long-date',
            'title' => 'Long date',
            'status' => 'draft',
            'date' => str_repeat('2', 41),
            'body' => '',
        ]);
    } catch (InvalidArgumentException $exception) {
        $metadataRejected = $exception->getMessage() === 'invalid_date';
    }
    check($metadataRejected, 'date longer than C64 metadata capacity rejected');

    foreach ([2147483648, -2147483649, '1.5'] as $badOrder) {
        $orderRejected = false;
        try {
            $documents->save([
                'type' => 'post',
                'slug' => 'bad-order',
                'title' => 'Bad order',
                'status' => 'draft',
                'order' => $badOrder,
                'body' => '',
            ]);
        } catch (InvalidArgumentException $exception) {
            $orderRejected = $exception->getMessage() === 'invalid_order';
        }
        check($orderRejected, 'out-of-range/invalid order rejected');
    }

    $largeRejected = false;
    try {
        $documents->save([
            'type' => 'post',
            'slug' => 'too-large',
            'title' => 'Too large',
            'status' => 'draft',
            'body' => str_repeat('x', 8193),
        ]);
    } catch (InvalidArgumentException $exception) {
        $largeRejected = $exception->getMessage() === 'document_too_large';
    }
    check($largeRejected, 'body beyond C64 buffer rejected');

    $duplicateWireRejected = false;
    try {
        S64Wire::parse(str_replace(
            "Type: page\n",
            "Type: page\nType: post\n",
            $wire
        ));
    } catch (InvalidArgumentException $exception) {
        $duplicateWireRejected = $exception->getMessage() === 'invalid_wire_document';
    }
    check($duplicateWireRejected, 'duplicate wire header rejected');

    $wireOrderRejected = false;
    try {
        S64Wire::parse(str_replace("Order: 10", "Order: 2147483648", $wire));
    } catch (InvalidArgumentException $exception) {
        $wireOrderRejected = $exception->getMessage() === 'invalid_wire_document';
    }
    check($wireOrderRejected, 'wire order beyond int32 rejected');

    echo "site tests passed\n";
} finally {
    $iterator = new RecursiveIteratorIterator(
        new RecursiveDirectoryIterator($root, FilesystemIterator::SKIP_DOTS),
        RecursiveIteratorIterator::CHILD_FIRST
    );

    foreach ($iterator as $item) {
        $item->isDir() ? rmdir($item->getPathname()) : unlink($item->getPathname());
    }
    @rmdir($root);
}
