<?php
declare(strict_types=1);

require_once __DIR__ . '/EightTml.php';

final class S64Documents
{
    private const GUARD = '<?php http_response_code(404); exit; ?>';

    public function __construct(private readonly string $root)
    {
    }

    public function list(string $type, bool $publishedOnly = false, bool $onlyEightTml = false): array
    {
        $directory = $this->directory($type);
        $documents = [];

        $paths = glob($directory . '/*.8tml.php') ?: [];
        if (!$onlyEightTml) $paths = array_merge(glob($directory . '/*.s64.php') ?: [], $paths);
        foreach ($paths as $path) {
            $isEightTml = str_ends_with($path, '.8tml.php');
            $slug = basename($path, $isEightTml ? '.8tml.php' : '.s64.php');
            $document = $isEightTml ? $this->readEightTml($path, $type, $slug) : $this->readPath($path, $type, $slug);

            if ($publishedOnly && $document['status'] !== 'published') {
                continue;
            }

            unset($document['body'], $document['raw8tml']);
            $documents[] = $document;
        }

        if ($type === 'post') {
            usort($documents, static fn(array $a, array $b): int =>
                strcmp((string)$b['date'], (string)$a['date'])
            );
        } else {
            usort($documents, static function (array $a, array $b): int {
                $order = ((int)$a['order']) <=> ((int)$b['order']);
                return $order !== 0 ? $order : strcasecmp((string)$a['title'], (string)$b['title']);
            });
        }

        return $documents;
    }

    public function get(string $type, string $slug): ?array
    {
        $slug = $this->validateSlug($slug);
        $path = $this->path($type, $slug);

        $sourcePath = $this->eightTmlPath($type, $slug);
        if (is_file($path) && is_file($sourcePath)) throw new RuntimeException('format_conflict');
        if (is_file($sourcePath)) return $this->readEightTml($sourcePath, $type, $slug);
        return is_file($path) ? $this->readPath($path, $type, $slug) : null;
    }

    public function save(array $input): array
    {
        if (array_key_exists('raw8tml', $input)) {
            foreach (['type', 'slug', 'status', 'date'] as $field) {
                if (array_key_exists($field, $input) && !is_string($input[$field])) {
                    throw new InvalidArgumentException('invalid_' . $field);
                }
            }
        }
        $type = $this->validateType((string)($input['type'] ?? ''));
        $this->validateSlug((string)($input['slug'] ?? ''));
        return $this->withWriteLock($type, fn(): array => $this->saveUnlocked($input));
    }

    public function publish(array $input, string $intent, string $identity, string $revision = ''): array
    {
        foreach (['type', 'slug', 'status', 'date'] as $field) {
            if (array_key_exists($field, $input) && !is_string($input[$field])) {
                throw new InvalidArgumentException('invalid_' . $field);
            }
        }
        $type = $this->validateType((string)($input['type'] ?? ''));
        $slug = $this->validateSlug((string)($input['slug'] ?? ''));
        if (!in_array($intent, ['create', 'update'], true)) throw new InvalidArgumentException('invalid_intent');
        if ($intent === 'update' && $identity !== $slug) throw new InvalidArgumentException('identity_required');
        if ($intent === 'update' && !preg_match('/^[a-f0-9]{32}$/D', $revision)) {
            throw new InvalidArgumentException('revision_required');
        }
        return $this->withWriteLock($type, function () use ($input, $intent, $type, $slug, $revision): array {
            $existing = $this->get($type, $slug);
            if ($intent === 'create' && $existing !== null) throw new InvalidArgumentException('already_exists');
            if ($intent === 'update' && $existing === null) throw new InvalidArgumentException('not_found');
            if ($intent === 'update' && !hash_equals((string)($existing['revision'] ?? ''), $revision)) {
                throw new InvalidArgumentException('revision_conflict');
            }
            return $this->saveUnlocked($input);
        });
    }

