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
docs/         Architecture, editor, protocol and deployment notes
tools/        Local validation/development helpers
```

## Current status

The project is in its first implementation phase. The repository includes the initial editor core, flat-file web backend and WordPress integration scaffold. See `todo.md` for the authoritative work queue.

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

Output:

```
client/build/see64you.prg
```

## Running the standalone site locally

Requires PHP 8.1+.

```sh
php -S 127.0.0.1:8080 -t site
```

Then open `http://127.0.0.1:8080/`.

For publishing/API access, create an empty `site/ENABLE_SETUP` marker, then open `http://127.0.0.1:8080/setup.php` and complete the one-time setup form. Setup is disabled without that server-side marker, and successful setup removes it automatically. The form creates `config.php` with a password hash and random token-signing secret. Manual setup using `config.example.php` remains available.

## Philosophy

See64You should feel like a useful editor first and a retro demonstration second. Features must remain understandable on a 40-column display, keyboard-first, recoverable from plain files, and simple enough to run on ordinary PHP hosting.
