<?php
declare(strict_types=1);

final class S64Auth
{
    public function __construct(private readonly array $config)
    {
    }

    public function configured(): bool
    {
        return (string)$this->config['password_hash'] !== ''
            && strlen((string)$this->config['token_secret']) >= 32;
    }

    public function login(string $password): array
    {
        if (!$this->configured()) {
            throw new RuntimeException('not_configured');
        }

        if (!password_verify($password, (string)$this->config['password_hash'])) {
            throw new RuntimeException('invalid_credentials');
        }

        $expiry = time() + max(300, (int)$this->config['token_ttl']);
        $nonce = bin2hex(random_bytes(8));
        $payload = 'v1|' . $expiry . '|' . $nonce;
        $signature = hash_hmac('sha256', $payload, (string)$this->config['token_secret']);

        return [
            'token' => 'v1.' . $expiry . '.' . $nonce . '.' . $signature,
            'expires' => $expiry,
        ];
    }

    public function valid(string $token): bool
    {
        if (!$this->configured()) {
            return false;
        }

        $parts = explode('.', $token);
        if (count($parts) !== 4 || $parts[0] !== 'v1') {
            return false;
        }

        [, $expiryText, $nonce, $signature] = $parts;
        if (!ctype_digit($expiryText) || (int)$expiryText < time()) {
            return false;
        }
        if (!preg_match('/^[a-f0-9]{16}$/', $nonce) || !preg_match('/^[a-f0-9]{64}$/', $signature)) {
            return false;
        }

        $payload = 'v1|' . $expiryText . '|' . $nonce;
        $expected = hash_hmac('sha256', $payload, (string)$this->config['token_secret']);

        return hash_equals($expected, $signature);
    }

    public function previewToken(string $type, string $slug): array
    {
        if (!$this->configured()) {
            throw new RuntimeException('not_configured');
        }
        if (!$this->validPreviewTarget($type, $slug)) {
            throw new InvalidArgumentException('invalid_preview_target');
        }

        $expiry = time() + 3600;
        $payload = 'p1|' . $expiry . '|' . $type . '|' . $slug;
        $signature = hash_hmac('sha256', $payload, (string)$this->config['token_secret']);

        return [
            'token' => 'p1.' . $expiry . '.' . $signature,
            'expires' => $expiry,
        ];
    }

    public function validPreviewToken(string $token, string $type, string $slug): bool
    {
        if (!$this->configured() || !$this->validPreviewTarget($type, $slug)) {
            return false;
        }

        $parts = explode('.', $token);
        if (count($parts) !== 3 || $parts[0] !== 'p1') {
            return false;
        }

        [, $expiryText, $signature] = $parts;
        if (!ctype_digit($expiryText) || (int)$expiryText < time()) {
            return false;
        }
        if (!preg_match('/^[a-f0-9]{64}$/', $signature)) {
            return false;
        }

        $payload = 'p1|' . $expiryText . '|' . $type . '|' . $slug;
        $expected = hash_hmac('sha256', $payload, (string)$this->config['token_secret']);

        return hash_equals($expected, $signature);
    }

    private function validPreviewTarget(string $type, string $slug): bool
    {
        return in_array($type, ['page', 'post'], true)
            && strlen($slug) <= 64
            && preg_match('/^[a-z0-9]+(?:-[a-z0-9]+)*$/', $slug) === 1;
    }

    public static function requestToken(): string
    {
        $authorization = (string)($_SERVER['HTTP_AUTHORIZATION'] ?? '');
        if (preg_match('/^Bearer\s+(.+)$/i', $authorization, $matches)) {
            return trim($matches[1]);
        }

        return trim((string)($_SERVER['HTTP_X_SEE64YOU_TOKEN'] ?? ''));
    }
}
