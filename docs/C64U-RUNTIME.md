# C64 Ultimate runtime and storage

**Platform:** C64 Ultimate only, Command Interface enabled, 16 MiB REU, Ultimate-DOS `$01`/`$02`. On firmware 1.1.0, UCI `$DF1C..$DF1F` and Network `$03` were verified and HTTP `$06` was unavailable; 1.2.1 introduces `$06` and has separate REST-assisted Editor test evidence. The standard Publisher still uses `$03` and has no general TLS. Probe capabilities and actual operations rather than inferring support from firmware strings; see [UCI compatibility](UCI.md).

## Program and package boundaries

Editor (`SEE64YOU.PRG` through its launcher/internal overlays), Publisher (`S64PUB.PRG`) and Settings (`S64CFG.PRG`) launch independently and return to BASIC. No cross-program live editing session or REU handoff. The current package contains ten files: four root PRGs and six support files under `SYSTEM/`. Earlier published alpha packages retain their original flat layout. It excludes the legacy browser. Install the complete matching package including its `SYSTEM/` folder; never mix flat-layout assets with a new launcher. Existing release assets are separate and must not be modified. Publisher links source/configuration readers without the Editor save/recovery writers or Settings writers (`S64_STORAGE_READ_ONLY`); these remain available in their owning programs. Publisher and launcher must retain at least 1,024 bytes of measured headroom; preserve Editor overlay fit and at least 1,024 bytes of independent Editor resident reserve. Cache-rebind descriptor validation remains resident even when session helpers use PREP.

Use a complete consistently built package for one storage root. Default: `/sd/SEE64YOU/`. A forced rebuild with `STORAGE_ROOT=/USB1/` (or the exact mounted name with trailing slash) changes embedded paths in all assets. No cross-device fallback or automatic migration is supported.

~~~text
SEE64YOU.PRG        Editor entry
S64EDIT.PRG         internal Editor executable
S64PUB.PRG          Publisher
S64CFG.PRG          Settings
SYSTEM/
  S64MODULE.MNF     checksummed module inventory
  S64OVL.MNF        verified Editor overlay manifest
  S64MODEL.BIN      Editor overlay
  S64FILE.BIN       Editor overlay
  S64RUN.BIN        Editor overlay
  S64PREP.BIN       Editor overlay
SEE64YOU.CFG        saved publishing settings (unchanged)
docs/
  *.8tml            8TML source documents
  *.8ss             literal stylesheet source
  *.txt / *.md      plain-text source (explicit safe suffixes allowed)
  <name>.tmp/.bak   recoverable save temporaries
SEE64YOU.REC        8TML recovery snapshot
SEE64YOU.8SS.REC    8SS recovery snapshot
SEE64YOU.TXT.REC    plain-text recovery snapshot
S64JRN*.DAT         state journals (unchanged)
~~~

Launcher/journal operations own DOS `$01`; Editor document/recovery owns `$02`. Each subsystem sets its directory context and closes handles. Ordinary Editor source operations explicitly select `SEE64YOU/docs/`, creating it on first save if absent. Publisher reads from that directory but never creates it. Recovery snapshots and configuration remain under `SEE64YOU/`; the Editor selects the package root for snapshot operations. Explicit absolute paths retain their origin. Older root-level `.8tml` files remain unchanged and are not silently found, overwritten or moved; back up and copy them manually if needed. A failed lookup must never silently select another file/device.

## File integrity and recovery

- Source uses ASCII/LF text up to 61,440 bytes. The Editor validates 8TML headers, while 8SS/plain text remain literal. Publisher loads only 8TML source and never writes source during publication. Failed rename can leave a recoverable complete `.tmp` file.
- Local saves refuse an existing `<filename>.tmp` or `<filename>.bak` before writing. Replacement writes and closes `.tmp`, renames the original to `.bak`, installs `.tmp`, then removes `.bak`. Failed installation attempts to restore the original; failed restoration leaves both recovery files. A backup-cleanup error leaves the installed source and old backup and reports failure. Inspect/recover retained files explicitly before retrying; no automatic deletion of pre-existing recovery copies. This sequence is recoverable, not an atomic or power-loss guarantee.
- Editor has no periodic autosave. Recovery filenames distinguish 8TML (`SEE64YOU.REC`), 8SS (`SEE64YOU.8SS.REC`) and text (`SEE64YOU.TXT.REC`). Multiple snapshots are treated as a conflict, not automatically selected or deleted. A failed read preserves the snapshot. Recovered content is dirty until manually saved.
- Configuration writes validate the temporary record before replacement and validate final readback. Current delete-then-rename is **not** fully atomic if final rename fails; preserve temporary data and surface errors.
- The fixed-size two-slot journal records version, sequence and ACTIVE/CLEAN machine-state ownership. Write/readback the alternate slot before superseding valid state. Corrupt/conflicting state must block unsafe changes.
- Preserve existing root journals/configuration and any verified backups; Settings may require explicit recovery/migration actions when an old root journal blocks safe startup. No silent removal or replacement of user files.

## Restore machine state

For temporary settings: **capture → durable ACTIVE → apply → use → restore/verify → durable CLEAN**. Failed ACTIVE persistence means do not change hardware; failed restoration means retain recoverable state and report an error. Restore prior live turbo/IRQ/colours, DOS contexts and BASIC workspace on clean return; do not permanently rewrite unrelated Ultimate audio/video/drive/network/input preferences.

Close bounded UCI, REU, DOS and TCP operations on success, failure and cancellation. The BASIC-return path protects its original context/scratch reservations and must not execute cc65 code after stack/zero-page restoration. Never weaken guards to fit optional features.

The Editor captures border, background and text colours before launcher startup in its checksummed, session-bound BASIC-return context. After successful shutdown and CLEAN persistence, it reapplies those colours before clearing the screen and returning to BASIC. Install the matching launcher and Editor together.

## Security and acceptance

Direct C64U publishing uses raw HTTP on UCI `$03` with per-site challenge/HMAC authentication; source remains unencrypted. Obtain a standard key through the independent server over HTTPS and require explicit HTTP consent. No silent TLS downgrade.

Host and VICE validation do not prove real Ultimate-DOS, REU, physical keys, USB storage, network or cold-power persistence. Preserve complete file/device backups and compare machine state before and after bounded real-C64U acceptance; never reset or alter unrelated data as a test shortcut. See [testing](LOCAL_TESTING.md).

Module verification accelerates compute only when the existing turbo-register probe establishes control. Turbo Control Off remains a valid base-speed mode, but verification can take longer. The C64U Turbo Registers setting enables that selective runtime acceleration; temporary test changes must retain a durable capture and verify restoration of the original setting.
