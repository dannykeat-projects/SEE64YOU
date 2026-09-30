# See64You TODO

This is the authoritative **live** project work queue. Keep completed work concise and move durable implementation detail into the relevant canonical docs. Stable docs describe implemented behaviour; transient incidents and next-step planning belong here until resolved.

## P0 - Foundation

- [x] Establish repository structure and project instructions.
- [x] Define editor/backend separation and canonical source-document model.
- [x] Add first cc65 editor core and C64 screen shell.
- [x] Add first standalone flat-file PHP site.
- [x] Add authenticated standalone publishing API.
- [x] Add WordPress plugin scaffold using WordPress-native authentication.
- [x] Compile and run the C64 client under VICE (user verified startup and basic text entry on 2026-09-24; focused editor-flow checks remain below).
- [x] Run the standalone site locally and exercise all API operations end-to-end.
- [ ] Install the WordPress plugin on a test WordPress instance and exercise REST operations.
- [x] Run integrated automated validation locally and on the `terra64-wsl` self-hosted runner (PHP, HTTP, relay, host C, UCI compile and cc65 build passed 2026-09-24; VICE/C64U remain separate hardware gates).
- [x] Restore the self-hosted runner after the repository transfer (runner returned online and the full integration validation plus downloadable PRG build passed 2026-09-25).

## P0 - C64U hardware reliability and automation

- [x] Verify the underlying settings DOS transaction on real C64U with `settings-io-probe.prg`: user confirmed create/write/close/rename/stat, exact 334-byte readback, zero trailing bytes and valid content/checksum. Expected FILE NOT FOUND preconditions are diagnostic-only successes; production DOS unchanged.
- [x] Add `settings-exchange-probe.prg` using production value/key validation, exchange store, immediate REU readback/checksum and normal settings save redirected to diagnostic filenames; key values never displayed and original REU settings restored.
- [x] Run settings exchange probe on real C64U: known-good Standalone records with empty keys and with the imported direct key both passed values, REU store/readback/checksum/exact bytes, normal save and restoration. Retained REU record was invalid; this does not identify the real helper failure.
- [x] Add `settings-save-probe.prg`, the actual helper with diagnostic-only Save tracing: real filenames/settings, passive GET_PATH, UI/store result, preserved command statuses and first failed DOS gate; normal PRGs/algorithm unchanged.
- [x] Run actual helper Save trace on real C64U: None and Standalone/HTTP-blocked saves passed all gates in `/sd/`; the earlier Save failure remains unreproduced.
- [x] Diagnose settings reload remotely on real C64U (2026-09-30): production FILE_STAT returned `82,FILE NOT FOUND` in `/sd/`; diagnostic-only trailing NUL made stat/read/checksum/UI validation pass. Added read-only `settings-load-probe.prg`; CFG and REU retained unchanged.
- [x] Correct FILE_STAT filename termination and firmware-1.1 status 82 mapping with narrow host regression coverage; normal settings Save and cold CFG reload passed on real hardware.
- [x] Identify current IEC chain environment blocker remotely: device 8 is enabled 1541 with no mounted image, while SD-backed IEC drive 11 is disabled. Enabled SD-backed software IEC on device 8, disabled the conflicting hardware drive, and selected /sd/ for the chain test.
- [x] Correct firmware-1.1 companion IEC names and restore Save + Return. Real C64U editor -> publisher -> settings -> publisher -> editor passed remotely on 2026-09-30, including retained destination/key state and normal Save. Initial Publish input was supplied by a temporary diagnostic fixture; subsequent companions were the normal packaged binaries. Working software IEC configuration saved to flash. Full local and GitHub validation passed.
- [ ] Make publishing-settings load/save/profile operations establish a deterministic C64U storage path themselves rather than relying on a previous key-import or browser-path side effect. Validate on stock C64 Ultimate firmware 1.1.
- [ ] Evaluate `barryw/sim6502` `u64sim` as an external CI lane for the exact Ultimate-DOS settings transaction and other UCI state-machine regressions; keep real hardware as the final gate.
- [ ] Integrate `JC-000/c64-test-harness` as the preferred reusable VICE/real-C64U smoke-test layer, including firmware-1.1-safe REST `/Temp` hygiene for repeated automated uploads.
- [ ] Add a developer hardware-deploy command for the current C64U package so normal test cycles no longer require manually copying PRGs to SD.
- [ ] Replace the current IEC-only internal PRG chaining assumption with a C64U SD/Ultimate-DOS-aware chain path while preserving the separate editor/publisher/settings memory budgets and REU handoff. Validate editor -> publisher -> settings -> publisher -> editor repeatedly on hardware.
- [ ] Audit UCI register-window compatibility against hardware-tested projects that probe both $DF1C-$DF1F and $DE1C-$DE1F; do not change the currently working path without target-hardware evidence.
- [ ] Keep external project adoption/licensing decisions in `docs/ENGINEERING-REFERENCES.md`; `todo.md` remains the implementation queue.

