# Architecture

## Components

See64You deliberately separates editing from publishing.

```
+----------------------------+
| C64/C64U editor            |
|                            |
| document <-> editor <-> UI |
|       \                    |
|        publisher interface |
+---------------|------------+
                |
          HTTP transport
          /            \
+----------------+  +------------------+
| Standalone PHP |  | WordPress plugin |
| flat-file CMS  |  | REST adapter     |
+----------------+  +------------------+
```

The editor remains useful without a network connection. A publisher receives and returns documents; it does not own editor state.

## Canonical document

The canonical editable representation is source text plus metadata:

- type: `page` or `post`
- slug
- title
- status: initially `draft` or `published`
- date
- order (primarily pages)
- body source

Generated HTML is disposable output. Backends must retain the source exactly enough for a later edit.

## Client

The first editor uses a fixed-capacity gap buffer. This gives cheap insertion/deletion around the cursor without repeated whole-document shifts. The initial capacity is deliberately modest so the first build can work in normal C64 address space. REU storage can later extend capacity behind the same document/editor interfaces.

The UI baseline is 40x25:
- row 0: menu/identity bar
- rows 1-23: editor viewport
- row 24: status

The conventional-C64 runtime is linked conservatively: implemented feature cores that have no active UI path yet may live in separate translation units and remain host/cc65-module tested without being linked into the shipping PRG. This prevents dormant functionality from consuming scarce base RAM. Formatting currently follows this pattern in `editor_format.c`; splitting it restored the software stack from 512 bytes to 1 KiB while preserving formatting regression coverage.

C64U builds retain the same explicit memory-headroom gate rather than treating REU capacity as conventional address space. If future transition-only code makes that gate difficult to hold, a supported cc65 escape hatch is a custom linker segment with distinct load/run addresses: ship the block in ordinary PRG space, copy it to a known transient execution area only for the transition, and keep it out of the resident editor footprint. This is a contingency, not the default architecture; prefer the current editor/publisher module boundary while it fits and remains simpler.

The first implementation prioritises correctness and a runnable shell. Incremental rendering and selection follow after baseline VICE validation.

## Transport

Publishing operations depend on a narrow transport layer rather than direct UCI calls scattered through UI code. The C64U implementation uses the firmware 3.15+ UCI HTTP target ($06).

The low-level UCI executor:
- enforces the documented 896-byte command limit;
- writes commands as small caller-owned parts, avoiding a second 896-byte staging buffer;
- drains response/status queues and acknowledges every returned block;
- streams multi-block response data through a bounded callback buffer;
- aborts timed-out commands so a failed request does not intentionally leave a command in flight.

The HTTP wrapper uses reusable firmware header/body handles and raw exchanges. Binary request bodies are split into 893-byte payload chunks (three bytes are reserved for target, command and body handle), while response bodies can be consumed incrementally. This is important for the bounded editor document and avoids requiring a second document-sized network buffer.

HTTP handles survive a C64 reset in the Ultimate firmware, so the publishing session issues HTTP FREE ALL before allocating its first handles.

The first standalone publisher operation is login. It lets the firmware build a URL-encoded `password` body, accepts the raw JSON response into a small bounded scratch buffer, stores only the returned bearer token and expiry, and frees request handles on success or failure. The normal site password is never retained in the session structure.

The client keeps one active publishing destination in a fixed-buffer profile. The profile stores only backend type, normalized base URL and the explicit insecure-LAN opt-in; all Standalone and WordPress endpoint URLs are constructed centrally with bounded output buffers. UI code therefore never assembles backend paths or query strings itself.

A separate small publishing-state structure tracks connection state and the current remote identity. For Standalone the identity is the canonical slug; for WordPress it is the numeric post ID. This lets the future UI distinguish create/update/delete safely. In particular, changing a Standalone slug after opening a remote document no longer qualifies as an in-place update, preventing an accidental duplicate file from masquerading as a rename.

The client document representation owns only bounded metadata and a pointer to the existing gap buffer. Its wire serializer emits metadata and body incrementally, so publishing a document does not require a second document-sized body allocation. Standalone Save uses that serializer with the compact `ack=1` API response; Delete uses the firmware's URL-encoded body builder. Both reuse the same bearer-auth and transport-safety path as login.

Standalone List uses the HTTP target's parsed response-body object and queries one result at a time. Results are delivered through a callback, so the client can render or page a large list without storing the whole array in C64 RAM. Open uses the raw `application/x-see64you` response and a streamed wire parser that writes body bytes directly into the existing gap buffer. Invalid, oversized or interrupted opens clear the partial destination rather than exposing truncated content as a valid document.

### C64U transport security

