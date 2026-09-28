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

Do not append `/api.php` or another backend path; the publisher constructs endpoint paths itself. Open **1 PUBLISHING SETTINGS**, choose the Standalone destination, set the relay machine's base URL, and enter the same 64-hex-character relay key from `config.json`. The relay maps the resulting `/api.php` request onto the configured `upstream_base` and forwards it over verified HTTPS.

Destination URL, plain-HTTP consent and relay key are stored by the separate `S64CFG.PRG` settings helper in `SEE64YOU.CFG`. Keeping Ultimate-DOS persistence in that helper avoids consuming the resident publisher's RAM. RUN/STOP cancels unsaved settings changes; **S SAVE + RETURN** writes them and returns to the publisher.

WordPress relay support remains implemented in source, but WordPress-specific client work is intentionally deferred until the standalone/general C64U path is working end-to-end.
