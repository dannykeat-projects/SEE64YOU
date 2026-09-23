# See64You TODO

This is the authoritative project work queue. Keep completed work concise and move implementation detail into code/docs rather than growing this file indefinitely.

## P0 - Foundation

- [x] Establish repository structure and project instructions.
- [x] Define editor/backend separation and canonical source-document model.
- [x] Add first cc65 editor core and C64 screen shell.
- [x] Add first standalone flat-file PHP site.
- [x] Add authenticated standalone publishing API.
- [x] Add WordPress plugin scaffold using WordPress-native authentication.
- [ ] Compile and run the C64 client under VICE; fix any cc65/keyboard incompatibilities found.
- [x] Run the standalone site locally and exercise all API operations end-to-end.
- [ ] Install the WordPress plugin on a test WordPress instance and exercise REST operations.

## P0 - Editor essentials

- [ ] Incremental screen redraw rather than whole-editor redraw after each input.
- [x] Accurate vertical cursor movement preserving preferred column.
- [ ] Word wrap toggle.
- [x] Home, End, Page Up/Page Down and word-navigation editor commands.
- [x] Selection model and visible selection.
- [x] Cut/copy/paste using an application clipboard.
- [ ] Undo/redo transaction history.
- [ ] Find / find next / find previous.
- [ ] Replace / replace all.
- [ ] Go to line.
- [ ] New/Open/Save/Save As using C64U-accessible storage.
- [ ] Dirty-document confirmation before destructive actions.
- [ ] Recovery/autosave files.
- [ ] Finish/validate modern USB-keyboard shortcuts and C64-keyboard-friendly mappings (MARK/function-key baseline implemented).

## P0 - Publishing

- [ ] Implement C64U HTTP transport behind the publisher interface.
- [x] Add bounded UCI/HTTP-target capability probe from the published interface.
- [ ] Verify current C64U UCI HTTP behaviour on target hardware.
- [ ] Standalone login from C64U and token handling.
- [ ] List/open/create/update/delete standalone pages.
- [ ] List/open/create/update/delete standalone posts.
- [ ] WordPress Application Password configuration and connection test.
- [ ] Publish/update/unpublish WordPress pages and posts.
- [ ] Friendly error mapping for network, auth and server failures.

## P1 - Document format

- [x] Define simple metadata + source-body representation.
- [x] Implement safe minimal markup rendering on the standalone backend.
- [ ] Freeze See64You Markup v1 after editor formatting commands are implemented.
- [ ] Bold / italic / headings / lists / links formatting commands.
- [ ] C64 preview view.
- [ ] Preserve unsupported markup as text rather than corrupting it.

## P1 - Standalone site

- [x] Pages and posts stored as ordinary files.
- [x] Automatic page navigation.
- [x] Newest-first posts listing.
- [x] Plain theme requiring neither CSS nor JavaScript.
- [x] Optional simple/C64-inspired inline styling.
- [x] Browser-based one-time setup helper.
- [x] Site-settings API for title/theme/colours.
- [ ] Draft preview links.
- [ ] Pagination/archive pages when post count warrants it.
- [ ] Packaging script producing a self-contained upload directory.

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
- [ ] Site backup/download command.
- [ ] Accessibility/readability pass for colour combinations.
