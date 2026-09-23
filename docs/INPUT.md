# Keyboard and input

See64You uses a normalized input layer so editor commands do not depend directly on PETSCII key values.

## Why selection cannot simply be Shift+Arrow

On a physical C64 keyboard, left and up are themselves produced by shifted cursor keys. A rule that interprets every Shift+cursor combination as "extend selection" would therefore make ordinary left/up movement ambiguous.

USB keyboards presented to C64 software may ultimately have the same ambiguity because the application receives the C64 keyboard model rather than a PC GUI key-event API.

For that reason the reliable baseline is **MARK mode**:

- F1: toggle MARK mode;
- F2: find next;
- cursor keys while MARK is active: extend/shrink the selection;
- F3: copy;
- F4: find previous;
- F5: cut;
- F6: open Replace (UI still being completed);
- F7: paste;
- F8: exit during early development.

The eventual menus will expose the same commands, so function-key shortcuts are never mandatory.

## Control shortcuts

The C64 KERNAL maintains `SHFLAG` at `$028D`:

- bit 0: Shift;
- bit 1: Commodore;
- bit 2: Control.

The input layer uses this state alongside buffered keyboard characters. Where firmware/key mapping preserves the expected Control chord, See64You recognizes:

- Ctrl+A: Select all
- Ctrl+C: Copy
- Ctrl+X: Cut
- Ctrl+V: Paste
- Ctrl+S: Save
- Ctrl+F: Find
- Ctrl+G: Go to line
- Ctrl+H: Replace
- Ctrl+Z: Undo
- Ctrl+Y: Redo

Find and Go to line now have working prompt flows. Save, Replace UI, Undo and Redo remain normalized commands awaiting their implementation layers.

## Validation rule

Do not assume PC-specific USB keys such as End or Page Down reach C64 software distinctly. Hardware-test them. If the KERNAL abstraction proves insufficient, add a narrowly scoped raw keyboard-matrix scanner rather than leaking hardware scanning into editor code.

References:
- Commodore 64 Programmer's Reference Guide, SHFLAG at $028D.
- cc65 `conio.h` / `cgetc()`.
