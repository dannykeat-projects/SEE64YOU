# SEE64YOU — current MVP architecture

## Current Editor migration

The independent Editor (`S64_INDEPENDENT_EDITOR`) now retains the complete 8TML 1 source in its existing REU gap buffer, including the editable header. New source has a minimal version/title header and starts the cursor in the body. Disk reads normalize CR/CRLF to LF, convert ASCII to native characters and reject unsupported bytes; disk writes preflight the complete header/source, convert back to ASCII and retain the existing temporary-file transaction and recovery snapshot. Header keys and body markup are never reconstructed or rendered during persistence. The header occupies part of the existing 61,440-byte capacity.

Save As defaults to `.8tml` and rejects other explicit extensions. Open identifies source by its header and permits retained `.tmp` files for recovery. No `.s64` parser runs in this Editor build. The shared legacy metadata structure remains for unmigrated Publisher/diagnostics; it is not written into 8TML. Native Publisher/Browser conversion and full site rendering remain pending; the flow below describes the historical alpha, not interoperability with this Editor.

The PHP source service now independently accepts JSON `raw8tml` plus service metadata under the existing authentication. One atomic guarded JSON `.8tml.php` snapshot stores exact source and metadata; title/body are derived rather than rewriting source. A directory lock prevents simultaneous legacy/new-format saves claiming the same identity. Authenticated `format=8tml` exports canonical source bytes. Native Publisher/Browser remain unmigrated, and current HTML rendering is the safe existing subset. See [PROTOCOL.md](PROTOCOL.md) for the new contract; the historical data flow below is not a completed 8TML publishing pipeline.

> **Historical alpha reference only, not future 8TML requirements.** The released version described here used `.s64`/SEE64YOU/1. The next build supports only `.8tml`, with no old-file migration, importer or compatibility support. Reuse applicable hardware safety rules when replacing code; see [8TML direction](8TML-ROADMAP.md).


**Authoritative release architecture.** SEE64YOU is a C64 Ultimate-only application for creating simple `.s64` text documents, publishing them to independently hosted PHP websites, and optionally browsing public SEE64YOU sites from the C64U. Historic cross-program managed/persistent runtime designs are retired; Git history preserves them.

## Runtime and data flow

```text
           /sd/SEE64YOU/*.s64
            ^             |
            |             v
    Editor (SEE64YOU)   Publisher (S64PUB) <----> PHP site/API
            |             ^                    |          |
            |             |                    |          +--> public HTML
            +---------- BASIC                  +--> SEE64YOU-BROWSE/1 or /2
                                                    ^
  Settings (S64CFG) --> /sd/SEE64YOU/SEE64YOU.CFG    |
                                             Browser (S64WEB)
```

- **Editor:** established Launcher→private Editor overlays; edit/format, save and reopen canonical `.s64`. The Editor must work without networking and leave a responsive BASIC environment.
- **Publisher:** a separate program; reads a saved source file and configuration, authenticates direct requests, publishes/updates; can download remote canonical source to a *new* file. Its UI cannot assume Editor session state.
- **Settings:** independent first-run wizard and disk configuration owner; settings are verified by readback before reported as saved. Manual, verified migration applies to legacy journals and configuration where already implemented.
- **Browser:** independently launched, optional and read-only; selects/enters public `http://` URLs, follows links and pages through output. No publishing credentials, documents or Editor state are transferred into it.
- **Standalone website:** PHP 8.1+, flat-file storage, ordinary HTML for modern browsers, generated compact public records for C64. No database or JavaScript required for public reading.

Each program launches fresh and returns to BASIC. `.s64` and `.CFG` are the **only shared persistent interfaces** between user programs. Internal overlays and REU storage *within* Editor are permitted; whole-program handoffs and shared persistent active application state are not MVP requirements.

## Canonical content and browser representation

