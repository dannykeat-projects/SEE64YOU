<?php
declare(strict_types=1);

final class S64PostArchive
{
    public const PAGE_SIZE = 10;

    public static function years(array $posts): array
    {
        $years = [];
        foreach ($posts as $post) {
            $year = self::yearFromDate((string)($post['date'] ?? ''));
            if ($year !== null && !in_array($year, $years, true)) {
                $years[] = $year;
            }
        }
        return $years;
    }

    public static function filterByYear(array $posts, ?string $year): array
    {
        if ($year === null) {
            return array_values($posts);
        }

        return array_values(array_filter(
            $posts,
            static fn(array $post): bool =>
                self::yearFromDate((string)($post['date'] ?? '')) === $year
        ));
    }

    public static function paginate(array $posts, int $page): array
    {
        $count = count($posts);
        $pages = max(1, (int)ceil($count / self::PAGE_SIZE));
        if ($page < 1 || $page > $pages) {
            return [
                'items' => [],
                'page' => $page,
                'pages' => $pages,
                'count' => $count,
                'valid' => false,
            ];
        }

        return [
            'items' => array_slice(
                $posts,
                ($page - 1) * self::PAGE_SIZE,
                self::PAGE_SIZE
            ),
            'page' => $page,
            'pages' => $pages,
            'count' => $count,
            'valid' => true,
        ];
    }

    public static function parseYear(string $value): ?string
    {
        if ($value === '') {
            return null;
        }
        return preg_match('/^[0-9]{4}$/', $value) === 1 ? $value : null;
    }

    public static function parsePage(string $value): int
    {
        if ($value === '') {
            return 1;
        }
        if (strlen($value) > 9 || preg_match('/^[1-9][0-9]*$/', $value) !== 1) {
            return 0;
        }

        $page = (int)$value;
        return $page > 0 ? $page : 0;
    }

    private static function yearFromDate(string $date): ?string
    {
        $year = substr($date, 0, 4);
        return preg_match('/^[0-9]{4}$/', $year) === 1 ? $year : null;
    }
}
