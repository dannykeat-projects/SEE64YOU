<?php
declare(strict_types=1);

require_once dirname(__DIR__) . '/lib/Documents.php';
require_once dirname(__DIR__) . '/lib/Auth.php';
require_once dirname(__DIR__) . '/lib/DirectAuth.php';
require_once dirname(__DIR__) . '/lib/Settings.php';
require_once dirname(__DIR__) . '/lib/Wire.php';
require_once dirname(__DIR__) . '/lib/Render.php';
require_once dirname(__DIR__) . '/lib/PostArchive.php';

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

    $archiveFixture = [];
    for ($i = 0; $i < 12; ++$i) {
        $archiveFixture[] = [
            'date' => ($i < 7 ? '2026' : '2025') . '-01-01T00:00:00+00:00',
            'slug' => 'post-' . $i,
            'title' => 'Post ' . $i,
        ];
    }
    check(S64PostArchive::years($archiveFixture) === ['2026', '2025'], 'archive years preserve newest order');
    check(count(S64PostArchive::filterByYear($archiveFixture, '2025')) === 5, 'archive filters by year');
    $firstArchivePage = S64PostArchive::paginate($archiveFixture, 1);
    $secondArchivePage = S64PostArchive::paginate($archiveFixture, 2);
    check(count($firstArchivePage['items']) === 10, 'archive first page uses fixed page size');
    check(count($secondArchivePage['items']) === 2, 'archive second page contains remainder');
    check($firstArchivePage['pages'] === 2 && $secondArchivePage['valid'], 'archive page count is correct');
    check(!S64PostArchive::paginate($archiveFixture, 3)['valid'], 'archive rejects out-of-range page');
    check(S64PostArchive::parseYear('2026') === '2026', 'archive accepts four-digit year');
    check(S64PostArchive::parseYear('20x6') === null, 'archive rejects invalid year');
    check(S64PostArchive::parsePage('2') === 2, 'archive accepts positive page');
    check(S64PostArchive::parsePage('0') === 0, 'archive rejects zero page');

    check(S64PostArchive::parsePage('9999999999') === 0, 'archive rejects oversized page input');

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

    $markupV1Source = "# Heading one\n"
        . "## Heading two\n"
        . "- list **bold**\n"
        . "- list *italic*\n"
        . "[OpenAI](https://openai.com)\n"
        . "### literal heading\n"
        . "`literal code`\n"
        . "~~literal strike~~";
    $markupV1Document = [
        'type' => 'page',
        'slug' => 'markup-v1',
        'title' => 'Markup v1',
        'status' => 'draft',
        'date' => '',
        'order' => 0,
        'body' => $markupV1Source,
    ];
    $markupV1Wire = S64Wire::serialize($markupV1Document);
    $markupV1RoundTrip = S64Wire::parse($markupV1Wire);
    check(
        $markupV1RoundTrip['body'] === $markupV1Source,
        'Markup v1 source round trips exactly through wire format'
    );
    $listSummaryWire = S64Wire::serializeListSummary([
        ['slug' => 'first-page', 'title' => 'First Page'],
        ['slug' => 'second-page', 'title' => 'Second Page'],
    ], 1);
    check(
        $listSummaryWire === "SEE64YOU-LIST/1\nCount: 2\nSlug: second-page\nTitle: Second Page\n\n",
        'compact list summary wire is exact'
    );
    check(
        S64Wire::serializeListSummary([
            ['slug' => 'first-page', 'title' => 'First Page'],
        ], 1) === "SEE64YOU-LIST/1\nCount: 1\n\n",
        'compact list summary wire marks end without document payload'
    );

    $markupV1Rendered = s64_markup($markupV1RoundTrip['body']);
    check(str_contains($markupV1Rendered, '<h1>Heading one</h1>'), 'Markup v1 h1 renders');
    check(str_contains($markupV1Rendered, '<h2>Heading two</h2>'), 'Markup v1 h2 renders');
    check(str_contains($markupV1Rendered, '<li>list <strong>bold</strong></li>'), 'Markup v1 bold list renders');
    check(str_contains($markupV1Rendered, '<li>list <em>italic</em></li>'), 'Markup v1 italic list renders');
    check(
        str_contains($markupV1Rendered, '<a href="https://openai.com">OpenAI</a>'),
        'Markup v1 HTTPS link renders'
    );
    check(
        str_contains($markupV1Rendered, '<p>### literal heading</p>')
            && str_contains($markupV1Rendered, '<p>`literal code`</p>')
            && str_contains($markupV1Rendered, '<p>~~literal strike~~</p>'),
        'Markup v1 unsupported constructs remain literal'
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
    check($auth->passwordValid($password), 'publishing password can authorize pairing');
    check(!$auth->passwordValid('wrong-password'), 'pairing rejects wrong publishing password');

    $directSecret = str_repeat('b', 64);
    $directAuth = new S64DirectAuth(
        ['direct_secret' => $directSecret],
        $root . '/direct-auth'
    );
    check($directAuth->configured(), 'direct C64U auth configured');

    $directChallenge = $directAuth->challenge();
    $directBody = "type=page&slug=hello-world";
    $directUri = '/api.php?action=save&ack=1';
    $directSignature = S64DirectAuth::signature(
        $directSecret,
        (string)$directChallenge['challenge'],
        'POST',
        $directUri,
        $directBody
    );
    check(
        $directAuth->verify(
            (string)$directChallenge['challenge'],
            $directSignature,
            'POST',
            $directUri,
            $directBody
        ),
        'direct C64U request signature validates'
    );
    check(
        !$directAuth->verify(
            (string)$directChallenge['challenge'],
            $directSignature,
            'POST',
            $directUri,
            $directBody
        ),
        'direct C64U challenge cannot be replayed'
    );

    $tamperChallenge = $directAuth->challenge();
    $tamperSignature = S64DirectAuth::signature(
        $directSecret,
        (string)$tamperChallenge['challenge'],
        'POST',
        $directUri,
        $directBody
    );
    check(
        !$directAuth->verify(
            (string)$tamperChallenge['challenge'],
            $tamperSignature,
            'POST',
            $directUri,
            $directBody . 'x'
        ),
        'direct C64U auth rejects modified body'
    );
    check(
        $directAuth->verify(
            (string)$tamperChallenge['challenge'],
            $tamperSignature,
            'POST',
            $directUri,
            $directBody
        ),
        'invalid signature does not consume direct challenge'
    );

    $preview = $auth->previewToken('page', 'hello-world');
    check(
        is_int($preview['expires'] ?? null) && $preview['expires'] > time(),
        'preview token includes a future expiry'
    );
    check(
        $auth->validPreviewToken((string)$preview['token'], 'page', 'hello-world'),
        'preview token validates for its document'
    );
    check(
        !$auth->validPreviewToken((string)$preview['token'], 'post', 'hello-world'),
        'preview token is bound to document type'
    );
    check(
        !$auth->validPreviewToken((string)$preview['token'], 'page', 'another-page'),
        'preview token is bound to document slug'
    );
    check(
        !$auth->validPreviewToken((string)$preview['token'] . 'x', 'page', 'hello-world'),
        'tampered preview token is rejected'
    );

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
        'direct_secret' => str_repeat('c', 64),
        'token_ttl' => 600,
    ];
    $configPath = $root . '/config.php';
    $settings = new S64Settings($configPath, $settingsConfig);
    $visible = $settings->update([
        'site_title' => 'New title',
        'theme' => 'simple',
        'text_color' => '#ffffff',
        'background_color' => '#000000',
        'link_color' => '#00ffff',
        'visited_color' => '#ffccff',
    ]);
    check($visible['site_title'] === 'New title', 'settings title updates');
    check($visible['theme'] === 'simple', 'settings theme updates');
    check($visible['text_color'] === '#ffffff', 'accessible text colour updates');
    $writtenConfig = require $configPath;
    check($writtenConfig['password_hash'] === 'keep-this-hash', 'settings preserve password hash');
    check($writtenConfig['token_secret'] === str_repeat('b', 64), 'settings preserve token secret');
    check($writtenConfig['direct_secret'] === str_repeat('c', 64), 'settings preserve direct C64U secret');

    $contrastRejected = false;
    try {
        $settings->update([
            'theme' => 'simple',
            'text_color' => '#777777',
            'background_color' => '#777777',
            'link_color' => '#777777',
            'visited_color' => '#777777',
        ]);
    } catch (InvalidArgumentException $exception) {
        $contrastRejected = $exception->getMessage() === 'insufficient_contrast';
    }
    check($contrastRejected, 'styled themes reject low-contrast colours');

    $simpleCss = s64_theme_css([
        'theme' => 'simple',
        'text_color' => '#b0b0ff',
        'background_color' => '#3e31a2',
        'link_color' => '#ffffff',
        'visited_color' => '#e49aff',
    ]);
    check(str_contains($simpleCss, 'line-height:1.5'), 'Simple theme uses readable line height');
    check(str_contains($simpleCss, 'focus-visible'), 'styled themes expose keyboard focus');

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

require __DIR__ . '/themes.php';
