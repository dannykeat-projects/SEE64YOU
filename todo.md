# See64You TODO

This is the authoritative project work queue. Keep completed work concise and move implementation detail into code/docs rather than growing this file indefinitely.

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
- [ ] Recovery/autosave files (reserved `SEE64YOU.REC` snapshot, startup recovery prompt and active-edit refresh implemented with host tests; integration/C64U validation pending).
- [ ] Finish/validate modern USB-keyboard shortcuts and C64-keyboard-friendly mappings (MARK/function-key baseline implemented; Shift+letter capitals confirmed in VICE 2026-09-25. VICE showed Ctrl+Arrow/Delete can produce no KERNAL buffered character at all, so waiting inside `cgetc()` prevented the earlier raw-key decoder from ever running. The input loop now polls raw matrix/modifier state before blocking, with a small repeat delay/rate for held Ctrl navigation. Ctrl+Left/Right, Ctrl+Up/Down and Ctrl/Commodore+Delete use this path. Base-C64 clipboard is temporarily bounded at 256 bytes to retain RAM headroom; REU-backed large clipboard remains planned. Shifted punctuation, lock keys, and final VICE/C64U shortcut validation remain).

## Manual VICE/C64U checks

These player-facing checks are assigned to the user where practical; automated source and regression checks remain part of development.

- [x] After incremental rendering: VICE test passed 2026-09-27 on `5da0c391`; continuous typing, mid-line insert/delete, held Backspace, newline edits and editing after scrolling/Page Up/Down were all reported working well, with no responsiveness issue reported.
- [x] Retest cursor/navigation/rendering in VICE: 2026-09-27 software-cursor + wrapped-row safety build reported working quite well. Word-by-word Ctrl navigation works, cursor visibility is much improved during navigation/typing, and no repeat of the transient disappearing-line issue was reported in this retest.
- [ ] After Shift handling: verify Shift+letter capitals, shifted punctuation, Caps Lock/Shift Lock behaviour, and unshifted input with both the emulated C64 keyboard and a modern USB keyboard where available.
- [ ] Exercise New, Open, Save and Save As, including dirty-document and overwrite prompts; confirm saved source reopens exactly.
- [ ] Exercise recovery/autosave: create dirty edits, confirm `SEE64YOU.REC`, restart and recover/discard, then confirm Save/discard removes the snapshot.
- [ ] Exercise word wrap, horizontal scrolling, selection, clipboard, undo/redo, find/replace, go-to-line and formatting shortcuts at 40x25.
- [ ] Repeat storage and keyboard checks on real C64U hardware when available; record firmware and keyboard type with any failure.

## P0 - Publishing

- [ ] Normalize C64 PETSCII text at external ASCII boundaries before end-to-end publishing (canonical .s64 plus UCI HTTP/DOS command/status/string conversion implemented with regression coverage; self-hosted/C64U validation pending).
- [ ] Implement C64U HTTP transport behind the publisher interface (bounded UCI/HTTP layer plus standalone login/save/delete operations implemented and host-tested; UI/hardware wiring pending).
- [x] Add bounded UCI/HTTP-target capability probe from the published interface.
- [ ] Verify current C64U UCI HTTP behaviour on target hardware.
- [ ] Standalone login from C64U and token handling (bounded login/session logic implemented and host-tested; direct plaintext login requires explicit trusted-LAN opt-in; UI/hardware validation pending).
- [ ] Provide a secure local relay path for remote publishing unless C64U firmware gains verified TLS support (relay implementation, client relay-key header and unit tests added; local/target validation and UI configuration pending).
- [ ] List/open/create/update/delete standalone pages (all core operations implemented; list/open regression tests added; local/target execution and UI wiring pending).
- [ ] List/open/create/update/delete standalone posts (all core operations implemented; list/open regression tests added; local/target execution and UI wiring pending).
- [ ] WordPress Application Password configuration and connection test (plugin connection endpoint, bounded C64 credential session and host regression tests implemented; WordPress-instance and C64U validation pending).
- [ ] List/open/publish/update/unpublish/delete WordPress pages and posts (C64-safe plugin protocol and bounded client CRUD core implemented with regression tests; real WordPress-instance, C64U and UI validation pending).
- [ ] Friendly error mapping for network, auth and server failures (short C64-facing result messages implemented; UI wiring pending).
- [ ] Publishing destination setup UI (backend-neutral active destination model, bounded endpoint URL construction and remote-identity state implemented/host-tested; interactive C64 UI and persistence pending).

