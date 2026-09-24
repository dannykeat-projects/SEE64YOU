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
- Ctrl+A/C/X/V/Z/Y/F/H/S: conventional desktop actions
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
- undo history (later)

UI rendering should read editor state; it should not become the document model.

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

The subset is not frozen until editor commands and round-trip tests exist.

## Large documents

The first implementation proves editing with a fixed buffer in normal memory. C64U REU storage is intended to support larger documents, clipboard and undo history later. The UI should not assume that all document bytes remain permanently addressable in main RAM.
