<?php
declare(strict_types=1);

require __DIR__ . '/lib/bootstrap.php';

header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store');

function respond(array $payload, int $status = 200): never
{
    http_response_code($status);
    echo json_encode($payload, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE);
    exit;
}

function input_data(string $rawBody): array
{
    $contentType = strtolower((string)($_SERVER['CONTENT_TYPE'] ?? ''));

    if (str_contains($contentType, S64Wire::MIME)) {
        return S64Wire::parse($rawBody);
    }

    if (str_contains($contentType, 'application/json')) {
        $decoded = json_decode($rawBody, true);
        return is_array($decoded) ? $decoded : [];
    }

    if ($rawBody !== '') {
        parse_str($rawBody, $parsed);
        return is_array($parsed) ? $parsed : [];
    }

    return $_POST;
}

function wants_wire(): bool
{
    return strtolower((string)($_GET['format'] ?? '')) === 's64';
}

function wants_ack(): bool
{
    return (string)($_GET['ack'] ?? '') === '1';
}

function respond_wire(array $document, int $status = 200): never
{
    http_response_code($status);
    header('Content-Type: ' . S64Wire::MIME . '; charset=utf-8');
    header('Cache-Control: no-store');
    echo S64Wire::serialize($document);
    exit;
}

function respond_list_summary(array $documents, int $offset): never
{
    http_response_code(200);
    header('Content-Type: ' . S64Wire::LIST_MIME . '; charset=utf-8');
    header('Cache-Control: no-store');
    echo S64Wire::serializeListSummary($documents, $offset);
    exit;
}

function backup_documents(S64Documents $documents, string $type): array
{
    $result = [];
    foreach ($documents->list($type, false) as $summary) {
        $document = $documents->get($type, (string)$summary['slug']);
        if ($document !== null) {
            $result[] = $document;
        }
    }
    return $result;
}

function respond_backup(S64Documents $documents, S64Settings $settings): never
{
    $backup = [
        'format' => 'see64you-backup/1',
        'exported_at' => date(DATE_ATOM),
        'settings' => $settings->publicValues(),
        'pages' => backup_documents($documents, 'page'),
        'posts' => backup_documents($documents, 'post'),
    ];

    header('Content-Type: application/json; charset=utf-8');
    header('Content-Disposition: attachment; filename="see64you-backup.json"');
    header('Cache-Control: no-store');
    echo json_encode(
        $backup,
        JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT
    );
    exit;
}

$action = (string)($_GET['action'] ?? 'status');

/* A fully escaped 60 KiB 8TML source is under 128 KiB as JSON. Bound the
 * request before authentication, including requests with no Content-Length. */
const S64_API_MAX_BODY_BYTES = 131072;
$contentLength = (string)($_SERVER['CONTENT_LENGTH'] ?? '');
if ($contentLength !== '' && ctype_digit($contentLength)
    && (float)$contentLength > S64_API_MAX_BODY_BYTES) {
    respond(['ok' => false, 'error' => 'request_too_large'], 413);
}
$inputStream = fopen('php://input', 'rb');
if ($inputStream === false) {
    respond(['ok' => false, 'error' => 'server_error'], 500);
}
$rawBody = stream_get_contents($inputStream, S64_API_MAX_BODY_BYTES + 1);
fclose($inputStream);
if ($rawBody === false) {
    respond(['ok' => false, 'error' => 'server_error'], 500);
}
if (strlen($rawBody) > S64_API_MAX_BODY_BYTES) {
    respond(['ok' => false, 'error' => 'request_too_large'], 413);
}

