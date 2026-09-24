<?php
declare(strict_types=1);

function s64_parse_int32(mixed $value, string $error = 'invalid_integer'): int
{
    if (is_int($value)) {
        if ($value < -2147483648 || $value > 2147483647) {
            throw new InvalidArgumentException($error);
        }
        return $value;
    }

    if (!is_string($value) || preg_match('/^[+-]?\d+$/', $value) !== 1) {
        throw new InvalidArgumentException($error);
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
        throw new InvalidArgumentException($error);
    }

    return (int)$value;
}