    private function saveUnlocked(array $input): array
    {
        if (array_key_exists('raw8tml', $input)) return $this->saveEightTml($input);
        $type = $this->validateType((string)($input['type'] ?? ''));
        $slug = $this->validateSlug((string)($input['slug'] ?? ''));
        if (is_file($this->eightTmlPath($type, $slug))) throw new InvalidArgumentException('format_conflict');
        $title = trim((string)($input['title'] ?? ''));
        $body = str_replace(["\r\n", "\r"], "\n", (string)($input['body'] ?? ''));
        $status = (string)($input['status'] ?? 'draft');
        $date = trim((string)($input['date'] ?? ''));
        $order = $this->validateOrder($input['order'] ?? 0);

        if ($title === '' || strlen($title) > 80) {
            throw new InvalidArgumentException('invalid_title');
        }
        if (!in_array($status, ['draft', 'published'], true)) {
            throw new InvalidArgumentException('invalid_status');
        }
        if ($date === '') {
            $date = date(DATE_ATOM);
        }
        if (strlen($date) > 40) {
            throw new InvalidArgumentException('invalid_date');
        }
        if (strlen($body) > 8192) {
            throw new InvalidArgumentException('document_too_large');
        }

        $document = [
            'type' => $type,
            'slug' => $slug,
            'title' => $title,
            'status' => $status,
            'date' => $date,
            'order' => $order,
            'body' => $body,
        ];

        $serialized = $this->serialize($document);
        $path = $this->path($type, $slug);
        $temporary = $path . '.tmp.' . bin2hex(random_bytes(4));

        if (file_put_contents($temporary, $serialized, LOCK_EX) === false) {
            throw new RuntimeException('write_failed');
        }

        if (!rename($temporary, $path)) {
            @unlink($temporary);
            throw new RuntimeException('write_failed');
        }

        return $document;
    }

    public function delete(string $type, string $slug): bool
    {
        $this->validateSlug($slug);
        return $this->withWriteLock($type, fn(): bool => $this->deleteUnlocked($type, $slug));
    }

    private function deleteUnlocked(string $type, string $slug): bool
    {
        $path = $this->path($type, $this->validateSlug($slug));

        $sourcePath = $this->eightTmlPath($type, $slug);
        if (is_file($path) && is_file($sourcePath)) throw new RuntimeException('format_conflict');
        if (is_file($sourcePath)) return unlink($sourcePath);
        if (!is_file($path)) return false;
        return unlink($path);
    }

    private function withWriteLock(string $type, callable $operation): mixed
    {
        /* Keep one stable inode per directory: deleting lock files can split
         * waiting writers across different locks. Source snapshots stay atomic. */
        $handle = @fopen($this->directory($type) . '/.documents.lock', 'c');
        if ($handle === false) throw new RuntimeException('storage_unavailable');
        try {
            if (!flock($handle, LOCK_EX)) throw new RuntimeException('storage_unavailable');
            clearstatcache();
            return $operation();
        } finally {
            flock($handle, LOCK_UN);
            fclose($handle);
        }
    }
    private function eightTmlPath(string $type, string $slug): string
    {
        return $this->directory($type) . '/' . $slug . '.8tml.php';
    }

    private function saveEightTml(array $input): array
    {
        $type = $this->validateType((string)($input['type'] ?? ''));
        $slug = $this->validateSlug((string)($input['slug'] ?? ''));
        if (!is_string($input['raw8tml'])) throw new InvalidArgumentException('invalid_8tml');
        $parsed = S64EightTml::parse($input['raw8tml'], $slug);
        if (is_file($this->path($type, $slug))) throw new InvalidArgumentException('format_conflict');
        $path = $this->eightTmlPath($type, $slug);
        /* This read is inside the directory write lock, after waiting writers
         * can observe a prior update or deletion. Never reuse stale metadata. */
        $needsExisting = !array_key_exists('date', $input) || !array_key_exists('order', $input);
        $existing = $needsExisting && is_file($path) ? $this->readEightTml($path, $type, $slug) : null;
        $status = (string)($input['status'] ?? 'draft');
        if (!in_array($status, ['draft', 'published'], true)) throw new InvalidArgumentException('invalid_status');
        $date = array_key_exists('date', $input)
            ? trim((string)$input['date']) : (string)($existing['date'] ?? '');
        if ($date === '') $date = date(DATE_ATOM);
        if (strlen($date) > 40) throw new InvalidArgumentException('invalid_date');
        $document = [
            'type' => $type, 'slug' => $slug, 'status' => $status, 'date' => $date,
            'order' => $this->validateOrder(array_key_exists('order', $input)
                ? $input['order'] : ($existing['order'] ?? 0)),
            'raw8tml' => $input['raw8tml'],
            'revision' => bin2hex(random_bytes(16)),
        ];
        $serialized = self::GUARD . "\n" . json_encode($document, JSON_THROW_ON_ERROR | JSON_UNESCAPED_SLASHES | JSON_HEX_TAG);
        $temporary = $path . '.tmp.' . bin2hex(random_bytes(4)) . '.php';
        if (file_put_contents($temporary, $serialized, LOCK_EX) !== strlen($serialized)) {
            @unlink($temporary);
            throw new RuntimeException('write_failed');
        }
        if (!rename($temporary, $path)) {
            @unlink($temporary);
            throw new RuntimeException('write_failed');
        }
        return $document + $parsed;
    }

