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

        $order = self::parseInt32($headers['order']);

        return [
            'type' => $headers['type'] ?? '',
            'slug' => $headers['slug'] ?? '',
            'title' => $headers['title'] ?? '',
            'status' => $headers['status'] ?? 'draft',
            'date' => $headers['date'] ?? '',
            'order' => $order,
            'body' => $body,
        ];
    }

    private static function parseInt32(string $value): int
    {
        if (!preg_match('/^[+-]?\d+$/', $value)) {
            throw new InvalidArgumentException('invalid_wire_document');
        }

        $negative = str_starts_with($value, '-');
        $digits = ltrim($value, '+-');
        $digits = ltrim($digits, '0');
        if ($digits === '') {
            return 0;
        }

        $limit = $negative ? '2147483648' : '2147483647';
        if (strlen($digits) > strlen($limit)
            || (strlen($digits) === strlen($limit) && strcmp($digits, $limit) > 0)) {
            throw new InvalidArgumentException('invalid_wire_document');
        }

        return (int)$value;
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
