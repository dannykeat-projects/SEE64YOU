<?php
declare(strict_types=1);

require_once dirname(__DIR__) . '/see64you/lib/protocol.php';

function check(bool $condition, string $message): void
{
    if (!$condition) {
        fwrite(STDERR, "FAIL: $message\n");
        exit(1);
    }
}

$wire = "SEE64YOU/1\n"
    . "Type: post\n"
    . "Slug: hello-world\n"
    . "Title: Hello World\n"
    . "Status: published\n"
    . "Date: 2026-09-24T08:00:00+10:00\n"
    . "Order: -3\n\n"
    . "Body line one.\nBody line two.";

$parsed = see64you_wire_parse($wire);
check($parsed['type'] === 'post', 'wire type');
check($parsed['slug'] === 'hello-world', 'wire slug');
check($parsed['title'] === 'Hello World', 'wire title');
check($parsed['status'] === 'published', 'wire status');
check($parsed['order'] === -3, 'wire order');
check($parsed['body'] === "Body line one.\nBody line two.", 'wire body exact');

$duplicateRejected = false;
try {
    see64you_wire_parse(str_replace(
        "Type: post\n",
        "Type: post\nType: page\n",
        $wire
    ));
} catch (InvalidArgumentException $exception) {
    $duplicateRejected = $exception->getMessage() === 'invalid_wire_document';
}
check($duplicateRejected, 'duplicate wire header rejected');

$missingRejected = false;
try {
    see64you_wire_parse(str_replace("Order: -3\n", '', $wire));
} catch (InvalidArgumentException $exception) {
    $missingRejected = $exception->getMessage() === 'invalid_wire_document';
}
check($missingRejected, 'missing required header rejected');

check(see64you_parse_int32('2147483647') === 2147483647, 'maximum int32 order accepted');
check(see64you_parse_int32('-2147483648') === -2147483648, 'minimum int32 order accepted');
check(see64you_parse_int32('+00042') === 42, 'signed padded order accepted');

foreach (['2147483648', '-2147483649', '1.5', ''] as $invalidOrder) {
    $rejected = false;
    try {
        see64you_parse_int32($invalidOrder);
    } catch (InvalidArgumentException $exception) {
        $rejected = $exception->getMessage() === 'invalid_wire_document';
    }
    check($rejected, 'invalid int32 order rejected: ' . $invalidOrder);
}

$source = str_repeat("abc\x00\xff", 100);
$chunks = see64you_source_chunks($source);
check(count($chunks) === (int)ceil(strlen($source) / SEE64YOU_SOURCE_CHUNK_BYTES), 'chunk count');
$roundTrip = '';
foreach ($chunks as $chunk) {
    $decoded = base64_decode($chunk, true);
    check($decoded !== false, 'chunk base64 valid');
    check(strlen($chunk) <= 240, 'chunk fits UCI structured string');
    $roundTrip .= $decoded;
}
check($roundTrip === $source, 'chunk source round trips bytes exactly');
check(see64you_source_chunks('') === [], 'empty source has no chunks');

check(see64you_status_from_wordpress('publish') === 'published', 'publish status canonicalized');
check(see64you_status_from_wordpress('future') === 'draft', 'non-published status maps to draft');
check(see64you_status_to_wordpress('published') === 'publish', 'published status maps to WordPress');
check(see64you_status_to_wordpress('draft') === 'draft', 'draft status maps to WordPress');
check(see64you_status_to_wordpress('private') === null, 'unsupported status rejected');

echo "wordpress protocol tests passed\n";
