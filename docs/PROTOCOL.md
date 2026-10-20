# Publishing protocol

> **Historical alpha reference only, not future 8TML requirements.** The released version described here used `.s64`/SEE64YOU/1. The next build supports only `.8tml`, with no old-file migration, importer or compatibility support. Reuse applicable hardware safety rules when replacing code; see [8TML direction](8TML-ROADMAP.md).


The standalone backend defines the reference See64You document operations. The WordPress adapter exposes equivalent semantics where WordPress permits them.

The C64 client is now a C64 Ultimate-only product, but the wire/backend contracts remain deliberately platform-independent.

## Canonical wire format

SEE64YOU/1 uses 7-bit ASCII text with LF line endings. The C64U editor stores native PETSCII and converts at the document boundary. Unrepresentable characters are rejected rather than silently changed.

The body uses See64You Markup v1 from MARKUP.md.

Media type:

application/x-see64you

## Authentication

The standalone site supports two authentication families.

### Browser / bearer session

POST api.php?action=login

Field:

- password

Success returns a short-lived bearer token.

Protected requests send:

Authorization: Bearer <token>

Passwords/bearer tokens are reusable credentials and require HTTPS outside controlled local testing.

### Direct C64U challenge/HMAC

The firmware-1.1 C64U baseline uses Network Target $03 and raw HTTP.

A site-specific 256-bit direct key is provisioned through HTTPS pair.php. The normal site password is not sent by the C64U direct publisher.

Before each protected direct request:

GET api.php?action=challenge

returns a short-lived 16-byte challenge as 32 hexadecimal characters.

The protected request includes:

~~~text
X-See64You-Challenge: <32-hex challenge>
X-See64You-Signature: <64-hex HMAC-SHA256>
~~~

HMAC input:

~~~text
see64you-direct/1
<challenge>
<METHOD>
<exact request URI>
<SHA-256(body) as 64 lowercase hex characters>
~~~

with newline separators and the 32-byte direct key.

The server verifies method, URI and body hash and consumes an accepted challenge.

Direct mode provides authentication, integrity and replay protection. It does **not** encrypt document content.

Therefore the current Network $03 direct transport requires explicit opt-in for http:// and does not pretend an https:// URL is TLS.

### Trusted-LAN relay

The optional relay accepts:

X-See64You-Relay-Key: <64-hex-key>

The key authenticates relay access only. It is not forwarded upstream and does not replace upstream authentication.

The relay forwards only configured paths to one fixed HTTPS origin.

### WordPress

WordPress uses WordPress-native REST authentication. Application Passwords are the preferred initial mechanism; never store the user's normal WordPress password in the C64U client.

## Standalone operations

### Status

GET api.php?action=status

Unauthenticated backend/protocol information.

### Challenge

GET api.php?action=challenge

Unauthenticated challenge issuance for direct authentication.

### Settings

GET api.php?action=settings

POST api.php?action=settings

Protected. Editable public settings include site title/theme/colours.

Styled themes enforce readable contrast.

Website colour controls accept only the 16 canonical C64 palette RGB values. Existing saved colours are preserved and approximated to the palette when displayed; new settings must use a listed palette value. Custom fonts, layout and user-saved themes remain independent of this restriction.

### Public C64U browser colours

The public browser still uses unauthenticated, read-only HTTP. The original client sends `Accept: application/x-see64you-browser` and receives `SEE64YOU-BROWSE/1` followed by ordinary `T` and `L` records.

Colour-aware clients request `Accept: application/x-see64you-browser; version=2`. The server replies with `SEE64YOU-BROWSE/2` and a mandatory first record `C\\t617` (example: blue background, white text, yellow links), followed by the same `T` and `L` records. The three uppercase hexadecimal digits are C64 colour indices (0-F) in background/text/link order. A new client accepts legacy v1 output from older servers with its default colours. Existing clients keep v1 output from upgraded servers. A v2 colour record must occur exactly once, immediately after the header. No document or font markup is added to the browser protocol, and no credentials are sent.


### List

GET api.php?action=list&type=page

GET api.php?action=list&type=post

Protected. Returns count plus document metadata.

For the constrained direct C64U browser, an additive bounded summary form is available:

~~~text
GET api.php?action=list&type=page&format=summary&offset=0
~~~

The response is ASCII text using `application/x-see64you-list`:

~~~text
SEE64YOU-LIST/1
Count: 2
Slug: example-page
Title: Example Page

~~~

`Count` is always present. `Slug` and `Title` are present only when `offset` is within the list. This lets the direct Network $03 client browse one item at a time with constant RAM use and without a general-purpose JSON parser.
### Get

GET api.php?action=get&type=post&slug=hello-world

Protected. The C64U path can stream canonical source directly into the editor/document service.

### Save

POST api.php?action=save

Fields/source provide:

- type
- slug
- title
- body
- status
- optional date
- optional order

Protected.

The C64U client may request compact acknowledgements (ack=1) because echoing an entire just-sent document is needless bandwidth and buffering, even though the new runtime has more memory.

### Delete

POST api.php?action=delete

Protected fields:

- type
- slug

### Draft preview

Authenticated clients can request a short-lived document-specific preview token. Draft preview responses are no-store/noindex/no-referrer.

### Backup

GET api.php?action=backup

Protected. Returns public settings plus canonical page/post source including drafts, but excludes authentication secrets.

## WordPress document operations

Equivalent adapter semantics under /wp-json/see64you/v1:

- GET /connection
- GET /documents?type=post&managed=1
- GET /documents/<id>?chunks=1
- POST /documents?ack=1
- PATCH /documents/<id>?ack=1
- DELETE /documents/<id>

Canonical slugs are never silently changed to resolve collisions.

## Response shape

Typical success:

~~~json
{"ok":true,"document":{...}}
~~~

Typical error:

~~~json
{"ok":false,"error":"invalid_slug"}
~~~

Clients map stable error identifiers to user-facing messages.

## Versioning

Document protocol identifier: see64you/1.

Direct authentication identifier: see64you-direct/1.

Additive fields may be introduced compatibly. Incompatible semantic changes require a new version.

The C64U runtime architecture may evolve independently without changing this wire protocol.
