# Architecture

See64You separates editing, local storage and publishing so the editor remains useful offline and backend-specific code does not own editor state.

## Runtime components

```
C64/C64U editor
    |
    | canonical source document
    v
publisher interface
   /              \
Standalone PHP    WordPress adapter
```

The conventional C64 build is a single editor runtime. The C64U package keeps the memory-heavy application split across:

- `SEE64YOU-C64U.PRG` / `S64EDIT.PRG` — editor;
- `S64PUB.PRG` — publisher;
- `S64CFG.PRG` — publishing settings.

The split is intentional: each program keeps an independent conventional-RAM budget. REU handoff preserves the document, editor/history metadata and publishing settings across transitions. Production transitions still use the temporary software-IEC chain. The Ultimate-DOS replacement reserves `$C800-$CBFF` for replacement-safe resident loading; the existing 1 KiB headroom gate already guarantees application BSS ends below that reservation and the cc65 software stack starts at `$CC00`. C64U-only preflight/file-metadata code remains isolated from the conventional C64 link. A dedicated resident-loader diagnostic gates replacement transfer and cc65 startup/return before the loader is wired into normal transitions.

## Canonical document

The editable source is metadata plus body text:

- type: `page` or `post`;
- slug;
- title;
- status: `draft` or `published`;
- date;
- order;
- body source.

Generated HTML is disposable. Backends retain enough canonical source to reopen and edit exactly.

The `SEE64YOU/1` wire representation is streamed so local saves, network publishing and remote opens do not require a second document-sized conventional-RAM buffer.

## Editor/storage model

The editor uses a gap-buffer interface.

The conventional-C64 build keeps a bounded document, clipboard and undo transaction in normal RAM. The C64U build uses REU-backed storage for a substantially larger document, clipboard and 16-level undo/redo history while preserving the same editor interfaces. CI enforces conventional-RAM headroom for both builds.

REU ownership is defined centrally in `client/src/platform/reu_layout.h`. Capacity probing must preserve occupied REU bytes because publisher/settings companions can start after document state already exists.

## Local C64U files

Local New/Open/Save/Save As use Ultimate-DOS. Canonical source is streamed directly between storage and the editor buffer.

Saves are transactional:

1. remove a stale temporary file;
2. create/write/close a new temporary file;
3. replace the destination through delete/rename.

A failed write does not intentionally truncate the previous destination. Save As checks for an existing file before overwrite.

Publishing settings use the same transactional design in `S64CFG.PRG`. The helper boundary keeps DOS/settings UI out of the memory-tight publisher.

## Publishing transports

Publisher/editor code consumes narrow operations and does not touch UCI registers directly.

### Direct Standalone transport

The active stock-firmware path uses Ultimate Network Target `$03` for DNS-backed TCP and raw HTTP. Protected requests use a dedicated 256-bit direct key, a one-time server challenge and HMAC-SHA256 covering protocol version, challenge, HTTP method, exact request URI and request-body SHA-256.

This provides authentication, integrity and replay protection without sending the normal site password or a reusable bearer token over the C64U connection. It does **not** provide confidentiality: source bodies travel as plaintext HTTP and require explicit user opt-in.

SHA/HMAC may temporarily use C64U turbo for CPU work, with the prior turbo state restored before REU/network I/O.

### HTTP Target compatibility path

The bounded UCI HTTP Target `$06` implementation remains behind the publisher interface for compatible/newer firmware. Stock C64 Ultimate firmware 1.1 does not expose that target, so it is not the active publisher path.

### Trusted-LAN relay

The optional relay accepts authenticated HTTP from a controlled LAN and forwards only approved paths to one fixed HTTPS origin with normal certificate verification. It is a compatibility path for HTTPS-required destinations and WordPress, not a general proxy.

## Publisher state

The publisher keeps remote-binding state separate from the document. A Standalone binding is identified by document type and canonical slug; WordPress uses the numeric post ID. This prevents a changed slug from silently being treated as an update to the previously opened remote document.

Remote source is streamed directly into the existing document buffer. Invalid, oversized or interrupted opens do not expose partial source as a successful document.

## Standalone backend

The standalone site:

- requires PHP but no database;
- stores ordinary source files under `pages/` and `posts/`;
- renders HTML server-side;
- exposes authenticated page/post CRUD, settings, backup and draft-preview operations;
- keeps the public site JavaScript-free.

See `PROTOCOL.md` and `SITE.md`.

## WordPress backend

The WordPress plugin maps the same document semantics onto WordPress REST operations. It uses WordPress-native permissions and Application Passwords, stores See64You source in post metadata and treats generated HTML as derived output.

WordPress support remains isolated from the editor and Standalone implementation. Service/hardware validation is tracked in `todo.md`.

## Security boundaries

The server validates authentication, type, slug and operation independently of the client.

- Normal browser/API bearer-token use requires HTTPS.
- Direct firmware-1.1 publishing uses the dedicated challenge/HMAC protocol and explicit plaintext-HTTP consent.
- The direct transport does not claim source confidentiality.
- The relay is trusted-LAN-only and never becomes an open proxy.
- HTTPS support must never be inferred from accepting an `https://` string; it must be demonstrated by an actual TLS implementation.

Current unresolved engineering and validation work belongs in `todo.md`, not this architecture document.
