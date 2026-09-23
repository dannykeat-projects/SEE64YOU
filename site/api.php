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

function input_data(): array
{
    $contentType = strtolower((string)($_SERVER['CONTENT_TYPE'] ?? ''));

    if (str_contains($contentType, S64Wire::MIME)) {
        return S64Wire::parse((string)file_get_contents('php://input'));
    }

    if (str_contains($contentType, 'application/json')) {
        $decoded = json_decode((string)file_get_contents('php://input'), true);
        return is_array($decoded) ? $decoded : [];
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

$action = (string)($_GET['action'] ?? 'status');

try {
    if ($action === 'status') {
        respond([
            'ok' => true,
            'protocol' => 'see64you/1',
            'backend' => 'standalone',
            'configured' => $auth->configured(),
        ]);
    }

    if ($action === 'login') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'POST') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }

        $input = input_data();
        $session = $auth->login((string)($input['password'] ?? ''));
        respond(['ok' => true] + $session);
    }

    if (!$auth->valid(S64Auth::requestToken())) {
        respond(['ok' => false, 'error' => 'unauthorized'], 401);
    }

    if ($action === 'settings') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'GET') {
            respond(['ok' => true, 'settings' => $settings->publicValues()]);
        }

        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'POST') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }

        respond(['ok' => true, 'settings' => $settings->update(input_data())]);
    }

    if ($action === 'list') {
        $type = (string)($_GET['type'] ?? '');
        respond(['ok' => true, 'documents' => $documents->list($type, false)]);
    }

    if ($action === 'get') {
        $type = (string)($_GET['type'] ?? '');
        $slug = (string)($_GET['slug'] ?? '');
        $document = $documents->get($type, $slug);
        if ($document === null) {
            respond(['ok' => false, 'error' => 'not_found'], 404);
        }
        if (wants_wire()) {
            respond_wire($document);
        }
        respond(['ok' => true, 'document' => $document]);
    }

    if ($action === 'save') {
        if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'POST') {
            respond(['ok' => false, 'error' => 'method_not_allowed'], 405);
        }
        $document = $documents->save(input_data());
        if (wants_wire()) {
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
        $input = input_data();
        $deleted = $documents->delete(
            (string)($input['type'] ?? ''),
            (string)($input['slug'] ?? '')
        );
        respond(['ok' => true, 'deleted' => $deleted]);
    }

    respond(['ok' => false, 'error' => 'unknown_action'], 404);
} catch (InvalidArgumentException | RuntimeException $exception) {
    $error = $exception->getMessage();
    $status = in_array($error, ['invalid_credentials'], true) ? 401 : 400;
    respond(['ok' => false, 'error' => $error], $status);
} catch (Throwable) {
    respond(['ok' => false, 'error' => 'server_error'], 500);
}