    private function readEightTml(string $path, string $type, string $slug): array
    {
        $content = file_get_contents($path);
        if ($content === false) throw new RuntimeException('read_failed');
        $prefix = self::GUARD . "\n";
        if (!str_starts_with($content, $prefix)) throw new RuntimeException('invalid_record');
        try {
            $document = json_decode(substr($content, strlen($prefix)), true, 16, JSON_THROW_ON_ERROR);
            if (!is_array($document) || ($document['type'] ?? null) !== $type
                || ($document['slug'] ?? null) !== $slug || !is_string($document['raw8tml'] ?? null)
                || !in_array($document['status'] ?? null, ['draft', 'published'], true)
                || !is_string($document['date'] ?? null) || strlen($document['date']) > 40) {
                throw new RuntimeException('invalid_record');
            }
            $document['order'] = $this->validateOrder($document['order'] ?? null);
            if (array_key_exists('revision', $document)
                && (!is_string($document['revision']) || !preg_match('/^[a-f0-9]{32}$/D', $document['revision']))) {
                throw new RuntimeException('invalid_record');
            }
            // Existing records gain a stable comparison token without a read-time write.
            $document['revision'] ??= substr(hash('sha256', $content), 0, 32);
            $parsed = S64EightTml::parse($document['raw8tml'], $slug);
            return array_intersect_key($document, array_flip(['type', 'slug', 'status', 'date', 'order', 'raw8tml', 'revision'])) + $parsed;
        } catch (JsonException | InvalidArgumentException $error) {
            throw new RuntimeException('invalid_record', 0, $error);
        }
    }
    private function directory(string $type): string
    {
        $type = $this->validateType($type);
        $directory = $this->root . '/' . ($type === 'page' ? 'pages' : 'posts');

        if (!is_dir($directory) && !mkdir($directory, 0775, true) && !is_dir($directory)) {
            throw new RuntimeException('storage_unavailable');
        }

        return $directory;
    }

    private function path(string $type, string $slug): string
    {
        return $this->directory($type) . '/' . $slug . '.s64.php';
    }

    private function validateType(string $type): string
    {
        if (!in_array($type, ['page', 'post'], true)) {
            throw new InvalidArgumentException('invalid_type');
        }
        return $type;
    }

    private function validateOrder(mixed $value): int
    {
        if (is_int($value)) {
            if ($value < -2147483648 || $value > 2147483647) {
                throw new InvalidArgumentException('invalid_order');
            }
            return $value;
        }

        if (!is_string($value)) {
            throw new InvalidArgumentException('invalid_order');
        }

        $value = trim($value);
        if ($value === '') {
            return 0;
        }
        if (!preg_match('/^[+-]?\d+$/', $value)) {
            throw new InvalidArgumentException('invalid_order');
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
            throw new InvalidArgumentException('invalid_order');
        }

        return (int)$value;
    }

    private function validateSlug(string $slug): string
    {
        if (strlen($slug) > 64
            || !preg_match('/^[a-z0-9]+(?:-[a-z0-9]+)*$/', $slug)) {
            throw new InvalidArgumentException('invalid_slug');
        }
        return $slug;
    }

    private function readPath(string $path, string $type, string $slug): array
    {
        $content = file_get_contents($path);
        if ($content === false) {
            throw new RuntimeException('read_failed');
        }

        $content = str_replace(["\r\n", "\r"], "\n", $content);
        $lines = explode("\n", $content);

        if (($lines[0] ?? '') === self::GUARD) {
            array_shift($lines);
        }

        $headers = [];
        while ($lines !== []) {
            $line = array_shift($lines);
            if ($line === '') {
                break;
            }
            $colon = strpos($line, ':');
            if ($colon === false) {
                continue;
            }
            $key = strtolower(trim(substr($line, 0, $colon)));
            $headers[$key] = trim(substr($line, $colon + 1));
        }

        return [
            'type' => $type,
            'slug' => $slug,
            'title' => $headers['title'] ?? $slug,
            'status' => $headers['status'] ?? 'draft',
            'date' => $headers['date'] ?? '',
            'order' => $this->validateOrder($headers['order'] ?? '0'),
            'body' => implode("\n", $lines),
        ];
    }

    private function serialize(array $document): string
    {
        $clean = static fn(string $value): string => str_replace(["\r", "\n"], ' ', $value);

        return self::GUARD . "\n"
            . 'Title: ' . $clean((string)$document['title']) . "\n"
            . 'Status: ' . $clean((string)$document['status']) . "\n"
            . 'Date: ' . $clean((string)$document['date']) . "\n"
            . 'Order: ' . (int)$document['order'] . "\n\n"
            . (string)$document['body'];
    }
}
