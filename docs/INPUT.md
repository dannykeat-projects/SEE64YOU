# Keyboard and input

See64You uses a normalized input layer so editor commands do not depend directly on PETSCII key values. Printable keyboard input is validated against an explicit 7-bit source-text PETSCII/ASCII mapping rather than assuming printable PETSCII occupies ASCII byte ranges.

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
- F6: Replace / Replace All;
- F7: paste;
- F8: toggle Word Wrap;
- Ctrl+Left/Right or Commodore+Left/Right: move one word at a time;
- Ctrl+Delete or Commodore+Delete: delete the character under the cursor (forward delete);
- RUN/STOP: exit the current development shell (inside a prompt it cancels the prompt).

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
- Ctrl+N: New
- Ctrl+O: Open
- Ctrl+S: Save
- Ctrl+Shift+S: Save As
- Ctrl+F: Find
- Ctrl+G: Go to line
- Ctrl+H: Replace
- Ctrl+Z: Undo
- Ctrl+Shift+Z: Redo

Find, Replace, Go to line and local file commands have working prompt flows. Ctrl+Z and Ctrl+Shift+Z use the bounded base-C64 undo/redo transaction layer; ordinary consecutive typing is coalesced into one undo step. On a physical C64 keyboard, Commodore+F1 creates a new document, Commodore+F3 opens, Commodore+F5 saves and Commodore+F7 invokes Save As. For navigation/editing, both CTRL and Commodore modifiers are accepted with Left/Right for word movement and with Delete for forward delete. This intentionally accommodates VICE symbolic mappings, where host Left Ctrl commonly acts as the C64 Commodore key while Tab acts as C64 CTRL. REU-backed multi-level history remains a later enhancement. A distinct modern USB Delete key remains a C64U hardware-validation item because firmware may present it through the C64 keyboard abstraction rather than as a unique key code.

## Validation rule

Do not assume PC-specific USB keys such as End or Page Down reach C64 software distinctly. Hardware-test them. If the KERNAL abstraction proves insufficient, add a narrowly scoped raw keyboard-matrix scanner rather than leaking hardware scanning into editor code.

References:
- Commodore 64 Programmer's Reference Guide, SHFLAG at $028D.
- cc65 `conio.h` / `cgetc()`.
