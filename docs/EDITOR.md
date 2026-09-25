# Editor design

See64You should feel closer to a small desktop text editor than a command-line editor.

## Screen

```
 SEE64YOU  FILE EDIT SEARCH FORMAT PUB
----------------------------------------
Document text occupies the main viewport.

The editor is keyboard-first and keeps commands
discoverable through menus rather than requiring
users to memorise everything.
----------------------------------------
Ln 12 Col 8  INS  MODIFIED       POST
```

The exact menu presentation will evolve after VICE/hardware testing.

## Input model

Modern USB keyboard target:
- arrows: cursor movement
- Shift+arrows: extend selection
- Ctrl+arrows: word movement
- Ctrl+Shift+arrows: wordwise selection
- Ctrl+A/C/X/V/F/H/S: conventional desktop actions
- Ctrl+Z: Undo; Ctrl+Shift+Z: Redo
- Home/End, Page Up/Page Down: conventional navigation

Original C64 keyboard cannot reproduce every modern chord naturally. It will have a separate mapping, likely using Commodore/function-key assisted commands or an explicit mark mode. The modern mapping must not make an original keyboard unusable.

## Current selection controls

Because the C64 uses Shift to obtain left/up cursor movement, Shift+Arrow cannot be the only selection mechanism. The current reliable mapping uses F1 to toggle MARK mode. Movement extends or shrinks the marked range; F3 copies, F5 cuts and F7 pastes. Selected characters are rendered in reverse video.

Ctrl+A/C/X/V are also normalized when the KERNAL reports a Control chord. These mappings require C64U USB-keyboard validation before they are treated as guaranteed.

## Editing model

The editor core owns:
- text buffer
- cursor
- selection range
- insert/overwrite state
- dirty state
- viewport offset
- bounded single-level undo/redo history

UI rendering should read editor state; it should not become the document model.

## Undo/redo and recovery

The base C64 editor keeps one bounded 256-byte reversible edit transaction in ordinary RAM. Consecutive typing is coalesced, while cursor movement or another editing command starts a new transaction. Ctrl+Z undoes the current transaction and Ctrl+Shift+Z reapplies it. REU-backed larger, multi-level history remains a later enhancement.

When Ultimate DOS storage is available, dirty documents also use the reserved `SEE64YOU.REC` recovery file. The first edit after a clean state is eligible for an immediate recovery write; during active editing the snapshot is refreshed at roughly 30-second intervals. Recovery writes use the same temporary-file replacement path as normal local saves and do not assign a real filename to an unsaved document.

At startup, an existing recovery snapshot is offered for recovery or discard. A recovered document remains dirty and must be saved explicitly. Successful Save/Save As, a confirmed New/Open replacement, or confirmed discard on Exit removes the recovery snapshot. Systems without Ultimate DOS continue editing normally without recovery storage.

## Word wrap and viewport

Word Wrap is enabled by default, preserving the original 40-column soft viewport behaviour. F8 toggles it.

When Word Wrap is off, one logical document line occupies one screen row and the editor keeps a horizontal viewport offset. Moving the cursor beyond the left/right edge scrolls the view just enough to keep the insertion point visible; Home returns the current line to the first column. Re-enabling Word Wrap clears the horizontal offset.

This is a visual layout setting only. It never inserts or removes newline characters from the canonical source.

## Search and navigation

The current editor supports a compact keyboard-driven search flow:

- Ctrl+F enters a search term and selects the next match;
- F2 repeats Find Next;
- F4 repeats Find Previous;
- Ctrl+G prompts for a 1-based line number;
- searches wrap at the start/end of the document.

Ctrl+H or F6 opens a bounded Replace flow: enter the search text, replacement text, then explicitly choose one replacement or Replace All. Replace All preflights the resulting document size before changing text, so an oversized expansion is rejected rather than partially applied.

## Markup

The editor remains source-oriented. Formatting commands insert/remove a deliberately small markup subset instead of creating a full WYSIWYG engine.

Provisional subset:
- `# Heading`
- `## Heading`
- `**bold**`
- `*italic*`
- `- list item`
- `[label](https://example.com)`

The formatting core now implements these source transformations:

- Bold wraps/toggles `**selection**`. With no selection it inserts `****` and leaves the cursor between the marker pairs.
- Italic wraps/toggles `*selection*`; applying Italic inside Bold produces the conventional `***selection***` nesting, and toggling it again removes only the italic layer.
- Heading 1 and Heading 2 replace/toggle the current logical line prefix (`# ` or `## `).
- List Item toggles `- ` at the start of the current logical line.
- Link requires selected label text and an `http://` or `https://` URL, producing `[label](url)`. Labels/new URLs that would break this deliberately small syntax are rejected instead of escaped ambiguously.

Every transformation checks buffer capacity before insertion and is designed to complete fully or leave the source unchanged. The FORMAT menu/UI bindings are still pending, so the subset is not frozen as Markup v1 yet.

## Large documents

The complete conventional-C64 build uses a 4 KiB fixed document buffer, a 512-byte application clipboard and a 256-byte undo transaction in normal memory. These bounds keep the editor, incremental renderer, local storage and recovery workflow inside ordinary C64 RAM. C64U REU storage is intended to support larger documents, clipboard and undo history later. The UI should not assume that all document bytes remain permanently addressable in main RAM.
