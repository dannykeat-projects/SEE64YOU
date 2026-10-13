# SEE64YOU

**Write on a Commodore 64 Ultimate. Publish to the web. Browse compatible sites on the C64U.**

SEE64YOU is a C64 Ultimate-only text editor, standalone publisher and optional public-site reader. It publishes simple, independently hosted websites viewable in modern browsers without C64-specific software. The C64U reader displays a compact representation of the same public content.

## MVP: four independent programs

| Launch | Purpose |
| --- | --- |
| `SEE64YOU.PRG` | Editor: write/save/reopen `.s64` documents offline; loads internal Editor assets. |
| `S64PUB.PRG` | Publisher: load `.s64`, authenticate, publish/update; download remote source into a **new** `.s64`. |
| `S64CFG.PRG` | Settings: first-run wizard, stored configuration and migration controls. |
| `S64WEB.PRG` | Optional read-only browser for public SEE64YOU websites, selected links and C64 site colours. |

Each is launched separately and returns to BASIC. Programs communicate through ordinary files rather than shared live session state. Do not directly launch internal `S64EDIT.PRG`.

## What is required

- A Commodore 64 Ultimate with UCI enabled, Ultimate-DOS, a **16 MiB REU**, accessible SD storage and networking for publication/browsing. The validated baseline is firmware 1.1 / UCI Network Target `$03`.
- A hosting account supporting **PHP 8.1+**, or an equivalent reachable disposable PHP instance for testing. The standalone website uses ordinary files, not a database.
- The complete, matching eleven-file client package; install it together in `/sd/SEE64YOU/`. Back up an existing installation before replacement.
- A modern browser to set up and pair the website initially. Editing is useful offline; publishing requires network access.

## First website: shortest path

1. Build or obtain the **version-matched client package** and standalone PHP website package; place all eleven client files in `/sd/SEE64YOU/` and upload the site to your PHP host.
2. On a fresh hosted site, create the documented setup-enable marker and open `setup.php`; then follow the server-rendered `start.php` walkthrough.
3. Use `pair.php` over **HTTPS** to obtain the dedicated direct publishing key; import it using `S64CFG.PRG` and save/verify Settings. Don't leave plaintext key-import files on your SD card.
4. Launch `SEE64YOU.PRG`, create/save a `.s64` document and exit. Launch `S64PUB.PRG` independently, load the file, confirm page/post and published status, then publish.
5. Verify the public page in an ordinary browser. Optionally launch `S64WEB.PRG`, enter the site's `http://` URL and follow links. For later editing, Publisher can download remote source to a **new** filename for reopening in Editor.

**Exact keys, screens, recovery and fresh-install precautions:** [First-page checklist](docs/FIRST-PAGE.md). Website `start.php` provides the interactive, no-JavaScript walkthrough.

## Important limitations

- The C64 browser reads the **SEE64YOU public text/link protocol**, not arbitrary HTML, CSS or JavaScript. It rejects HTTPS rather than downgrading it.
- Firmware-1.1 direct publication uses HTTP with challenge/HMAC authentication. Requests have authenticity, integrity and replay protection, **not confidentiality**: document contents can be observed on the network. The normal website password is not sent via direct C64U HTTP. Explicit consent is required; configure an appropriate trusted relay if confidentiality is required.
- The public site can customise its layout/fonts and choose from the 16 C64 palette colours. Only the website edits theme colours; C64U Editor-side colour editing is outside the MVP.
- This is a **release candidate, not a hardware-accepted public beta**. An unassisted first-time setup and the final cross-site C64 colour test remain outstanding. USB keyboard and uncontrolled power/SD interruption acceptance are deferred and must not be implied to have passed.

## Build and validate

With cc65 installed, from the repository root:

```sh
make -C client independent-package
```

The matching binaries are staged in `client/build/independent-package/SEE64YOU/`. Do not mix them with older managed-runtime packages. The standalone website is built with:

```sh
php tools/package-site.php
```

Local PHP development (PHP 8.1+):

```sh
php -S 127.0.0.1:8080 -t site
```

See [the validation runbook](docs/LOCAL_TESTING.md) for host/PHP, VICE and real-C64U acceptance steps. Passing CI is not a substitute for real-hardware acceptance.

## Documentation and project status

- [`todo.md`](todo.md): **only** active MVP release checklist and stop rules.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): current program/data boundaries.
- [`docs/C64U-RUNTIME.md`](docs/C64U-RUNTIME.md): hardware, resource, recovery and restoration contracts.
- [`docs/PROTOCOL.md`](docs/PROTOCOL.md) and [`docs/MARKUP.md`](docs/MARKUP.md): established wire/authentication and editable-source formats.
- [`docs/SETUP-UX.md`](docs/SETUP-UX.md): first-use wizard behaviour.
- [`docs/ENGINEERING-REFERENCES.md`](docs/ENGINEERING-REFERENCES.md): external implementation evidence and adoption/licence rules.

Broader features are **shelved**, not release commitments. No more managed cross-program runtime, wholesale rewrite, Compunet/CML integration or arbitrary HTML browsing is planned without measurable evidence or user feedback. If first-time publication cannot be made reliable with bounded fixes, reassess whether to archive the project instead of starting another architecture exercise.