## P0 - Editor essentials

- [x] Replace whole-editor redraws during typing with incremental updates (validated in VICE on 2026-09-27 at `5da0c391`: continuous typing, mid-line insertion near the top and bottom of a full page, Backspace in the middle of the document, newline insertion/deletion, and editing after scrolling/Page Up/Down all performed well. The final path combines constant-time append/status updates, local editor position maintenance, direct affected-row character redraws, and conservative fallbacks for complex edits).
- [x] Accurate vertical cursor movement preserving preferred column.
- [x] Word wrap toggle and horizontal scrolling (F8; base-C64 cc65 path validated under VICE 2026-09-28).
- [x] Home, End, Page Up/Page Down and word-navigation editor commands.
- [x] Selection model and visible selection.
- [x] Cut/copy/paste using an application clipboard.
- [x] Undo/redo transaction history (bounded single-level RAM implementation with Ctrl+Z / Ctrl+Shift+Z plus host regression coverage; base-C64 cc65 undo/redo and saved-state dirty tracking validated under VICE 2026-09-28. C64U additionally uses the validated 16-level REU history backend).
- [x] Find / find next / find previous (UI flows implemented; base-C64 cc65 editor path validated under VICE 2026-09-28).
- [x] Replace / replace all (UI flows implemented; base-C64 cc65 editor path including undo/redo validated under VICE 2026-09-28).
- [x] Go to line (UI flow implemented; existing and missing-line behaviour validated on the base-C64 cc65 path under VICE 2026-09-28).
- [x] New/Open/Save/Save As using C64U-accessible storage (streamed canonical files, temporary-file replacement, overwrite detection and host tests implemented; C64U validation pending).
- [x] Dirty-document confirmation before New, Open and Exit.
- [ ] Recovery/autosave files (reserved `SEE64YOU.REC` snapshot, startup recovery prompt and active-edit refresh implemented with host tests. The real cc65 document/DOS/local-storage path now passes a VICE recovery workload covering snapshot save, discovery, exact reopen and deletion. Real C64U DOS/UI timing remains to validate before closing this item).
- [ ] Finish/validate modern USB-keyboard shortcuts and C64-keyboard-friendly mappings (MARK/function-key baseline implemented; Shift+letter capitals confirmed in VICE 2026-09-25. VICE showed Ctrl+Arrow/Delete can produce no KERNAL buffered character at all, so waiting inside `cgetc()` prevented the earlier raw-key decoder from ever running. The input loop now polls raw matrix/modifier state before blocking, with a small repeat delay/rate for held Ctrl navigation. Ctrl+Left/Right, Ctrl+Up/Down and Ctrl/Commodore+Delete use this path. Printable-key KERNAL repeat is enabled while Shift remains allowed; Ctrl/Commodore chords disable KERNAL repeat so Save/Publish/etc. cannot auto-fire while held, and stock repeat mode is restored on exit. A deterministic VICE diagnostic now verifies $028A behavior for no modifier, Shift, Commodore, Ctrl, combined shortcut modifiers and restoration. Base-C64 clipboard is temporarily bounded at 256 bytes to retain RAM headroom. Shifted punctuation, lock keys, modern USB-keyboard behavior and final real-C64U shortcut validation remain).

## Manual VICE/C64U checks

- [x] Add standalone non-destructive `settings-io-probe.prg` to the C64U build/package: stepped raw DOS status/path/stat, explicit `/sd/`, production CREATE_NEW, checksum and exact readback using only `S64IOTST.TMP`/`S64IOTST.CFG`; real settings are stat-only and production save unchanged.
- [x] Run settings I/O probe on real C64U: underlying file transaction and exact settings readback/checksum passed (user confirmed). Deliberately stale-temp repeat remains unverified.

