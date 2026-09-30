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
- Ctrl+Left/Right: move one word at a time;
- Ctrl+Up/Down: page up/down;
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
- Ctrl+B: toggle bold markers around the selection (or insert an empty bold pair)
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
- Ctrl+I: toggle italic markers around the selection (or insert an empty italic pair)
- Ctrl+L: toggle list-item formatting on the current line
- Ctrl+Z: Undo
- Ctrl+Shift+Z: Redo

Find, Replace, Go to line and local file commands have working prompt flows. Ctrl+Z and Ctrl+Shift+Z use the bounded base-C64 undo/redo transaction layer; ordinary consecutive typing is coalesced into one undo step. On a physical C64 keyboard, Commodore+F1 creates a new document, Commodore+F3 opens, Commodore+F5 saves and Commodore+F7 invokes Save As. Navigation uses C64 CTRL with Left/Right for word movement and Up/Down for page movement; forward delete accepts either CTRL+Delete or Commodore+Delete. In VICE's stock GTK symbolic map, host Left Ctrl maps to C64 CTRL and Tab maps to the Commodore key. Modifier+cursor/delete commands are decoded from the raw C64 keyboard-matrix key so the KERNAL's modifier-specific PETSCII tables cannot reverse or transform the intended direction. The conventional-C64 build keeps its bounded RAM undo/redo transaction; the C64U build uses the implemented 16-level REU history backend. A distinct modern USB Delete key remains a C64U hardware-validation item because firmware may present it through the C64 keyboard abstraction rather than as a unique key code.

## Validation rule

Do not assume PC-specific USB keys such as End or Page Down reach C64 software distinctly. Hardware-test them. If the KERNAL abstraction proves insufficient, add a narrowly scoped raw keyboard-matrix scanner rather than leaking hardware scanning into editor code.

References:
- Commodore 64 Programmer's Reference Guide, SHFLAG at $028D.
- cc65 `conio.h` / `cgetc()`.


### VICE symbolic modifier note

The C64 has only physical Cursor Right and Cursor Down keys; Cursor Left and Cursor Up are the shifted forms of those keys. Consequently, Commodore+Left/Up also contains SHIFT at the C64 level. SHIFT+Commodore is the C64's character-set toggle, which is why Tab+Left/Up in VICE symbolic mode can make the screen appear to switch case/graphics. SEE64YOU therefore uses C64 CTRL, not Commodore, for arrow-based navigation commands. Tab/Commodore remains valid with Delete because that combination does not require SHIFT.
