# See64You

**Write on a Commodore 64 Ultimate. Publish to the web.**

See64You is a Notepad-style text editor and lightweight publishing system for the Commodore 64 Ultimate (C64U). The editor supports conventional C64 operation and a larger C64U/REU path, while publishing keeps web complexity on the server.

## Goals

- A fast, predictable plain-text editor with familiar desktop editing behaviour.
- A small Markdown-like source format that remains the canonical editable document.
- Publish the same document to a tiny database-free PHP website or to WordPress through a See64You plugin.
- Public standalone sites that require no JavaScript and remain usable in very old browsers.
- Optional styling, including a C64-inspired theme.
- No HTML round-tripping: source text stays source text; HTML is generated for display.
- Keep the C64 client independent of any one publishing backend.

## Repository layout

```
client/       C64/C64U editor client (cc65)
site/         Standalone flat-file PHP site and publishing API
wordpress/    WordPress publishing plugin
relay/        Optional trusted-LAN HTTPS relay
docs/         Stable architecture, protocol, editor and validation documentation
tools/        Validation/development helpers
```

## Implemented baseline

The C64U application is split into editor, publisher and publishing-settings PRGs so each keeps an independent conventional-RAM budget while document/history/settings handoff lives in REU. The standalone site is usable end-to-end, and the stock C64 Ultimate firmware-1.1 publishing path uses Network Target `$03` with a dedicated direct key, one-time challenge/HMAC authentication and explicit plaintext-HTTP consent. Real hardware has authenticated, uploaded and reopened an exact 4,608-byte source document and has applied draft/published state changes.

The standalone publisher is the active target. The WordPress implementation and tests remain in the repository, but WordPress-specific hardware/service validation is intentionally deferred until the general C64U path is complete. Remaining work is tracked only in `todo.md`.

## Documentation map

- `todo.md` — authoritative live work queue and unresolved validation.
- `docs/ARCHITECTURE.md` — implemented component boundaries and data flow.
- `docs/PROTOCOL.md` — current publishing/wire/authentication contracts.
- `docs/UCI.md` — Ultimate/UCI transport and hardware behaviour.
- `docs/EDITOR.md` and `docs/INPUT.md` — editor and keyboard behaviour.
- `docs/LOCAL_TESTING.md` — current reproducible validation procedures only.
- `docs/ENGINEERING-REFERENCES.md` — external projects, licensing and adoption boundaries.

Historical investigations and completed troubleshooting are preserved by Git history rather than retained as active instructions.

## Design principle

The C64 client works with documents, not HTML:

```
Document -> Publisher interface -> Standalone PHP site
                             \-> WordPress plugin
```

A document has a title, body, type (page/post), status, slug and metadata. Publishing backends are responsible for storage and HTML rendering.

## Building the C64 client

Requires [cc65](https://cc65.github.io/).

```sh
make -C client
```

The conventional C64 output is:

```
client/build/see64you.prg
```

For the C64U build, `make -C client c64u publisher` produces:

```
client/build/SEE64YOU-C64U.PRG
client/build/S64EDIT.PRG
client/build/S64PUB.PRG
client/build/S64CFG.PRG
```

`SEE64YOU-C64U.PRG` is the user-facing entry point. The short aliases are deliberate CBM-DOS-safe internal chain names and are packaged together by CI.

## Running the standalone site locally

Requires PHP 8.1+.

```sh
php -S 127.0.0.1:8080 -t site
```

Create an empty `site/ENABLE_SETUP` marker, open `http://127.0.0.1:8080/setup.php`, and complete the one-time setup. Successful setup removes the marker and creates the local configuration.

To create a clean upload-ready package:

```sh
php tools/package-site.php
```

The default output is `dist/see64you-site/`. It contains runtime files and starter content, excludes tests and local `config.php`, and includes a fresh `ENABLE_SETUP` marker.

An authenticated backup is available from `api.php?action=backup`; it contains public settings and all page/post source, including drafts, but excludes authentication secrets.

## Remote publishing security

The active firmware-1.1 direct publisher uses raw TCP/HTTP through Network Target `$03`. It never sends the normal site password or a reusable bearer token over that connection. Instead it signs each protected request with a dedicated 256-bit direct key and a single-use server challenge. Direct mode is still plaintext HTTP, so source bodies are not confidential and the user must explicitly enable that transport.

For HTTPS-required destinations, WordPress, or environments where plaintext LAN/internet transport is unsuitable, the trusted-LAN relay remains the compatibility path. See `docs/PROTOCOL.md`, `docs/UCI.md` and `relay/README.md`.

## Philosophy

See64You should feel like a useful editor first and a retro demonstration second. Features must remain understandable on a 40-column display, keyboard-first, recoverable from ordinary files, and simple enough to run on ordinary PHP hosting.
