# Publishing protocol

The standalone backend defines the reference See64You document operations. The WordPress adapter should expose equivalent semantics where WordPress permits them.

## Authentication

Standalone:

`POST api.php?action=login`

Fields:
- `password`

Success returns JSON containing a bearer token and expiry. The C64U standalone publisher stores the returned token and expiry only; it does not retain the submitted password. Subsequent protected calls send:

`Authorization: Bearer <token>`

Normal browser/API clients should use HTTPS outside trusted local testing.

### C64U transport note

The current upstream UCI HTTP target must not be treated as TLS merely because it accepts an `https://` resource string. Rechecking upstream `master` shows the header parser selecting port 443 and recording a secure flag, while the exchange path still opens a plain TCP socket and sends the HTTP request directly with no TLS handshake. See64You therefore rejects direct `https://` login until secure target behaviour is verified, and requires explicit opt-in before sending credentials over plaintext HTTP.

For real remote publishing, use the trusted local relay in `relay/` or future firmware with verified TLS support. The relay accepts the normal See64You path/query over LAN HTTP, requires its own 64-hex-character relay key, and forwards the request to one fixed HTTPS upstream with certificate verification. When relay authentication is configured, the C64 client adds:

`X-See64You-Relay-Key: <64-hex-key>`

This key authenticates access to the relay only; it does not replace the standalone bearer token or WordPress Authorization header and is never forwarded to the upstream site.

Until the standalone site is configured, no bearer token is accepted for protected operations. An empty or missing token secret must never act as a usable signing key.

WordPress uses WordPress-native REST authentication. Application Passwords are the preferred initial mechanism. The C64U client accepts the WordPress username plus an Application Password, strips the display-only spaces from the Application Password, and stores only the resulting Basic Authorization header for the active session. A protected `GET /wp-json/see64you/v1/connection` endpoint verifies both authentication and edit capability before document operations are enabled.

## Operations

### Status

`GET api.php?action=status`

Unauthenticated. Returns backend/protocol information without secrets.

### Settings

`GET api.php?action=settings`

Returns the editable public site settings after authentication.

`POST api.php?action=settings`

Accepts any supported subset of:
- `site_title`
- `theme`: `plain`, `simple` or `c64`
- `text_color`
- `background_color`
- `link_color`
- `visited_color`

Colours use six-digit CSS hex form such as `#a5a5ff`. Authentication secrets are never accepted or returned by this operation.

### List

`GET api.php?action=list&type=page`

`GET api.php?action=list&type=post`

Requires authentication.

The response includes `count` as well as the `documents` array. The C64U client uses the firmware's structured response-body queries to read `count` and then one metadata record at a time, avoiding a second large list buffer.

### Get

`GET api.php?action=get&type=post&slug=hello-world`

Requires authentication.

### Save

`POST api.php?action=save`

Form or JSON fields:
- `type`: `page` or `post`
- `slug`
- `title`
- `body`
- `status`: `draft` or `published`
- `date` (optional)
- `order` (optional)

Requires authentication.

For memory-constrained clients, append `ack=1` to request a compact response:

`POST api.php?action=save&ack=1`

This returns only `ok`, `slug` and `status` rather than echoing the complete saved document. It is particularly useful when the request body uses `application/x-see64you`.

### Delete

`POST api.php?action=delete`

Fields:
- `type`
- `slug`

Requires authentication.

## Response shape

Responses are small JSON objects:

```json
{"ok":true,"document":{...}}
```

Errors:

```json
{"ok":false,"error":"invalid_slug"}
```

The C64 client should map stable error identifiers to short user-facing messages.

## Versioning

The initial protocol identifier is `see64you/1`. Additive fields may be introduced without changing it. Incompatible operation/meaning changes require a new protocol version.
