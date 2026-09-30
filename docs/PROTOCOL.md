# Publishing protocol

The standalone backend defines the reference See64You document operations. The WordPress adapter exposes equivalent semantics where WordPress permits them.

## Canonical wire format

`SEE64YOU/1` uses 7-bit ASCII text with LF line endings. The C64 editor stores native PETSCII and converts at the document boundary. Unrepresentable characters are rejected rather than silently changed.

The body uses **See64You Markup v1**, defined in [MARKUP.md](MARKUP.md). Unsupported constructs remain literal source text.

For streamed document upload/download the media type is:

`application/x-see64you`

## Authentication

The standalone site supports two authentication families because normal web clients and the stock C64U direct publisher have different transport constraints.

### Browser / legacy bearer session

`POST api.php?action=login`

Field:

- `password`

Success returns a short-lived bearer token. Protected requests send:

`Authorization: Bearer <token>`

Passwords and bearer tokens are reusable credentials and must use HTTPS outside controlled local testing. This mode remains useful for browsers, the relay and compatibility transports.

### Direct C64U challenge/HMAC

Stock C64 Ultimate firmware 1.1 uses Network Target `$03` and raw HTTP rather than the unavailable HTTP Target `$06`.

A site-specific 256-bit direct key is provisioned through `pair.php`, which is HTTPS-only. The normal site password is used only on that HTTPS pairing page and is never sent by the C64U direct publisher.

Before each protected direct request:

`GET api.php?action=challenge`

returns a short-lived 16-byte challenge encoded as 32 hexadecimal characters.

The protected request includes:

```
X-See64You-Challenge: <32-hex challenge>
X-See64You-Signature: <64-hex HMAC-SHA256>
```

The HMAC input is the UTF-8/ASCII concatenation:

```
see64you-direct/1
<challenge>
<METHOD>
<exact request URI>
<SHA-256(body) as 64 lowercase hex characters>
```

using newline separators and the 32-byte direct key. The server verifies the method, exact URI and body hash and consumes an accepted challenge so a captured signed request cannot be replayed.

Direct mode provides request authentication, integrity and replay protection. It does not encrypt the document body. The current C64U transport therefore requires an explicit opt-in for `http://` and rejects `https://` rather than pretending raw TCP is TLS.

### Trusted-LAN relay

The optional relay accepts:

`X-See64You-Relay-Key: <64-hex-key>`

This key authenticates access to the relay only. It is never forwarded upstream and does not replace the upstream bearer token or WordPress Authorization header. The relay forwards only configured paths to one fixed HTTPS origin.

### WordPress

WordPress uses WordPress-native REST authentication. Application Passwords are the preferred initial mechanism. The client stores only the derived Basic Authorization header for the active session; it does not store the user's normal WordPress password.

## Standalone operations

### Status

`GET api.php?action=status`

Unauthenticated backend/protocol information.

### Challenge

`GET api.php?action=challenge`

Unauthenticated challenge issuance for direct C64U authentication. A challenge is useful only when direct authentication is configured.

### Settings

`GET api.php?action=settings`

`POST api.php?action=settings`

Protected. Editable public settings include:

- `site_title`
- `theme`: `plain`, `simple` or `c64`
- `text_color`
- `background_color`
- `link_color`
- `visited_color`

Styled themes enforce at least 4.5:1 foreground/background contrast for body, link and visited-link colours.

### List

`GET api.php?action=list&type=page`

`GET api.php?action=list&type=post`

Protected. Returns `count` plus document metadata.

### Get

`GET api.php?action=get&type=post&slug=hello-world`

Protected. The C64 path can request canonical streamed source so the body is decoded directly into the existing document buffer.

### Save

`POST api.php?action=save`

Fields/source provide:

- `type`
- `slug`
- `title`
- `body`
- `status`
- optional `date`
- optional `order`

Protected. Memory-constrained clients append `ack=1` for a compact acknowledgement rather than echoing the full saved document.

### Delete

`POST api.php?action=delete`

Protected. Fields:

- `type`
- `slug`

### Draft preview

Authenticated clients can request a short-lived document-specific preview token. Draft preview responses are no-store/noindex/no-referrer.

### Backup

`GET api.php?action=backup`

Protected. Returns public settings plus canonical page/post source, including drafts, but excludes authentication secrets.

## WordPress document operations

The WordPress adapter exposes equivalent semantics under `/wp-json/see64you/v1`:

- `GET /connection` — permission/capability check.
- `GET /documents?type=post&managed=1` — list editable managed documents.
- `GET /documents/<id>?chunks=1` — open source in bounded base64 chunks.
- `POST /documents?ack=1` — create from canonical wire source.
- `PATCH /documents/<id>?ack=1` — update; status `draft` unpublishes.
- `DELETE /documents/<id>` — move to WordPress Trash.

Canonical slugs are never silently changed to resolve WordPress collisions; the adapter returns a conflict instead.

## Response shape

Typical success:

```json
{"ok":true,"document":{...}}
```

Typical error:

```json
{"ok":false,"error":"invalid_slug"}
```

Clients map stable error identifiers to short user-facing messages.

## Versioning

The document protocol identifier is `see64you/1`. Direct request authentication is versioned independently as `see64you-direct/1`. Additive fields may be introduced compatibly; incompatible semantic changes require a new version.
