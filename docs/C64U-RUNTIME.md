# C64 Ultimate runtime contract — MVP

**8TML migration:** the independent Editor now keeps the complete `.8tml` header/body in the existing 61,440-byte REU document region. Its disk format is ASCII/LF; no new allocator, handoff or temporary machine setting is introduced. Publisher/Browser retain historical protocols until their later migration. Resource ownership, bootstrap, recovery and restoration rules below remain binding. REST-assisted real-C64U Editor/recovery/native-storage checks passed on 2026-10-21 with verified folder, configuration, DOS-path and machine-baseline restoration; physical keyboard and cold restart remain unverified.

**Current hardware/resource/safety authority.** SEE64YOU is C64 Ultimate-only with a required 16 MiB REU. The shipped candidate has independently launched Editor, Publisher, Settings and optional Browser programs. Old persistent-runtime adoption, managed Publisher/Settings transitions and one-owner REU session proposals are superseded design experiments retained in Git history, not active requirements.

## 1. Platform and launch contract

- Tested machine baseline: Commodore 64 Ultimate firmware 1.1.0, UCI at `$DF1C..$DF1F`, Ultimate-DOS targets `$01`/`$02`, 16 MiB REU, SD storage, and Network Target `$03` for direct DNS/TCP/HTTP. HTTP Target `$06` was absent and must not be assumed. Probe actual available capabilities rather than guessing from firmware names. Offline Editor may run without networking.
- Public package contains eleven matching installed files. Launch `SEE64YOU.PRG` (Editor through existing private overlay/launcher), `S64PUB.PRG`, `S64CFG.PRG` and optionally `S64WEB.PRG` **independently**. Internal `S64EDIT.PRG` and BIN assets are not user entry points. Every user program returns to responsive BASIC.
- No cross-program REU document/session/history/clipboard or live handoff. Ordinary `.s64` files and canonical disk configuration are the only shared persisted interfaces. Private overlays in Editor remain allowed where required by current memory fit.
- The existing launcher still verifies/preloads unused Publisher and Settings module images as part of its established bootstrap. **Do not remove or alter its module manifest/preload semantics without source tracing, complete package/fit/VICE checks and real-hardware entry/save/return validation.**
- Keep existing Publisher and launcher ≥1,024-byte conventional-RAM headroom guards and Editor resident/overlay slot checks. Do not trade stack/BASIC-return safety for a few more features.

## 2. User file layout and DOS ownership

The following paths show the default SD build. `make -B -C client STORAGE_ROOT=/Usb0/ independent-package` selects another absolute mount (use its actual Ultimate name and retain the trailing slash). `platform/package_paths.h` derives the app directory from that root; preflight, loader, Editor, settings, journals and root-journal checks use it consistently. Install the entire matching package on the chosen device. Do not mix binaries/manifests from different roots. Selection is explicit at build time; no automatic root search/fallback, persistent Ultimate home-directory change or migration across devices is performed. SD regression and USB-root host/package guards are separate from USB hardware acceptance, which remains pending.

```text
/sd/SEE64YOU/
    SEE64YOU.PRG       # Editor user launch
    S64PUB.PRG         # independent Publisher
    S64CFG.PRG         # independent Settings
    S64WEB.PRG         # optional read-only Browser
    S64EDIT.PRG / *.BIN / manifests    # internal matching Editor assets
    SEE64YOU.CFG       # canonical persisted settings
    *.S64              # editable source and new downloads
    SEE64YOU.REC       # durable Editor recovery snapshot
    S64JRN0.DAT / S64JRN1.DAT     # journal slots
    S64OLD0.DAT / S64OLD1.DAT     # verified retired root backups, when migrated
    S64CHK*.TMP and save temporary files # narrowly scoped probes/writes
```

- Bare source filenames resolve inside this folder. Explicit `/sd/NAME.S64` opens older SD-root source; Save retains that origin. Save As with a *new bare name* creates a folder copy. Same-name root/folder files must never be substituted. Failure/corruption is an error, not a reason to silently fall back.
- Editor document/autosave I/O owns Ultimate-DOS target `$02`; launcher/journal I/O owns `$01`. Each establishes its own directory context and closes files before conflicting journal activity. Publisher reestablishes the folder for loading/saving.
- Publisher downloads source only to a **new** filename and refuses existing-file collision. Save As obeys the second existence check. Do not remove recoverable completed `filename.tmp` after rename failure.
- Configuration writes validate the temporary record **before** replacing the old destination and validate final readback afterward. Config reads reject trailing as well as truncated/corrupt data. The current delete-then-rename replacement isn't fully atomic: final rename can leave destination absent and completed temp recoverable; never promise both old and new copies survive a failure.

## 3. Journal/recovery and migration