These player-facing checks are assigned to the user where practical; automated source and regression checks remain part of development.

- [x] After incremental rendering: VICE test passed 2026-09-27 on `5da0c391`; continuous typing, mid-line insert/delete, held Backspace, newline edits and editing after scrolling/Page Up/Down were all reported working well, with no responsiveness issue reported.
- [x] Retest cursor/navigation/rendering in VICE: 2026-09-27 software-cursor + wrapped-row safety build reported working quite well. Word-by-word Ctrl navigation works, cursor visibility is much improved during navigation/typing, and no repeat of the transient disappearing-line issue was reported in this retest.
- [x] Add deterministic VICE screen-RAM regression coverage for wrapped append, mid-line incremental insertion, newline split/reflow, newline join/reflow, stale-row clearing and reversed software-cursor placement; the monitor checks each 40-column body row at staged checkpoints.
- [x] Add deterministic VICE keyboard-repeat register coverage: no modifier and Shift select all-key repeat, Ctrl/Commodore shortcut chords disable KERNAL repeat, and editor exit restores the stock repeat setting.
- [ ] After Shift handling: verify Shift+letter capitals, shifted punctuation, Caps Lock/Shift Lock behaviour, and unshifted input with both the emulated C64 keyboard and a modern USB keyboard where available.
- [ ] Exercise New, Open, Save and Save As, including dirty-document and overwrite prompts; confirm saved source reopens exactly.
- [ ] Exercise recovery/autosave: create dirty edits, confirm `SEE64YOU.REC`, restart and recover/discard, then confirm Save/discard removes the snapshot.
- [ ] Exercise word wrap, horizontal scrolling, selection, clipboard, undo/redo, find/replace, go-to-line and formatting shortcuts at 40x25.
- [ ] Repeat storage and keyboard checks on real C64U hardware when available; record firmware and keyboard type with any failure.

## P0 - Publishing

- [x] Normalize C64 PETSCII text at external ASCII boundaries before end-to-end publishing (canonical .s64 plus UCI HTTP/DOS conversion are implemented; target-compiled VICE regression round-trips mixed case, shifted punctuation and newlines across the real document wire boundary).
- [x] Make direct C64U-to-Standalone-site publishing the default without a relay: active S64PUB uses Network `$03`, one-time challenge/HMAC and explicit plaintext HTTP opt-in. On 2026-09-30 the real C64U authenticated, created/updated a live page and published it; independent canonical download matched all 4,608 source bytes. SHA/HMAC temporarily uses turbo and restores it before REU/network I/O. HTTPS is not supported by this transport. See `docs/LOCAL_TESTING.md` for the normal workflow.
- [x] Preserve occupied REU bytes during companion startup capacity probing. Live draft comparison exposed the old destructive bank stamp overwriting the first document byte; replacement preserves candidate/bank-zero bytes, handles unmapped-bank bus echo and passes the 128 KiB–16 MiB VICE matrix plus exact real-C64U upload.
- [ ] Add remote list browsing to the direct publisher. Firmware-1.1 direct mode currently opens pages/posts by typed slug; legacy HTTP-target list support remains in source/tests.
- [ ] Implement C64U HTTP transport behind the publisher interface as a compatibility/newer-firmware path (bounded UCI/HTTP and Standalone login/list/open/create/update/delete are implemented and host-tested. The active downloadable publisher now uses direct Network $03; legacy HTTP-target logic and WordPress remain in source/tests. Firmware 1.1 has no HTTP target, so this compatibility path remains unverified).
- [x] Add bounded UCI capability probes for Ultimate-DOS `$01`, Network `$03` and HTTP `$06`.
- [x] Verify Network Target `$03` on the user's stock C64 Ultimate 1.1 hardware using `network-probe.prg` (2026-09-29: UCI, DOS `$01` and Network `$03` identified; `$03` resolved `example.com`, opened TCP/80, sent raw HTTP and received `HTTP/1.1 200 OK`).
- [x] Verify HTTP Target `$06` availability on the user's stock C64 Ultimate 1.1: it is absent. The firmware dispatcher returns `NO TARGET` with `00,OK` to `$06 IDENTIFY`, and `$06 $10` returns `21,UNKNOWN COMMAND`. The capability probe was corrected to reject the generic `NO TARGET` identity.
- [ ] Standalone legacy HTTP-target login/token handling (host-tested; real HTTP-target validation remains. The active firmware-1.1 publisher uses a direct key and no password/token login).
- [ ] Keep the secure local relay as a compatibility/fallback path rather than the default Standalone requirement (relay implementation, client relay-key header and persistent masked relay-key configuration through S64CFG.PRG are complete. Sixteen unit/policy/loopback-handler tests cover authentication, allowed paths, forwarding, credential stripping, responses and size limits. Real LAN/C64U relay validation remains).
- [ ] List/open/create/update/delete standalone pages (legacy operations are host-tested. Active direct mode supports open by slug, save/update, delete and draft/published status; direct list browsing and remaining hardware CRUD checks are still pending).
- [ ] List/open/create/update/delete standalone posts (legacy operations are host-tested. Active direct mode supports open by slug, save/update, delete and draft/published status; direct list browsing and remaining hardware CRUD checks are still pending).
- [ ] WordPress Application Password configuration and connection test — deferred until the standalone/general editor, storage, relay and C64U paths are otherwise working end-to-end. Existing implementation remains intact; no further WordPress-specific work for now.
- [ ] List/open/publish/update/unpublish/delete WordPress pages and posts — deferred until the rest of SEE64YOU is working end-to-end. Existing CRUD/list/open implementation and tests remain in place; real WordPress-instance/C64U validation comes later.
- [x] Friendly error mapping for network, auth and server failures (short C64-facing result messages are wired through destination/login, list/open, save/update and delete flows for both backends).
- [ ] Publishing destination setup UI (persistent destination/relay configuration now uses the separate `S64CFG.PRG` helper so Ultimate-DOS and settings UI do not consume the memory-tight publisher. Safe cancel and persisted-destination validation are implemented. FAT companion files are `S64EDIT.PRG`, `S64PUB.PRG`, `S64CFG.PRG`; internal IEC names omit the suffix and use unshifted PETSCII. D64 aliases round-trip byte-for-byte in CI, and the real C64U editor/publisher/settings round trip passed on 2026-09-30. Direct authentication, exact draft upload and publish/unpublish are now proven on the same real C64U).