Do not infer TLS support from the HTTP target accepting an `https://` resource string. This was rechecked against current upstream `master` during development: `HttpHeaderSlot` recognises `https://`, sets port 443 and records a `secure` flag, but `HttpTarget::cmd_exchange` still passes only host/port to `HttpRequest::connect_to_server`. That function opens a normal `AF_INET/SOCK_STREAM` socket and `send_request` writes the HTTP request bytes directly. No TLS handshake is present in that path.

Accordingly:
- See64You rejects `https://` direct-login attempts as unsupported instead of silently sending plaintext to port 443;
- plaintext `http://` or bare-host login is disabled unless the caller explicitly opts into insecure transport;
- insecure direct login is intended only for controlled trusted-LAN testing or the local compatibility relay;
- future direct HTTPS support must be verified on target hardware before this safety gate is relaxed.

### Trusted-LAN HTTPS relay

The optional `relay/` service provides the secure remote path without making the C64 client a TLS implementation. The C64U sends HTTP to a relay on a controlled LAN. Every relay request may carry a separate 32-byte random relay key in `X-See64You-Relay-Key`; the site bearer token or WordPress Authorization header remains independent. The relay then forwards only approved paths to one configured HTTPS origin using the host Python TLS stack with certificate verification enabled.

The relay is deliberately constrained rather than acting as a general proxy:
- the upstream origin is fixed and must use HTTPS;
- redirects are not followed;
- optional client IP/CIDR restrictions can limit the LAN source;
- only configured endpoint prefixes are exposed;
- relay authentication, request/response size limits and header filtering are enforced;
- relay credentials, Host and Cookie headers are never forwarded upstream.

This protects the internet-facing leg, but the C64U-to-relay leg remains plaintext. It therefore belongs only on a trusted LAN, not public Wi-Fi or another untrusted local network.

Target hardware still needs to validate the exact UCI behaviour and keyboard/network integration. No transport is required for local document editing.

## Local C64U storage

Local New/Open/Save/Save As use the Ultimate-DOS UCI target rather than depending on a mounted disk image. Open streams the canonical See64You wire representation directly into the existing document buffer. A failed parse or interrupted read clears the partial destination, so truncated source cannot appear to be a successfully opened document.

Reads use the shared UCI executor, which acknowledges the Ultimate's streamed response blocks. Saves serialize incrementally to a temporary file, close it, then replace the destination through delete/rename operations. A failed write removes the temporary file without truncating the existing destination; a failed final rename leaves the complete temporary file recoverable. Writes are split into bounded payloads by the Ultimate-DOS layer and do not allocate a second document-sized buffer. Save As checks for an existing destination before asking the user to overwrite it.

New, Open and Exit protect dirty documents with an explicit confirmation. A new document derives default title and slug metadata from its first Save As filename. The capability probe reports Ultimate-DOS separately from the HTTP target so File commands degrade cleanly when direct filesystem access is unavailable. Directory browsing remains deferred until its per-entry packet-boundary behavior is handled without flattening entries in the generic UCI stream.

## Standalone web backend

The standalone site:
- requires PHP but no database;
- stores pages under `pages/`;
- stores posts under `posts/`;
- stores canonical See64You document files rather than generated HTML;
- renders HTML on request;
- exposes an authenticated API for the C64 client.

Public rendering is intentionally server-side and JavaScript-free.

## WordPress backend

The WordPress plugin exposes a deliberately small document API. WordPress remains responsible for accounts, permissions and normal REST authentication. See64You source is stored in post metadata while generated HTML is stored as the post content.

The C64 client uses a WordPress Application Password rather than the user's normal account password. Configuration converts `username:application-password` directly into a bounded Basic Authorization header without retaining a separate plaintext credential pair. The plugin's authenticated `/connection` endpoint is used as the initial capability/permission test. Remote WordPress access should use the trusted-LAN relay until direct C64U TLS is verified.

WordPress document operations deliberately avoid returning an entire source body as one structured string. Lists are queried one item at a time. Open returns source as base64 strings representing 180 raw source bytes each; the client validates the advertised chunk count/length and decodes directly into the existing gap buffer. Create/update use the same streamed `application/x-see64you` wire representation as the standalone backend, while compact acknowledgements return only bounded identifiers/status. Updating a published document with status `draft` is the unpublish operation.

The adapter filters list results through WordPress's per-post `edit_post` capability rather than relying only on broad post-type capabilities. Managed C64 lists also omit documents that have become incompatible with current C64 metadata/body limits. Canonical slugs are never silently changed for WordPress uniqueness: a collision is returned to the editor instead.

## Security boundaries

The C64 client cannot make an untrusted server safe. Backends therefore validate document type, slug and authentication independently.

Standalone-site login returns a short-lived signed token. Ordinary web clients should use HTTPS. On the C64U path, credentials or bearer tokens must not cross an untrusted network until TLS is provided by verified firmware or a trusted relay.