try {
    if ($action === 'status') {
        respond([
            'ok' => true,
            'protocol' => 'see64you/1',
            'backend' => 'standalone',
            'configured' => $auth->configured(),
            'direct_auth' => $directAuth->configured(),
        ]);
    }

    if ($action === 'challenge') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }
        respond(['ok' => true] + $directAuth->challenge());
    }

    if ($action === 'login') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'POST') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }

        $input = input_data($rawBody);
        $session = $auth->login((string)($input['password'] ?? ''));
        respond(['ok' => true] + $session);
    }

    $browserAuthenticated = $auth->valid(S64Auth::requestToken());
    $directAuthenticated = false;

    if (!$browserAuthenticated) {
        $directAuthenticated = $directAuth->verify(
            strtolower(trim((string)($_SERVER['HTTP_X_SEE64YOU_CHALLENGE'] ?? ''))),
            strtolower(trim((string)($_SERVER['HTTP_X_SEE64YOU_SIGNATURE'] ?? ''))),
            (string)($_SERVER['REQUEST_METHOD'] ?? 'GET'),
            (string)($_SERVER['REQUEST_URI'] ?? ''),
            $rawBody
        );
    }

    if (!$browserAuthenticated && !$directAuthenticated) {
        respond(['ok' => false, 'error' => 'unauthorized'], 401);
    }

    if ($action === 'backup') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }
        respond_backup($documents, $settings);
    }

    if ($action === 'preview') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }

        $type = (string)($_GET['type'] ?? '');
        $slug = (string)($_GET['slug'] ?? '');
        $document = $documents->get($type, $slug);
        if ($document === null) {
            respond(['ok' => false, 'error' => 'not_found'], 404);
        }
        if ($document['status'] !== 'draft') {
            respond(['ok' => false, 'error' => 'not_draft'], 400);
        }

        $preview = $auth->previewToken($type, $slug);
        $script = $type === 'page' ? 'page.php' : 'post.php';
        respond([
            'ok' => true,
            'preview_url' => $script
                . '?slug=' . rawurlencode($slug)
                . '&preview=' . rawurlencode((string)$preview['token']),
            'expires' => $preview['expires'],
        ]);
    }

    if ($action === 'settings') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'GET') {
            respond(['ok' => true, 'settings' => $settings->publicValues()]);
        }

        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'POST') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }

        respond(['ok' => true, 'settings' => $settings->update(input_data($rawBody))]);
    }

    if ($action === 'list') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }

        $type = (string)($_GET['type'] ?? '');
        $source = $_GET['source'] ?? '';
        if (!is_string($source) || !in_array($source, ['', '8tml'], true)) {
            respond(['ok' => false, 'error' => 'invalid_source'], 400);
        }
        $listed = $documents->list($type, false, $source === '8tml');
        if (strtolower((string)($_GET['format'] ?? '')) === 'summary') {
            $offsetText = (string)($_GET['offset'] ?? '0');
            if (!preg_match('/^\\d{1,5}$/', $offsetText)
                || (int)$offsetText > 65535) {
                respond(['ok' => false, 'error' => 'invalid_offset'], 400);
            }
            respond_list_summary($listed, (int)$offsetText);
        }
        respond([
            'ok' => true,
            'count' => count($listed),
            'documents' => $listed,
        ]);
    }

    if ($action === 'get') {
        $type = (string)($_GET['type'] ?? '');
        $slug = (string)($_GET['slug'] ?? '');
        $document = $documents->get($type, $slug);
        if ($document === null) {
            respond(['ok' => false, 'error' => 'not_found'], 404);
        }
        if (strtolower((string)($_GET['format'] ?? '')) === '8tml') {
            if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') {
                respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
            }
            if (!isset($document['raw8tml'])) throw new InvalidArgumentException('not_8tml');
            header('Content-Type: text/8tml; charset=us-ascii');
            header('Content-Disposition: attachment; filename="' . $document['slug'] . '.8tml"');
            header('X-See64You-Status: ' . $document['status']);
            echo $document['raw8tml'];
            exit;
        }
        if (wants_wire()) {
            if (isset($document['raw8tml'])) throw new InvalidArgumentException('not_s64');
            respond_wire($document);
        }
        respond(['ok' => true, 'document' => $document]);
    }

    if ($action === 'save') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'POST') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }
        $input = input_data($rawBody);
        if (wants_wire() && array_key_exists('raw8tml', $input)) throw new InvalidArgumentException('not_s64');
        $document = $documents->save($input);
        if (wants_wire()) {
            if (isset($document['raw8tml'])) throw new InvalidArgumentException('not_s64');
            respond_wire($document);
        }
        if (wants_ack()) {
            respond([
                'ok' => true,
                'slug' => (string)$document['slug'],
                'status' => (string)$document['status'],
            ]);
        }
        respond(['ok' => true, 'document' => $document]);
    }

    if ($action === 'delete') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'POST') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }
        $input = input_data($rawBody);
        $deleted = $documents->delete(
            (string)($input['type'] ?? ''),
            (string)($input['slug'] ?? '')
        );
        respond(['ok' => true, 'deleted' => $deleted]);
    }

    respond(['ok' => false, 'error' => 'unknown_action'], 404);
} catch (InvalidArgumentException | RuntimeException $exception) {
    $error = $exception->getMessage();
    $status = $error === 'invalid_credentials' ? 401 : ($error === 'direct_auth_busy' ? 503 : 400);
    respond(['ok' => false, 'error' => $error], $status);
} catch (Throwable) {
    respond(['ok' => false, 'error' => 'server_error'], 500);
}
