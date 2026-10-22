# SEE64YOU

**Create and publish 8TML pages from the Commodore 64 Ultimate, and browse them from the C64U too.**

## Format direction (20 October 2026)

SEE64YOU is moving to the external **8TML 1 draft** for all new documents (`.8tml`), with optional `.8ss` stylesheets. The legacy `.s64` document format is retired; no compatibility layer or conversion is planned. The user-supplied external specification and validator are our reference; they are not bundled until licensing is clarified.

**Editor persistence is implemented on this development branch.** The maintained C64U Editor creates, saves and reopens ASCII/LF `.8tml` source using the existing disk/recovery path. The complete header is editable; new documents start below a minimal header. Unknown metadata and markup survive unchanged, except CR/CRLF input is normalized to LF. Save As adds `.8tml` to a basename and refuses other explicit extensions. Invalid headers or unsupported bytes are rejected before writing.

The independent Publisher now loads `.8tml`, streams authenticated JSON source to the PHP site, opens remote source and saves downloads to a new `.8tml` file. Source and publishing identity remain separate; see the [source service contract](docs/PROTOCOL.md). Host integration against the actual PHP service and native PETSCII/VICE checks pass; new Publisher hardware acceptance remains pending. Browser still uses its historical protocol, and public HTML renders the existing escaped subset rather than the full 8TML language. Historical alpha binaries remain `.s64` builds; build the migration sources for testing. REST-assisted real-C64U Editor/storage checks passed previously; physical-keyboard Open and cold restart remain unverified. Neither independent Editor nor Publisher imports the old document format.

See [migration and acceptance criteria](docs/8TML-MIGRATION.md) and the [authoritative work queue](todo.md).

Download the [v0.2.0-alpha.1 Editor preview prerelease](https://github.com/dannykeat-projects/SEE64YOU/releases/tag/v0.2.0-alpha.1). Its [installation guide](docs/releases/v0.2.0-alpha.1.md) covers offline editing, the separate SD/USB0 packages and remaining acceptance limits. The SD payload matches the restored real-C64U Editor/storage run of 22 October 2026 exactly; physical-keyboard and cold-power gates remain pending.

## Development Publisher workflow

Use the matching development PHP service and client package; do not replace historical alpha release assets. Configure the site and provision its dedicated direct key using the existing HTTPS pairing flow. In `S64CFG.PRG`, configure the site/key, explicitly enable **plain HTTP consent**, and save. HMAC authenticates requests but does not encrypt content. The development client accepts an optional decimal port from 1 to 65535, default 80, such as `http://192.168.0.250:8080/`. The published `v0.2.0-alpha.1` retains its port-80-only limit. Settings verifies saved configuration; Publisher tests the actual connection.

Create/save source in Editor, then launch `S64PUB.PRG` independently. `L` loads the `.8tml` file; `2` tests the key; `T` selects page/post; `4` chooses draft/published; `3` saves and asks for a valid remote slug on the first local upload. Optional title and unknown metadata stay in the source. `6`/`7` open a remote page/post with its publication state; `D` saves that source to a **new** local filename, refusing overwrite. Service date/order survive source updates. Editor remains useful offline and independently reopens the downloaded file.

## USB-only installations

Choose the storage mount when building the complete package. For example:

```sh
make -B -C client STORAGE_ROOT=/Usb0/ independent-package
```

Replace `/Usb0/` with the actual USB mount shown by your Ultimate browser, including the final slash. Copy **all eleven files** from `client/build/independent-package/SEE64YOU/` into that device's `SEE64YOU` folder. `-B` rebuilds every program and both manifests with the same root. The default is `/sd/`; there is no automatic device search or cross-device fallback. Startup, document/configuration storage, asset loading and recovery journals all use the selected root. USB-root host/build checks pass. The available drive mounts as `/USB1/`; its hardware test stopped during FTP staging because a program read back with different bytes, before execution. Cleanup and SD preservation passed; USB Editor acceptance remains pending. The published USB0 archive requires a matching `/Usb0/` mount; rebuild for other mount names.

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

Visit `http://127.0.0.1:8080/` for a host-only demonstration and `setup.php` for one-time configuration. The development Publisher accepts explicit ports; the published alpha still requires port 80. This command binds loopback; a C64U needs the server listening on its reachable LAN address. See [local validation](docs/LOCAL_TESTING.md) for the isolated native-code/PHP integration test.
