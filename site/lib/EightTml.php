<?php
declare(strict_types=1);

/* Independently implemented producer validation for the 2026-10-20 8TML 1 draft. */
final class S64EightTml
{
    public const MAX_BYTES = 61440;

    public static function parse(string $source, string $fallbackTitle): array
    {
        if (strlen($source) > self::MAX_BYTES) {
            throw new InvalidArgumentException('document_too_large');
        }
        if (preg_match('/[^\x09\x0A\x20-\x7E]/', $source)
            || !str_starts_with($source, "---\n")) {
            throw new InvalidArgumentException('invalid_8tml');
        }
        $offset = 4;
        $versionSeen = false;
        $title = $fallbackTitle;
        while ($offset < strlen($source)) {
            $newline = strpos($source, "\n", $offset);
            $end = $newline === false ? strlen($source) : $newline;
            $line = substr($source, $offset, $end - $offset);
            $offset = $newline === false ? $end : $end + 1;
            if ($line === '---') {
                if (!$versionSeen) break;
                return ['title' => substr($title, 0, 80), 'body' => substr($source, $offset)];
            }
            if (trim($line, " \t") === '' || str_starts_with($line, '#')) continue;
            $colon = strpos($line, ':');
            if ($colon === false || $colon === 0) break;
            $key = strtolower(trim(substr($line, 0, $colon), " \t"));
            $value = trim(substr($line, $colon + 1), " \t");
            if ($key === '') break;
            if (!$versionSeen) {
                if ($key !== '8tml' || $value !== '1') break;
                $versionSeen = true;
            } elseif ($key === '8tml' && $value !== '1') {
                break;
            }
            if ($key === 'title') $title = $value === '' ? $fallbackTitle : $value;
        }
        throw new InvalidArgumentException('invalid_8tml');
    }
}