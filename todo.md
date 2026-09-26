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

- [ ] Replace whole-editor redraws during typing with incremental updates (constant-time character/newline append rendering, native blinking cursor and cursor-only drawing are implemented; ordinary navigation avoids whole-document-prefix position rescans. VICE comparison on 2026-09-26 showed BASIC and SEE64YOU share the same first-line fast-touch-typing rollover losses, but only SEE64YOU accumulated latency. Replacing per-key printf/status rebuilding with direct C64 screen-RAM line/column/dirty updates fixed sustained typing. Remaining production printf formatting was removed, allowing the 1 KiB cc65 stack to be restored. The first affected-row redraw still proved slow by line 4, so the current compact path now erases non-newline Backspace directly at document tail and stops ordinary single-character redraws at the first safe short explicit-line boundary instead of continuing through later lines. Full validation and cc65 build pass in run 36230126206; focused VICE Backspace/mid-line retest pending).
- [x] Accurate vertical cursor movement preserving preferred column.
- [x] Word wrap toggle and horizontal scrolling (F8; VICE validation pending).
- [x] Home, End, Page Up/Page Down and word-navigation editor commands.
- [x] Selection model and visible selection.
- [x] Cut/copy/paste using an application clipboard.
- [ ] Undo/redo transaction history (bounded single-level RAM implementation with Ctrl+Z / Ctrl+Shift+Z and regression tests added; integration/VICE validation pending).
- [x] Find / find next / find previous (UI flows implemented; VICE validation pending).
- [x] Replace / replace all (UI flows implemented; VICE validation pending).
- [x] Go to line (UI flow implemented; VICE validation pending).
- [x] New/Open/Save/Save As using C64U-accessible storage (streamed canonical files, temporary-file replacement, overwrite detection and host tests implemented; C64U validation pending).
- [x] Dirty-document confirmation before New, Open and Exit.
- [ ] Recovery/autosave files (reserved `SEE64YOU.REC` snapshot, startup recovery prompt and active-edit refresh implemented with host tests; integration/C64U validation pending).
- [ ] Finish/validate modern USB-keyboard shortcuts and C64-keyboard-friendly mappings (MARK/function-key baseline implemented; Shift+letter capitals confirmed in VICE 2026-09-25; shifted punctuation, lock keys and hardware mappings remain).

## Manual VICE/C64U checks

These player-facing checks are assigned to the user where practical; automated source and regression checks remain part of development.

- [ ] After incremental rendering: type continuously, insert/delete within a line, hold Backspace, move the cursor and scroll; confirm immediate display with no full-screen flicker or stale characters.
- [ ] Retest the 2026-09-25 editor checkpoint in VICE: confirm a visible blinking cursor; type continuously through multiple wrapped screenfuls; use Left/Right and Up/Down across wrapped rows; then use Page Up to return to the preceding screen. Record separately whether append typing, navigation, Backspace and mid-document insertion remain slow.
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
- [x] Bold / italic / headings / lists / links formatting command core (UI/menu wiring and VICE validation pending).
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

- [ ] REU-backed document storage for substantially larger files.
- [ ] Large clipboard backed by REU.
- [ ] Multi-level undo backed by REU.
- [ ] Mouse selection/menu support where practical.
- [ ] Detect capabilities and degrade cleanly on a conventional C64 where possible.

## P2 - Polish

- [ ] Optional spell-check workflow if a sensible server-assisted design emerges.
- [ ] Multiple saved publishing destinations.
- [ ] Export/import plain text and See64You source.
- [x] Site backup/download command (authenticated `action=backup` JSON download exports public settings plus all page/post source, excluding auth secrets, with HTTP regression coverage).
- [ ] Accessibility/readability pass for colour combinations.
