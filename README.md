# SEE64YOU

SEE64YOU is an offline source editor for C64 Ultimate, with a small companion for publishing to a compatible 8TML server.

The Editor handles portable `.8tml` documents, literal `.8ss` stylesheets and general ASCII text, including `.txt` and `.md`, through the same editing engine.

The Editor does not require an account or internet connection, and you can also just use it as a standard text editor.

It has many functions you would expect in a modern Markdown editor, with multiple undo/redo, copy/cut/paste with shortcuts, dropdown menus, formatting shortcuts and menus, text selection with mouse.

## What works

- **Editor:** New offers 8TML, 8SS and plain text. Open uses the exact filename; Save As defaults to `.8tml`, `.8ss` or `.txt` and preserves explicit safe suffixes. Includes cursor/selection, search, 8TML formatting shortcuts, undo/redo and format-specific snapshot recovery. Source and unknown constructs are preserved; files use ASCII with LF endings, with a 61,440-byte source limit. Empty text files are valid.
- **Publisher (experimental):** separately load local `.8tml`, `.8ss` or `.txt`, choose a remote path and publish or explicitly replace through standard API 1. Settings imports the standard endpoint/ID/key. Markdown and other text suffixes remain local; use Save As `.txt` to publish their literal text. The 26 October 2026 API draft remains subject to interoperability and hardware acceptance.

For reading 8TML on an 8-bit machine, use a compatible browser from the 8TML/T8WN ecosystem. SEE64YOU does not aim to maintain its own 8TML browser. The active package does not include a browser.

## Get started

See [installation and first document](docs/INSTALL.md) for prerequisites, copying the complete matching package, Editor controls and optional publishing setup. The latest [GitHub Releases](https://github.com/dannykeat-projects/SEE64YOU/releases) include checksums; read the release's stated acceptance limits before testing. Back up any files on the target storage device.

The Editor saves **only when you ask it to** (Ctrl+S / File → Save or Save As). It does not autosave. Existing recovery snapshots may be reopened, but unsaved edits are not continuously protected.

**Security:** direct C64U publishing over HTTP uses a site-specific challenge/HMAC key for authentication and integrity; it **does not encrypt document content**. Pair the key over HTTPS and explicitly approve unencrypted direct publishing only for suitable content/networks. The Editor works without publishing.

## Build and contribute

The supported client build is:

~~~sh
make -C client independent-package
~~~

The default storage root is `/sd/`. Install the whole package with the four PRGs in `SEE64YOU/` and verified `.BIN`/`.MNF` files in `SEE64YOU/SYSTEM/`. To build for an actual USB mount, rebuild the **whole** package with the corresponding `STORAGE_ROOT` (including trailing slash); there is no automatic fallback. Do not mix files between builds. Current main stores ordinary source in `SEE64YOU/docs/`; released alpha.6 uses the root. Settings and recovery stay at the root. See installation before upgrading.

See [development and validation](docs/LOCAL_TESTING.md), [architecture](docs/ARCHITECTURE.md), [product scope](docs/PRODUCT.md), and the active [work queue](todo.md). Current program names and package layout are documented in the installation guide.

This is experimental software. Cold-start, USB-only, rebuilt Publisher-on-hardware and independent standard-server acceptance remain required; do not use it as the sole copy of important work.
