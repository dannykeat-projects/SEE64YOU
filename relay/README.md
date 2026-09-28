# See64You secure local relay

Current C64U UCI HTTP firmware can perform outbound HTTP requests, but See64You does not treat its current upstream implementation as verified TLS. The optional relay keeps the C64U hop on a trusted LAN and performs the remote HTTPS connection using Python's normal certificate-verifying TLS stack.

## Security model

The relay is deliberately **not** a general web proxy.

- The upstream origin is fixed in `config.json` and must use `https://`.
- Redirects are not followed.
- Requests require a 64-hex-character `X-See64You-Relay-Key`.
- `allowed_clients` can restrict access to the C64U's IP address or subnet.
- Only configured path prefixes are forwarded.
- Request and response sizes are bounded.
- The relay never forwards its own relay key upstream.
- Upstream TLS certificate verification is enabled and cannot be disabled by configuration.

The LAN leg is still plaintext HTTP. Treat the configured LAN as trusted and prefer a dedicated/home network rather than public Wi-Fi.

## Setup

Python 3.10+ is sufficient; no third-party packages are required.

Generate a relay key:

```sh
python relay/see64you_relay.py --generate-key
```

Copy the sample configuration:

```sh
cp relay/config.example.json relay/config.json
```

On Windows, copy the file in Explorer or PowerShell instead.

Set:

- `upstream_base` to the remote HTTPS site root. For a standalone site in `https://example.com/site/`, keep the trailing slash.
- `relay_key` to the generated key.
- `allowed_clients` to the C64U IP/CIDR where practical.
- `allowed_path_prefixes` to the endpoints that this relay should expose.

Run:

```sh
python relay/see64you_relay.py --config relay/config.json
```

Then choose the appropriate destination type in the C64U publisher and set its **site URL** to the relay machine's base LAN address, for example:

```text
http://192.168.1.10:6464
```

Do not append `/api.php` or a WordPress REST path; the publisher constructs those endpoint paths itself. Use **8 RELAY KEY** in the publisher and enter the same 64-hex-character relay key from `config.json`. The relay maps the resulting `/api.php` or `/wp-json/see64you/` request onto the configured `upstream_base` and forwards it over verified HTTPS.

The relay key currently lasts for the publisher session. Durable publisher destination/relay persistence is intentionally deferred until it can be implemented without breaching the C64 publisher's 1 KiB memory-headroom guard.

For WordPress, a root upstream such as `https://blog.example.com/` plus the allowed prefix `/wp-json/see64you/` lets the same relay serve the See64You WordPress adapter without becoming an arbitrary proxy.
