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

The first implementation prioritises correctness and a runnable shell. Incremental rendering and selection follow after baseline VICE validation.

## Transport

Publishing operations depend on a narrow transport layer rather than direct UCI calls scattered through UI code. The C64U implementation uses the firmware 3.15+ UCI HTTP target ($06).

The low-level UCI executor:
- enforces the documented 896-byte command limit;
- writes commands as small caller-owned parts, avoiding a second 896-byte staging buffer;
- drains response/status queues and acknowledges every returned block;
- streams multi-block response data through a bounded callback buffer;
- aborts timed-out commands so a failed request does not intentionally leave a command in flight.

The HTTP wrapper uses reusable firmware header/body handles and raw exchanges. Binary request bodies are split into 893-byte payload chunks (three bytes are reserved for target, command and body handle), while response bodies can be consumed incrementally. This is important for an 8 KiB editor document and avoids requiring a second document-sized network buffer.

HTTP handles survive a C64 reset in the Ultimate firmware, so the publishing session issues HTTP FREE ALL before allocating its first handles.

The first standalone publisher operation is login. It lets the firmware build a URL-encoded `password` body, accepts the raw JSON response into a small bounded scratch buffer, stores only the returned bearer token and expiry, and frees request handles on success or failure. The normal site password is never retained in the session structure.

### C64U transport security

Do not infer TLS support from the HTTP target accepting an `https://` resource string. In the upstream 1541 Ultimate source inspected during development (commit `ef9937a73c7db391d5434bfda08e96216d7099cc`), `HttpHeaderSlot` recognises `https://` and selects port 443, but `HttpTarget::cmd_exchange` passes the host/port to `HttpRequest::connect_to_server`, which opens a normal `AF_INET/SOCK_STREAM` socket and sends the HTTP request directly. No TLS handshake is performed in that path.

Accordingly:
- See64You rejects `https://` direct-login attempts as unsupported instead of silently sending plaintext to port 443;
- plaintext `http://` or bare-host login is disabled unless the caller explicitly opts into insecure transport;
- insecure direct login is intended only for controlled trusted-LAN testing or a local compatibility relay;
- secure remote publishing needs a trusted local relay that terminates HTTPS, or future C64U firmware whose TLS behaviour has been verified on target hardware.

Target hardware still needs to validate the exact UCI behaviour and keyboard/network integration. No transport is required for local document editing.

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

## Security boundaries

The C64 client cannot make an untrusted server safe. Backends therefore validate document type, slug and authentication independently.

Standalone-site login returns a short-lived signed token. Ordinary web clients should use HTTPS. On the C64U path, credentials or bearer tokens must not cross an untrusted network until TLS is provided by verified firmware or a trusted relay.