## P1 - Document format

- [x] Define simple metadata + source-body representation.
- [x] Implement safe minimal markup rendering on the standalone backend.
- [ ] Freeze See64You Markup v1 after formatting UI/menu wiring and round-trip validation.
- [x] Bold / italic / headings / lists / links formatting command core (basic formatting is isolated in `editor_format.c`; Ctrl+B, Ctrl+I and Ctrl+L remain wired into the conventional-C64 runtime. Heading formatting remains available to the future Format menu, but Ctrl+1/Ctrl+2 were removed after VICE validation on 2026-09-27 because typing `#` / `##` is equally direct. The URL/link core remains host-tested but dormant: directly linking `editor_format_link.c` overflowed the C64 BSS memory area by 936 bytes with the 1 KiB stack on 2026-09-27, so link UI needs a different memory/overlay strategy).
- [ ] C64 preview view.
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

- [x] REU-backed document storage for substantially larger files (renderer storage seam, REU DMA/capacity layer and 60 KiB REU gap-buffer backend are implemented. The separate `see64you-c64u.prg` links that backend; with the REU clipboard it now requires at least a 128 KiB REU. Base-C64 memory remains unchanged).
- [x] Validate REU DMA and capacity detection in VICE across 128 KiB, 256 KiB, 512 KiB, 1 MiB, 2 MiB, 4 MiB, 8 MiB and 16 MiB (binary-monitor matrix passes on the self-hosted runner).
- [x] Validate REU-backed editor correctness in VICE with a 6,000-byte document (line/word navigation, edits beyond 4 KiB, selection/cut/paste, undo/redo and find/replace pass through the real editor core).
- [ ] Validate large-document interactive responsiveness on real C64U hardware before making it the preferred C64U path (automated 6,000-byte VICE workload is now heavily tuned: logical cursor movement no longer relocates the REU gap until an edit, and C64U search/position/newline scans use chunked REU reads. Against the pre-tuning baseline, search improved from ~42.08s to 12.82s, replace/history from ~39.08s to 10.23s, and large undo/redo-related moves from ~12s to ~2.8s. Base-C64 memory is unchanged; final C64U BSS ends at $CA1C, leaving about 483 bytes before the $CC00 stack boundary, so further conventional-RAM growth should be conservative).
- [ ] Repeat REU/editor validation on real C64U hardware when available.
- [x] Large clipboard backed by REU (C64U build uses a 60 KiB clipboard in REU bank 1 while the base C64 keeps its 256-byte RAM clipboard. >256-byte copy/paste/cut passes host regression testing, and VICE validated copying the full ~6 KiB editor fixture plus reading clipboard data beyond 4 KiB).
- [x] Multi-level undo backed by REU (C64U build keeps 16 undo/redo transactions in the final 4 KiB of REU bank 0, with 256-byte payloads per transaction; base C64 retains its existing single-level RAM history. Host tests cover redo branching, saved-state dirty tracking and ring rollover, and the real-REU 6,000-byte editor diagnostic passes with this backend).
- [ ] Mouse selection/menu support where practical.
- [ ] Detect capabilities and degrade cleanly on a conventional C64 where possible (base build now omits dormant formatting runtime code and restores a 1 KiB software stack; current validated base build leaves roughly 427 bytes below the 1 KiB stack; REU support remains dormant until capability-gated).

## P2 - Polish

- [ ] Optional spell-check workflow if a sensible server-assisted design emerges.
- [ ] Multiple saved publishing destinations.
- [ ] Export/import plain text and See64You source.
- [x] Site backup/download command (authenticated `action=backup` JSON download exports public settings plus all page/post source, excluding auth secrets, with HTTP regression coverage).
- [ ] Accessibility/readability pass for colour combinations.
