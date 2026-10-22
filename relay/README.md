# See64You secure local relay

The relay is an optional compatibility path for C64 Ultimate destinations that require HTTPS, including deferred WordPress use.

The firmware-1.1 Standalone baseline publishes directly through Network Target $03 and does not require a relay.

The relay keeps the C64U hop on a trusted LAN and performs the remote HTTPS connection using Python's certificate-verifying TLS stack.

## Security model

The relay is deliberately **not** a general web proxy.

- upstream origin is fixed in config.json and must use https://;
- redirects are not followed;
- requests require X-See64You-Relay-Key;
- allowed_clients can restrict C64U IP/subnet;
- only configured path prefixes are forwarded;
- request/response sizes are bounded;
- relay key is never forwarded upstream;
- upstream TLS certificate verification cannot be disabled by configuration.

The LAN leg is plaintext HTTP. Use it only on a trusted LAN.

## Setup

Python 3.10+; no third-party packages required.

Generate a key:

~~~sh
python relay/see64you_relay.py --generate-key
~~~

Copy configuration:

~~~sh
cp relay/config.example.json relay/config.json
~~~

Configure:

- upstream_base;
- relay_key;
- allowed_clients where practical;
- allowed_path_prefixes.

Run:

~~~sh
python relay/see64you_relay.py --config relay/config.json
~~~

Configure the C64U profile to use the relay machine's LAN base URL, for example:

~~~text
http://192.168.1.10:6464
~~~

Do not append /api.php; the publisher constructs backend paths.

The C64U setup/settings UX should hide relay details from users who do not need this path. Relay configuration belongs in Advanced Settings.

## Scope

The relay is optional and not required for offline editing or the default direct Publisher transport. Keep its access key distinct from the upstream site key. Configuration remains in Advanced Settings and is saved under the selected C64U application root with verified readback.
