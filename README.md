# SEE64YOU

**Write 8TML pages on a Commodore 64 Ultimate.** SEE64YOU is an offline-first source editor with a small companion for publishing to a compatible 8TML server.

Documents are portable `.8tml` text files. You can copy, back up, or host them independently. The Editor does not require an account or internet connection. Optional `.8ss` stylesheets are part of the wider 8TML ecosystem; stylesheet editing is not a dedicated SEE64YOU feature.

## What works

- **Editor:** create, open, edit and manually save 8TML 1 draft source on SD or a configured USB mount. Includes cursor/selection, search, formatting shortcuts, undo/redo and existing-snapshot recovery. The complete source and unknown markup are preserved; files use ASCII with LF endings, with a 61,440-byte source limit.
- **Publisher (experimental):** separately load local `.8tml`, choose a remote path and publish or explicitly replace through standard API 1. Settings imports the standard endpoint/ID/key. The 26 October 2026 API draft remains subject to interoperability and hardware acceptance.

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

The default storage root is `/sd/`. To build for an actual USB mount, rebuild the **whole** package with the corresponding `STORAGE_ROOT` (including trailing slash); there is no automatic fallback. Do not mix files between builds.

See [development and validation](docs/LOCAL_TESTING.md), [architecture](docs/ARCHITECTURE.md), [product scope](docs/PRODUCT.md), and the active [work queue](todo.md). Current program names and package layout are documented in the installation guide.

This is experimental software. Cold-start, USB-only, rebuilt Publisher-on-hardware and independent standard-server acceptance remain required; do not use it as the sole copy of important work.
