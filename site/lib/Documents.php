<?php
declare(strict_types=1);

final class S64Documents
{
    private const GUARD = '<?php http_response_code(404); exit; ?>';

    public function __construct(private readonly string $root)
    {
    }

    public function list(string $type, bool $publishedOnly = false): array
    {
        $directory = $this->directory($type);
        $documents = [];

        foreach (glob($directory . '/*.s64.php') ?: [] as $path) {
            $slug = basename($path, '.s64.php');
            $document = $this->readPath($path, $type, $slug);

            if ($publishedOnly && $document['status'] !== 'published') {
                continue;
            }

            unset($document['body']);
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

        return is_file($path) ? $this->readPath($path, $type, $slug) : null;
    }

    public function save(array $input): array
    {
        $type = $this->validateType((string)($input['type'] ?? ''));
        $slug = $this->validateSlug((string)($input['slug'] ?? ''));
        $title = trim((string)($input['title'] ?? ''));
        $body = str_replace(["\r\n", "\r"], "\n", (string)($input['body'] ?? ''));
        $status = (string)($input['status'] ?? 'draft');
        $date = trim((string)($input['date'] ?? ''));
        $order = (int)($input['order'] ?? 0);

        if ($title === '') {
            throw new InvalidArgumentException('invalid_title');
        }
        if (!in_array($status, ['draft', 'published'], true)) {
            throw new InvalidArgumentException('invalid_status');
        }
        if ($date === '') {
            $date = date(DATE_ATOM);
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
        $path = $this->path($type, $this->validateSlug($slug));

        if (!is_file($path)) {
            return false;
        }

        return unlink($path);
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

    private function validateSlug(string $slug): string
    {
        if (!preg_match('/^[a-z0-9]+(?:-[a-z0-9]+)*$/', $slug)) {
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
            'order' => (int)($headers['order'] ?? 0),
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
