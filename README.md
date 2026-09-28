# See64You

**Write on a Commodore 64 Ultimate. Publish to the web.**

See64You is a Notepad-style text editor and lightweight publishing system for the Commodore 64 Ultimate (C64U). The editor is designed to work well with both a C64 keyboard and modern USB keyboards, while the publishing layer keeps web complexity on the server.

## Goals

- A fast, predictable plain-text editor with familiar desktop editing behaviour.
- A small Markdown-like source format that remains the canonical editable document.
- Publish the same document to:
  - a tiny database-free PHP website; or
  - WordPress through a See64You plugin.
- Public sites that require no JavaScript and remain usable in very old browsers.
- Optional styling, including a C64-inspired theme.
- No HTML round-tripping: source text stays source text; HTML is generated for display.
- Keep the C64 client independent of any one publishing backend.

## Repository layout

```
client/       C64/C64U editor client (cc65)
site/         Standalone flat-file PHP site and publishing API
wordpress/    WordPress publishing plugin
relay/        Optional trusted-LAN HTTPS relay for C64U publishing
docs/         Architecture, editor, protocol and deployment notes
tools/        Local validation/development helpers
```

## Current status

The editor, standalone flat-file backend, secure relay and C64U publishing modules are under active integration. The current C64U package uses separate editor, publisher and publishing-settings PRGs while presenting them as one application. The active publisher is Standalone-first and includes remote CRUD/list/open, C64 preview, four saved publishing profiles, persistent relay configuration, and body-only plain-text import/export. Canonical See64You source remains the editable `.s64` format. WordPress implementation/tests remain in the repository but WordPress-specific validation is deferred until the rest of the C64U path is working end-to-end. See `todo.md` for the authoritative work queue and `docs/LOCAL_TESTING.md` for validation order.

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

For the C64U build, `make -C client c64u publisher` produces the editor plus internal chain targets:

```
client/build/SEE64YOU-C64U.PRG
client/build/S64EDIT.PRG
client/build/S64PUB.PRG
client/build/S64CFG.PRG
```

`SEE64YOU-C64U.PRG` is the friendly entry point. The short aliases are deliberate CBM-DOS-safe internal chain names and are packaged together by CI. The integration workflow also publishes downloadable PRG artifacts for manual VICE/C64U testing.

## Running the standalone site locally

Requires PHP 8.1+.

```sh
php -S 127.0.0.1:8080 -t site
```

Then open `http://127.0.0.1:8080/`.

For publishing/API access, create an empty `site/ENABLE_SETUP` marker, then open `http://127.0.0.1:8080/setup.php` and complete the one-time setup form. Setup is disabled without that server-side marker, and successful setup removes it automatically. The form creates `config.php` with a password hash and random token-signing secret. Manual setup using `config.example.php` remains available.

Authenticated clients can request a short-lived signed preview link for a draft page or post through the standalone API. Draft previews remain hidden without the matching document-specific token and are served with no-store/noindex/no-referrer headers.

To create a clean upload-ready copy of the standalone site:

```sh
php tools/package-site.php
```

By default this rebuilds `dist/see64you-site/`. It includes the runtime files and starter content, excludes tests and any local `config.php`, and creates `ENABLE_SETUP` so the one-time browser setup is available immediately after upload. An alternate output directory can be supplied as the first argument.

An authenticated full-content backup can be downloaded from `api.php?action=backup`. The JSON backup contains public site settings and every page/post source document, including drafts, but deliberately excludes the password hash and token-signing secret. It is intended for safe archival/export rather than credential cloning.

## Secure remote publishing

The current C64U UCI HTTP path is not treated as verified TLS. For publishing to a remote HTTPS site, See64You includes an optional trusted-LAN relay in `relay/`. The C64U talks HTTP only to the relay on the local network; the relay authenticates the C64U request and forwards it to one fixed HTTPS upstream using normal certificate verification.

See `relay/README.md` for setup and security details.

## Philosophy

See64You should feel like a useful editor first and a retro demonstration second. Features must remain understandable on a 40-column display, keyboard-first, recoverable from plain files, and simple enough to run on ordinary PHP hosting.
