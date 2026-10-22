# C64 Ultimate runtime and storage

**Platform:** C64 Ultimate only, Command Interface enabled, 16 MiB REU, Ultimate-DOS `$01`/`$02`. Firmware 1.1 evidence supports UCI `$DF1C..$DF1F` and Network Target `$03`; HTTP Target `$06` was unavailable. Probe capabilities instead of inferring them from firmware strings.

## Program and package boundaries

Editor (`SEE64YOU.PRG` through its launcher/internal overlays), Publisher (`S64PUB.PRG`) and Settings (`S64CFG.PRG`) launch independently and return to BASIC. No cross-program live editing session or REU handoff. The shipped eleven-file package currently also contains a standalone site browser program that is not an 8TML reader. Do not delete it or change launcher manifests/preloads until callers and release tests are verified. Publisher and launcher must retain at least 1,024 bytes of measured headroom; preserve Editor overlay and resident-RAM fit checks.

Use a complete consistently built package for one storage root. Default: `/sd/SEE64YOU/`. A forced rebuild with `STORAGE_ROOT=/Usb0/` (or the exact mounted name with trailing slash) changes embedded paths in all assets. No cross-device fallback or automatic migration is supported.

~~~text
SEE64YOU.PRG        Editor entry
S64EDIT.PRG, *.BIN  private Editor assets
S64PUB.PRG          Publisher
S64CFG.PRG          Settings
*.MNF               matching manifests
SEE64YOU.CFG        saved publishing settings
*.8tml              source documents
SEE64YOU.REC        possible recovery snapshot
S64JRN*.DAT         state journals
~~~

Launcher/journal operations own DOS `$01`; Editor document/recovery owns `$02`. Each subsystem sets its directory context and closes handles. Bare source names resolve in the selected app folder; explicit absolute paths must retain their origin. A failed lookup must never silently select another file/device.

## File integrity and recovery

- Source uses complete ASCII/LF 8TML 1 draft text, up to 61,440 bytes. Publisher downloads to a **new** filename and must refuse collisions. Failed rename can leave a recoverable complete `.tmp` file.
- Editor has no periodic autosave. Existing `SEE64YOU.REC` may be offered; a failed read preserves the snapshot. Recovered content is dirty until manually saved.
- Configuration writes validate the temporary record before replacement and validate final readback. Current delete-then-rename is **not** fully atomic if final rename fails; preserve temporary data and surface errors.
- The fixed-size two-slot journal records version, sequence and ACTIVE/CLEAN machine-state ownership. Write/readback the alternate slot before superseding valid state. Corrupt/conflicting state must block unsafe changes.
- Preserve existing root journals/configuration and any verified backups; Settings may require explicit recovery/migration actions when an old root journal blocks safe startup. No silent removal or replacement of user files.

## Restore machine state

For temporary settings: **capture → durable ACTIVE → apply → use → restore/verify → durable CLEAN**. Failed ACTIVE persistence means do not change hardware; failed restoration means retain recoverable state and report an error. Restore prior live turbo/IRQ/colours, DOS contexts and BASIC workspace on clean return; do not permanently rewrite unrelated Ultimate audio/video/drive/network/input preferences.

Close bounded UCI, REU, DOS and TCP operations on success, failure and cancellation. The BASIC-return path protects its original context/scratch reservations and must not execute cc65 code after stack/zero-page restoration. Never weaken guards to fit optional features.

## Security and acceptance

Direct C64U publishing uses raw HTTP on UCI `$03` with per-site challenge/HMAC authentication; source remains unencrypted. Pair on HTTPS and require explicit consent. No silent TLS downgrade.

Host and VICE validation do not prove real Ultimate-DOS, REU, physical keys, USB storage, network or cold-power persistence. Preserve complete file/device backups and compare machine state before and after bounded real-C64U acceptance; never reset or alter unrelated data as a test shortcut. See [testing](LOCAL_TESTING.md).
