# SEE64YOU

**Create and publish 8TML pages from the Commodore 64 Ultimate, and browse them from the C64U too.**

## Format direction (20 October 2026)

SEE64YOU is moving to the external **8TML 1 draft** for all new documents (`.8tml`), with optional `.8ss` stylesheets. The legacy `.s64` document format is retired; no compatibility layer or conversion is planned. The user-supplied external specification and validator are our reference; they are not bundled until licensing is clarified.

**Editor persistence is implemented on this development branch.** The maintained C64U Editor creates, saves and reopens ASCII/LF `.8tml` source using the existing disk/recovery path. The complete header is editable; new documents start below a minimal header. Unknown metadata and markup survive unchanged, except CR/CRLF input is normalized to LF. Save As adds `.8tml` to a basename and refuses other explicit extensions. Invalid headers or unsupported bytes are rejected before writing.

The PHP site now accepts authenticated 8TML source with separate publishing metadata and exports identical `.8tml` bytes; see the [source service contract](docs/PROTOCOL.md). Native Publisher and Browser still use their historical protocols and must be adapted next. Public HTML currently renders the existing escaped subset, not the full 8TML language. The committed alpha binaries are historical `.s64` builds; build the migration sources for testing. REST-assisted real-C64U Editor/storage checks pass, with zero errors/warnings from the supplied validator. Physical-keyboard Open and cold restart remain unverified. There is no old-format importer or compatibility fallback in the independent Editor.

See [migration and acceptance criteria](docs/8TML-MIGRATION.md) and the [authoritative work queue](todo.md).

## USB-only installations

Choose the storage mount when building the complete package. For example:

```sh
make -B -C client STORAGE_ROOT=/Usb0/ independent-package
```

Replace `/Usb0/` with the actual USB mount shown by your Ultimate browser, including the final slash. Copy **all eleven files** from `client/build/independent-package/SEE64YOU/` into that device's `SEE64YOU` folder. `-B` rebuilds every program and both manifests with the same root. The default is `/sd/`; there is no automatic device search or cross-device fallback. Startup, document/configuration storage, asset loading and recovery journals all use the selected root. USB-root host/build checks pass; a real USB-only hardware run remains pending.

## Repository layout

- `client/`: cc65 editor and C64U client code
- `site/`: standalone flat-file PHP site and publishing API
- `wordpress/`: historical WordPress integration, subject to MVP scope review
- `docs/`: project architecture and migration guidance
- `tools/`: local validation and development helpers

The intended MVP preserves separate Editor, Publisher and Settings programs, plus an independent 8TML browser. Source files remain unchanged when the web server generates HTML for modern browsers. Publishing transport and authentication are SEE64YOU services, not extensions to the 8TML document standard.

## Running the existing standalone site locally

Requires PHP 8.1+:

```sh
php -S 127.0.0.1:8080 -t site
```

Visit `http://127.0.0.1:8080/`. For the existing API, visit `http://127.0.0.1:8080/setup.php` for one-time configuration. **This is the existing backend, not yet an 8TML interoperability demonstration.**