- Session journal is an explicit versioned fixed-size wire representation (24 bytes), not direct serialisation of a cc65 structure; it records sequence, package/session identity, ACTIVE/CLEAN ownership and only captured restorable fields. Store/readback-verify the alternate slot before superseding the previous one. A torn newest slot may fall back to the prior valid record.
- System state changes follow **capture → durable ACTIVE → apply → use → restore/verify → durable CLEAN**. If ACTIVE persistence fails, don't change the hardware. If restoration verification or CLEAN persistence fails, leave recoverable ACTIVE state rather than claiming exit succeeded. Only explicitly valid captured fields may be restored.
- On startup, distinguish no journal, CLEAN, interrupted ACTIVE, invalid data and storage failure. Only a valid interrupted ACTIVE record authorises bounded restoration. Invalid records or unreadable storage are not implicit permission to reset or overwrite user state.
- Legacy `/sd/S64JRN*.DAT` journals **block** Editor startup (`JOURNAL BLOCKED - RUN S64CFG.PRG`). Only independent Settings `M`, then explicit `Y`, performs the verified migration. Inspect both stores and resolve ACTIVE ownership, verify original CLEAN and folder CLEAN staging, preserve byte-exact `S64OLD*.DAT` backups, then retire root slots in safe order. Conflicting ACTIVE ownership, corrupt/unreadable stores, sequence wrap, readback/restore failure or divergent existing backups block deletion and allow retry.
- Folder `SEE64YOU.REC` is the only snapshot automatically offered by Editor. Old root `SEE64YOU.REC` must not be silently moved/deleted. Compare and back up snapshots before manual copying. Failed recovery load preserves the original REC, **disables autosave** until explicitly resolved and never silently discards data. Recovered content stays dirty until saved.
- Never delete, migrate or overwrite unrelated root files, old source, journals, backups or configuration automatically. Verify exact bytes before any test cleanup; keep prior backups until the user explicitly authorises removal.

## 4. BASIC return and machine restoration

- Capture and preserve the original incoming SYS/BASIC context; protect the 321-byte original snapshot at `$CA82..$CBC2` while teardown uses `$C800` scratch. The 642-byte scratch plus 321-byte context must fit the protected 1 KiB area before the `$CC00` stack reservation. Guards protect overlap.
- On exit, preserve ordering: resolve dirty source/recovery; close DOS/network; revoke active transient records where applicable; restore and verify captured settings; persist CLEAN; execute the verified BASIC return path. Do not run cc65 code after its zero-page/stack state has been restored.
- The implemented transient exit stub uses the cassette buffer `$033C`, clears the screen via KERNAL without executing from screen RAM, then restores original CPU context. It protects BASIC numerical and string commands; when STREND is strictly above MEMSIZ, the application restores a valid BASIC variable boundary without disturbing valid workspaces.
- Do not permanently change unrelated SID/audio, HDMI/video, drives, cartridge settings, Ultimate home directory, DHCP/static IP, palette or input preferences. Temporary turbo/IRQ/runtime screen colours must restore prior supported values on normal exit. A failed verification is visible, not silently successful.
- UCI/REU/DOS/TCP handles are bounded and closed in success, error and user cancellation. The public Browser changes its current screen colours per site and restores original screen colours; it must not change turbo/drives/IRQ or send Publisher credentials.
- REST-based test harness resets can change machine state; separate this from application-owned turbo restoration. Real CPU-executed `PEEK(53297)` after a load-only launch is stronger evidence than I/O-address RAM shadow readback.

## 5. Resource and security boundaries

- 16 MiB REU is actual working storage, not justification for unlimited buffers. Preserve checked document/history/preview allocator boundaries and DMA bank isolation. The Browser uses two 64 KiB preview regions with bounded rows/links; its conventional RAM contains only current row, URL, headers and record buffers.
- The C64U direct Publisher speaks HTTP over UCI Network Target `$03`. Dedicated site key, one-time challenge and HMAC-SHA256 authenticate method/URI/body and prevent replay but **do not encrypt** document contents. Require explicit plaintext consent. `pair.php` provisioning and ordinary site passwords use HTTPS in the modern browser; never leak reusable credentials to direct HTTP.
- Browser traffic is public read-only HTTP with explicit `SEE64YOU-BROWSE` Accept. It rejects HTTPS (never downgrades), malformed/oversized URLs/records and unsupported scheme; clears partial frames and closes sockets. No HTML/JS/PETSCII memory commands are executed.
- Server validates permissions, path/slug/type and source before writes; generated HTML escapes unsafe content. Keep [PROTOCOL.md](PROTOCOL.md) and [MARKUP.md](MARKUP.md) formats intact for this beta.

## 6. Acceptance and maintenance

- Current independent hardware evidence: bounded Editor→Publisher publish/download/reopen/update, Settings persistence, SD-folder migration, browser navigation and machine restoration on firmware 1.1. The **full unassisted first-user installation, updated palette on two independent sites, physical storage/power interruption and USB keyboard acceptance are not yet all verified**.
- Exact developer build/host/PHP/VICE/C64U gates, known receipts and negative tests live in [LOCAL_TESTING.md](LOCAL_TESTING.md). The only active queue, deferred scope and go/no-go rules live in [todo.md](../todo.md).
- Run current targeted tests before changes. Replace obsolete loader/managed code **only if no active source/package/CI caller requires it**, package checks still pass and hardware-dependent behaviour is validated on the C64U. Do not force a wholesale runtime refactor just to reduce line count.

## Outside the beta

No shared managed runtime, WordPress publication, general HTML web browser, original-C64 port, arbitrary network-executed machine instructions, new system-state abstraction, large-document/allocator redesign, USB acceptance, mouse/joystick, or TLS implementation project unless required to fix a reproduced MVP blocker. Historical implementation specifics remain available through Git history.
