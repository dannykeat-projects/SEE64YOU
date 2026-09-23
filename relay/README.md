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

Python 3.9+ is sufficient; no third-party packages are required.

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

Then configure the C64U publisher to use the relay machine's LAN address, for example:

```text
http://192.168.1.10:6464/api.php?action=login
```

and enter the same relay key in See64You. The relay maps `/api.php` onto the configured `upstream_base` and forwards it over verified HTTPS.

For WordPress, a root upstream such as `https://blog.example.com/` plus the allowed prefix `/wp-json/see64you/` lets the same relay serve the See64You WordPress adapter without becoming an arbitrary proxy.
