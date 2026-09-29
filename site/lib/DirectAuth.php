<?php
declare(strict_types=1);

final class S64DirectAuth
{
    private const VERSION = 'see64you-direct/1';
    private const CHALLENGE_TTL = 300;

    public function __construct(
        private readonly array $config,
        private readonly string $storagePath
    ) {
    }

    public function configured(): bool
    {
        return preg_match(
            '/^[a-f0-9]{64}$/',
            strtolower((string)($this->config['direct_secret'] ?? ''))
        ) === 1;
    }

    public function challenge(): array
    {
        if (!$this->configured()) {
            throw new RuntimeException('direct_auth_not_configured');
        }

        $this->ensureStorage();
        $challenge = bin2hex(random_bytes(16));
        $expires = time() + self::CHALLENGE_TTL;
        $path = $this->challengePath($challenge);

        if (file_put_contents($path, (string)$expires, LOCK_EX) === false) {
            throw new RuntimeException('direct_auth_storage_failed');
        }

        return [
            'challenge' => $challenge,
            'expires' => $expires,
        ];
    }

    public function verify(
        string $challenge,
        string $signature,
        string $method,
        string $requestUri,
        string $body
    ): bool {
        if (!$this->configured()
            || preg_match('/^[a-f0-9]{32}$/', $challenge) !== 1
            || preg_match('/^[a-f0-9]{64}$/', $signature) !== 1) {
            return false;
        }

        $path = $this->challengePath($challenge);
        $handle = @fopen($path, 'r+');
        if ($handle === false) {
            return false;
        }

        $consume = false;

        $consume = false;

        try {
            if (!flock($handle, LOCK_EX)) {
                return false;
            }

            rewind($handle);
            $stored = trim((string)stream_get_contents($handle));
            if ($stored === '' || $stored === 'used' || !ctype_digit($stored)) {
                return false;
            }
            if ((int)$stored < time()) {
                $consume = true;
                $consume = true;
                ftruncate($handle, 0);
                rewind($handle);
                fwrite($handle, 'used');
                return false;
            }

            $bodyHash = hash('sha256', $body);
            $canonical = self::VERSION
                . "\n" . $challenge
                . "\n" . strtoupper($method)
                . "\n" . $requestUri
                . "\n" . $bodyHash;

            $key = hex2bin((string)$this->config['direct_secret']);
            if ($key === false) {
                return false;
            }

            $expected = hash_hmac('sha256', $canonical, $key);
            if (!hash_equals($expected, strtolower($signature))) {
                return false;
            }

            $consume = true;
            ftruncate($handle, 0);
            rewind($handle);
            fwrite($handle, 'used');
            fflush($handle);
            return true;
        } finally {
            flock($handle, LOCK_UN);
            fclose($handle);
            if ($consume) {
                @unlink($path);
            }
        }
    }

    public static function signature(
        string $secretHex,
        string $challenge,
        string $method,
        string $requestUri,
        string $body
    ): string {
        $key = hex2bin($secretHex);
        if ($key === false) {
            throw new InvalidArgumentException('invalid_direct_secret');
        }

        $canonical = self::VERSION
            . "\n" . $challenge
            . "\n" . strtoupper($method)
            . "\n" . $requestUri
            . "\n" . hash('sha256', $body);

        return hash_hmac('sha256', $canonical, $key);
    }

    private function ensureStorage(): void
    {
        if (is_dir($this->storagePath)) {
            return;
        }
        if (!mkdir($this->storagePath, 0700, true) && !is_dir($this->storagePath)) {
            throw new RuntimeException('direct_auth_storage_failed');
        }
    }

    private function challengePath(string $challenge): string
    {
        return $this->storagePath . '/challenge-' . $challenge;
    }
}
