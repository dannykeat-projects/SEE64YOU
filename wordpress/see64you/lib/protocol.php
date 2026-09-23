<?php
declare(strict_types=1);

const SEE64YOU_WIRE_MIME = 'application/x-see64you';
const SEE64YOU_SOURCE_CHUNK_BYTES = 180;

function see64you_wire_parse(string $source): array
{
    $source = str_replace(["\r\n", "\r"], "\n", $source);
    $separator = strpos($source, "\n\n");
    if ($separator === false) {
        throw new InvalidArgumentException('invalid_wire_document');
    }

    $headerText = substr($source, 0, $separator);
    $body = substr($source, $separator + 2);
    $lines = explode("\n", $headerText);

    if (array_shift($lines) !== 'SEE64YOU/1') {
        throw new InvalidArgumentException('invalid_wire_document');
    }

    $headers = [];
    foreach ($lines as $line) {
        $colon = strpos($line, ':');
        if ($colon === false) {
            throw new InvalidArgumentException('invalid_wire_document');
        }

        $key = strtolower(trim(substr($line, 0, $colon)));
        $value = ltrim(substr($line, $colon + 1));
        if ($key === '' || array_key_exists($key, $headers)) {
            throw new InvalidArgumentException('invalid_wire_document');
        }
        $headers[$key] = $value;
    }

    foreach (['type', 'slug', 'title', 'status', 'date', 'order'] as $required) {
        if (!array_key_exists($required, $headers)) {
            throw new InvalidArgumentException('invalid_wire_document');
        }
    }

    if (!preg_match('/^[+-]?\d+$/', $headers['order'])) {
        throw new InvalidArgumentException('invalid_wire_document');
    }

    return [
        'type' => $headers['type'],
        'slug' => $headers['slug'],
        'title' => $headers['title'],
        'status' => $headers['status'],
        'date' => $headers['date'],
        'order' => (int)$headers['order'],
        'body' => $body,
    ];
}

function see64you_source_chunks(string $source): array
{
    if ($source === '') {
        return [];
    }

    return array_map(
        static fn(string $chunk): string => base64_encode($chunk),
        str_split($source, SEE64YOU_SOURCE_CHUNK_BYTES)
    );
}

function see64you_status_from_wordpress(string $status): string
{
    return $status === 'publish' ? 'published' : 'draft';
}

function see64you_status_to_wordpress(string $status): ?string
{
    return match ($status) {
        'published', 'publish' => 'publish',
        'draft' => 'draft',
        default => null,
    };
}
