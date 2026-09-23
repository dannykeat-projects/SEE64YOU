<?php
declare(strict_types=1);

final class S64Wire
{
    public const MIME = 'application/x-see64you';

    public static function parse(string $source): array
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
            $headers[$key] = $value;
        }

        return [
            'type' => $headers['type'] ?? '',
            'slug' => $headers['slug'] ?? '',
            'title' => $headers['title'] ?? '',
            'status' => $headers['status'] ?? 'draft',
            'date' => $headers['date'] ?? '',
            'order' => (int)($headers['order'] ?? 0),
            'body' => $body,
        ];
    }

    public static function serialize(array $document): string
    {
        $line = static fn(mixed $value): string =>
            str_replace(["\r", "\n"], ' ', (string)$value);

        return "SEE64YOU/1\n"
            . 'Type: ' . $line($document['type'] ?? '') . "\n"
            . 'Slug: ' . $line($document['slug'] ?? '') . "\n"
            . 'Title: ' . $line($document['title'] ?? '') . "\n"
            . 'Status: ' . $line($document['status'] ?? 'draft') . "\n"
            . 'Date: ' . $line($document['date'] ?? '') . "\n"
            . 'Order: ' . (int)($document['order'] ?? 0) . "\n\n"
            . str_replace(["\r\n", "\r"], "\n", (string)($document['body'] ?? ''));
    }
}
