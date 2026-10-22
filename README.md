# SEE64YOU

**Write 8TML pages on a Commodore 64 Ultimate.** SEE64YOU is an offline-first, source-text editor with optional tools for publishing to your own website.

Documents are portable `.8tml` text files. You can copy, back up, or host them independently. The Editor does not require an account or internet connection. Optional `.8ss` stylesheets are part of the wider 8TML ecosystem; stylesheet editing is not a dedicated SEE64YOU feature.

## What works

- **Editor:** create, open, edit and manually save 8TML 1 draft source on SD or a configured USB mount. Includes cursor/selection, search, formatting shortcuts, undo/redo and existing-snapshot recovery. The complete source and unknown markup are preserved; files use ASCII with LF endings, with a 61,440-byte source limit.
- **Publisher (experimental):** separately load local `.8tml`, choose page/post and draft/published state, and send it to the bundled self-hosted PHP service. Remote-source download saves a new local file. Settings is a separate program.
- **Modern website:** the bundled PHP 8.1+ service stores exact source and serves HTML. Its renderer currently implements only a limited, safely escaped subset of 8TML. Full 8TML rendering and compatibility with external publishing services are **not yet verified**.

For reading 8TML on an 8-bit machine, use a compatible browser from the 8TML/T8WN ecosystem. SEE64YOU does not aim to maintain its own 8TML browser. The currently packaged `S64WEB.PRG` is not an 8TML browser.

## Get started

See [installation and first document](docs/INSTALL.md) for prerequisites, copying the complete matching package, Editor controls and optional website setup. The latest [GitHub Releases](https://github.com/dannykeat-projects/SEE64YOU/releases) include checksums; read the release's stated acceptance limits before testing. Back up any files on the target storage device.

The Editor saves **only when you ask it to** (Ctrl+S / File → Save or Save As). It does not autosave. Existing recovery snapshots may be reopened, but unsaved edits are not continuously protected.

**Security:** direct C64U publishing over HTTP uses a site-specific challenge/HMAC key for authentication and integrity; it **does not encrypt document content**. Pair the key over HTTPS and explicitly approve unencrypted direct publishing only for suitable content/networks. The Editor works without publishing.

## Build and contribute

The supported client build is:

~~~sh
make -C client independent-package
~~~

The default storage root is `/sd/`. To build for an actual USB mount, rebuild the **whole** package with the corresponding `STORAGE_ROOT` (including trailing slash); there is no automatic fallback. Do not mix files between builds.

Start the optional local PHP site with:

~~~sh
php -S 127.0.0.1:8080 -t site
~~~

This loopback address is for host testing, not a C64U-reachable service. See [development and validation](docs/LOCAL_TESTING.md), [architecture](docs/ARCHITECTURE.md), [product scope](docs/PRODUCT.md), and the active [work queue](todo.md). Current program names and package layout are documented in the installation guide.

This is experimental software. Physical-keyboard, cold-start, USB-only, Publisher-on-hardware, full renderer and external interoperability acceptance are outstanding; do not use it as the sole copy of important work.