- The editable truth is `SEE64YOU/1` canonical source: title, slug, page/post type, draft/published state and source body. Generated HTML is disposable; reopen remote source without HTML round-tripping. See [PROTOCOL.md](PROTOCOL.md) and [MARKUP.md](MARKUP.md).
- C64 browsing uses server-rendered `SEE64YOU-BROWSE/1` (`T` and `L` text/link records). A colour-aware client requests v2: mandatory `C` palette record followed by the same text/link records. v1 clients retain v1 output, and v2 clients accept v1 fallback. These are existing compatibility contracts, not invitations to redesign the wire.
- The browser is a bounded network parser, not an HTML, CSS, JavaScript, PETSCII executable-markup or HTTPS engine. Sites can be changed through their normal browser-admin interface; C64U editing of site appearance is deferred.
- Public content can be shown both ways because the *server* translates canonical source into HTML or the C64 records. Do not duplicate a second authored copy for the C64.

## Platform, memory and files

- Target: C64 Ultimate firmware-1.1 hardware baseline, UCI, Ultimate-DOS, SD-backed `/sd/SEE64YOU/` and a required 16 MiB REU. Offline Editor doesn't require networking. Direct publishing and browsing use Network Target `$03`; HTTP Target `$06` is not available on the tested firmware.
- Existing Launcher→Editor bootstrap, module validation, protection of BASIC return context, state capture/restore, documented REU/overlay boundaries and 1 KiB Publisher/Launcher memory guards are preserved until a measured and hardware-tested replacement is available.
- Current Launcher still checks/preloads unused Publisher/Settings module images to preserve its proven bootstrap. Removal is a **candidate future cleanup**, not approval to change module manifests or bootstrap without traces and a real-C64U acceptance gate.
- Bare document names, autosave, journals, temporary storage probes, binaries, configuration and downloads live under `/sd/SEE64YOU/`. New Editor Save As and Publisher download names receive `.s64` when their final path component has no extension; explicit extensions and paths remain intact. Ordinary Save retains the opened source filename, including legacy extensionless names. Explicit `/sd/NAME.S64` continues to reference older SD-root documents; no automatic fallback hides a corrupt or missing folder file.
- Editor documents/autosave use DOS target `$02`; Launcher/journal uses `$01`. Handles and directory contexts must remain isolated. Save As/new downloads must refuse unintended overwrite; completed `.tmp` files remain recoverable after a failed replacement.
- `S64CFG.PRG` owns explicit legacy-journal migration. Root journal presence blocks Editor startup pending verified migration; root recovery sources and unrelated SD files are not silently deleted. Protect original bytes, backups and interrupted recovery.

Detailed machine ownership, journal, migration, tempfile, resource limits and interruption rules stay in [C64U-RUNTIME.md](C64U-RUNTIME.md); they remain binding wherever current shipping code relies on them.

## Network and trust

- Pair direct credentials from the website over **HTTPS**, not the C64U's plaintext transport; do not send the normal site password or bearer token over direct HTTP.
- Firmware-1.1 Publisher uses a separate 256-bit key, server challenge and HMAC-SHA256 to bind method, URI and body hash. This authenticates and protects request integrity/replay, **not confidentiality**. Explicit consent to plaintext document content remains mandatory. Never silently downgrade HTTPS.
- Public browser GETs never transmit publishing keys, cached credentials, auth headers or arbitrary HTML. Reject unsupported schemes and malformed/oversized records; close sockets on success, failure and cancellation.
- Validate slugs/types and permissions server-side. Escape generated HTML. File/configuration writes preserve integrity/readback and failed-recovery safeguards. Preserve prior turbo, IRQ, drive/user machine settings on exit to the extent the supported interface permits.

## Status and controlled simplification

Hardware receipts demonstrate bounded independent edit→publish→download→edit→update, Settings persistence, app-folder migration and browser navigation. They do **not** certify a complete unassisted first-user install, final browser colour changes or every physical interruption. See [LOCAL_TESTING.md](LOCAL_TESTING.md) for executable gates and [todo.md](../todo.md) for the **only** active work.

Remove code only when its callers are traced, a smaller implementation is measured, the new package passes all applicable tests, and hardware proves the changed bootstrap or I/O path. No managed-runtime redesign, WordPress expansion, general HTML browsing, large-document refactor or new REU allocator is part of the MVP.
