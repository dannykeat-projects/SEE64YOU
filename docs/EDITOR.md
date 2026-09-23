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