## P1 - Document format

- [x] Define simple metadata + source-body representation.
- [x] Implement safe minimal markup rendering on the standalone backend.
- [x] Freeze See64You Markup v1 (H1/H2, bold, italic, unordered list items and HTTP(S) links). The syntax is documented independently of convenience UI, and canonical fixtures round-trip exactly through both the C document wire path and PHP wire/parser/renderer tests; unsupported constructs remain literal.
- [x] Bold / italic / headings / lists / links formatting command core (basic formatting is isolated in `editor_format.c`; Ctrl+B, Ctrl+I and Ctrl+L remain wired into the conventional-C64 runtime. Heading formatting remains available to the future Format menu, but Ctrl+1/Ctrl+2 were removed after VICE validation on 2026-09-27 because typing `#` / `##` is equally direct. The URL/link core remains host-tested but dormant: directly linking `editor_format_link.c` overflowed the C64 BSS memory area by 936 bytes with the 1 KiB stack on 2026-09-27, so link UI needs a different memory/overlay strategy).
- [x] C64 preview view (publisher-side preview keeps editor RAM unchanged; Markup v1 headings/bold/links are visually distinguished, formatting markers/link targets are hidden, unsupported source remains literal, long documents page/restart, and host/full hosted validation pass. Real C64U visual validation remains a hardware check).
- [x] Preserve unsupported markup as text rather than corrupting it (verified by renderer regression coverage 2026-09-25).

## P1 - Standalone site

- [x] Pages and posts stored as ordinary files.
- [x] Automatic page navigation.
- [x] Newest-first posts listing.
- [x] Plain theme requiring neither CSS nor JavaScript.
- [x] Optional simple/C64-inspired inline styling.
- [x] Browser-based one-time setup helper.
- [x] Site-settings API for title/theme/colours.
- [x] Draft preview links (authenticated API issues signed one-hour document-specific preview URLs; draft rendering is no-store/noindex/no-referrer with unit and HTTP regression coverage).
- [x] Pagination/archive pages when post count warrants it (10 posts/page with plain year archive links and regression coverage).
- [x] Packaging script producing a self-contained upload directory (`php tools/package-site.php`; excludes local config/tests, includes starter content and fresh setup marker, with regression coverage).

