# Publishing protocol

## Current 8TML source service

The PHP backend accepts 8TML independently of the still-unmigrated native Publisher. Use the existing authenticated `POST api.php?action=save&ack=1`, with `Content-Type: application/json` and fields `type`, `slug`, `status`, `date`, `order`, and `raw8tml`. The last field is the complete source, including its header. Type/slug/publication state/date/order are service metadata and are never inserted into that header. Source must be ASCII/LF, have a closed version-1 header with `8tml` as its first key, and fit the Editor's 61,440-byte capacity. Invalid input leaves existing source intact. Display title/body are derived from source; title is limited to 80 characters for service summaries without changing the source.

After the existing bearer or challenge/HMAC authentication, `GET api.php?action=get&type=page&slug=example&format=8tml` returns the exact stored source as `text/8tml; charset=us-ascii`, with a `.8tml` attachment filename and `Cache-Control: no-store`. GET without `format` returns JSON metadata plus `raw8tml`; lists omit raw source. Existing delete and backup actions handle the new record. The HMAC remains `see64you-direct/1` over the original method, URI and complete HTTP body before JSON decoding. HTTPS provisioning, explicit direct-HTTP consent and the absence of content encryption remain unchanged.

Each page/post is one guarded JSON service record (`pages/SLUG.8tml.php` or `posts/SLUG.8tml.php`), containing metadata and exact source together. This envelope is storage/transport, not an editing format: canonical `.8tml` export is its unchanged `raw8tml` value. Recovery requires removing the first PHP-guard line and decoding ordinary JSON, or using authenticated export/backup. Save writes a complete temporary guarded `.php` snapshot and atomically renames it; one persistent empty `.documents.lock` per type directory serializes save/delete conflict checks. Do not remove those locks during operation. JSON tag escaping protects records containing PHP examples while decoding to identical source. There is no separate metadata sidecar that can commit independently of source.

Legacy records remain readable through their existing service, but a new-format save cannot replace a legacy identity (or vice versa), and 8TML cannot be retrieved through `format=s64`. No conversion/importer is provided. Public HTML currently uses the existing escaped limited markup subset on the extracted body; full 8TML rendering is not claimed. Native Publisher upload/download still needs to switch to this contract. Its streamed request hash and Content-Length now use a checked 32-bit byte count so JSON escaping can exceed 65,535 bytes without truncation. Source capacity remains 61,440 bytes; individual UCI writes and the response reader retain their existing bounded sizes. This transport prerequisite does not itself implement native JSON source upload.

## Historical alpha protocol

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