## P1 - C64U enhancements

- [ ] Apply C64U turbo acceleration to CPU-bound paths that materially improve user experience, starting with direct-publishing SHA/HMAC and then profiling large-document search/replace, preview/rendering, import/export conversion and other long-running transforms. Keep acceleration scoped around pure CPU work, restore the prior turbo state immediately afterward, preserve conventional-C64 behavior, and require real-hardware validation before extending turbo across timing-sensitive I/O.

- [x] REU-backed document storage for substantially larger files (renderer storage seam, REU DMA/capacity layer and 60 KiB REU gap-buffer backend are implemented. The separate `see64you-c64u.prg` links that backend; with the REU clipboard it now requires at least a 128 KiB REU. Base-C64 memory remains unchanged).
- [x] Validate REU DMA and capacity detection in VICE across 128 KiB, 256 KiB, 512 KiB, 1 MiB, 2 MiB, 4 MiB, 8 MiB and 16 MiB (binary-monitor matrix passes on the self-hosted runner).
- [x] Validate REU-backed editor correctness in VICE with a 6,000-byte document (line/word navigation, edits beyond 4 KiB, selection/cut/paste, undo/redo and find/replace pass through the real editor core).
- [ ] Validate large-document interactive responsiveness on real C64U hardware before making it the preferred C64U path (automated 6,000-byte VICE workload is now heavily tuned: logical cursor movement no longer relocates the REU gap until an edit, and C64U search/position/newline scans use chunked REU reads. Against the pre-tuning baseline, search improved from ~42.08s to 12.82s, replace/history from ~39.08s to 10.23s, and large undo/redo-related moves from ~12s to ~2.8s. Base-C64 memory is unchanged; moving C64U history metadata into REU reduced generated code/BSS enough that current C64U editor BSS ends at $C7DE, leaving 1,057 bytes before the $CC00 stack boundary).
- [ ] Repeat REU/editor validation on real C64U hardware when available.
- [x] Large clipboard backed by REU (C64U build uses a 60 KiB clipboard in REU bank 1 while the base C64 keeps its 256-byte RAM clipboard. >256-byte copy/paste/cut passes host regression testing, and VICE validated copying the full ~6 KiB editor fixture plus reading clipboard data beyond 4 KiB).
- [x] Multi-level undo backed by REU (C64U build keeps 16 undo/redo payloads in the final 4 KiB of REU bank 0 and stores the compact 16-entry metadata ring in the unused final 4 KiB of REU bank 1 above the 60 KiB clipboard; base C64 retains its existing single-level RAM history. Host tests cover redo branching, saved-state dirty tracking, ring rollover and clipboard-region separation, and the real-REU 6,000-byte editor diagnostic passes with this backend).
- [ ] Mouse selection/menu support where practical.
- [ ] Detect capabilities and degrade cleanly on a conventional C64 where possible (base build now omits dormant formatting runtime code and restores a 1 KiB software stack; current validated base build leaves 379 bytes below the 1 KiB stack; REU support remains dormant until capability-gated).

## P2 - Polish

- [ ] Optional spell-check workflow if a sensible server-assisted design emerges.
- [x] Multiple saved publishing destinations (S64CFG.PRG provides four reusable Standalone profile slots with independent URL, HTTP policy and relay key. Loading a slot is non-destructive until Save; saving a profile does not replace the active configuration; empty/invalid slots and active-config preservation are regression-tested).
- [x] Export/import plain text and See64You source (canonical .s64 source already uses Open/Save; publisher now adds body-only TXT export/import with overwrite confirmation, ASCII/PETSCII conversion, CR/LF normalization and transactional REU staging so invalid imports leave the current document untouched. Hosted validation passes).
- [x] Site backup/download command (authenticated `action=backup` JSON download exports public settings plus all page/post source, excluding auth secrets, with HTTP regression coverage).
- [x] Accessibility/readability pass for colour combinations (styled themes now enforce at least 4.5:1 foreground/background contrast for body text, links and visited links; C64 defaults were raised to compliant values, line height increased, keyboard-focus outlines made explicit, and rendered/setup pages include a mobile viewport. Site and HTTP regression suites pass).
