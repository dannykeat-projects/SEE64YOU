# Keyboard and input

See64You runs on the C64 Ultimate and supports both:

- the machine's native C64 keyboard model;
- modern USB keyboards presented by the C64U.

Neither is treated as a compatibility afterthought.

The editor consumes normalized input actions rather than embedding PETSCII/key-matrix rules throughout editor logic.

## Native C64 keyboard reality

The C64 has physical Cursor Right and Cursor Down keys; left/up are shifted forms. Therefore a simple rule that interprets every Shift+cursor chord as “extend selection” is ambiguous.

The reliable baseline remains MARK mode for the native keyboard.

Current C64-oriented controls:

- F1: toggle MARK mode;
- F2: Find Next;
- cursor movement while MARK is active: extend/shrink selection;
- F3: Copy;
- F4: Find Previous;
- F5: Cut;
- F6: Replace / Replace All;
- F7: Paste;
- F8: Help;
- Ctrl+Left/Right: word movement;
- Ctrl+Up/Down: page movement;
- Ctrl+Delete or Commodore+Delete: forward delete;
- RUN/STOP: cancel/exit the current shell/context where applicable.

FILE / EDIT / SEARCH / FORMAT / PUB are now interactive menus as well as command-group labels. Commodore+M opens the menu bar; Left/Right changes group, Up/Down changes item, and Enter activates the selected command. Existing direct shortcuts remain available. F8 opens the integrated Help screen.

## Modern USB keyboard goal

Where the C64U firmware exposes distinct keys/chords, prefer familiar behaviour:

- Arrow keys: movement;
- Shift+Arrow: selection;
- Ctrl+Arrow: word movement;
- Home/End;
- Page Up/Page Down;
- Delete: forward delete;
- Ctrl+A/C/X/V;
- Ctrl+S;
- Ctrl+F/H/G;
- Ctrl+Z / Ctrl+Shift+Z;
- Ctrl+W: toggle Word Wrap.

Do not assume a PC key necessarily reaches C64 software as a unique event. Hardware-test it.

If the KERNAL abstraction loses a useful distinction, add a narrowly scoped C64U/native input adapter rather than leaking low-level scanning into editor code.

## Current Control shortcuts

Where firmware/key mapping preserves the expected Control chord, See64You recognizes:

- Ctrl+A: Select all
- Ctrl+B: Bold
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
- Ctrl+I: Italic
- Ctrl+L: List item
- Ctrl+Z: Undo
- Ctrl+Shift+Z: Redo
- Ctrl+W: toggle Word Wrap

The current native-keyboard file shortcuts remain:

- Commodore+F1: New
- Commodore+F3: Open
- Commodore+F5: Save
- Commodore+F7: Save As

These may be redesigned as menus mature, but keyboard-only operation must remain complete.

## Joystick and mouse

The C64U editor installs cc65's standard C64 joystick and 1351 mouse drivers in the C64U build only.

Current joystick behaviour uses port 2 so it can coexist with the C64U USB-mouse path on port 1:

- directions move the editor cursor;
- Fire opens FILE from the editor;
- directions navigate an open menu;
- Fire activates the selected menu item;
- Help can be dismissed with joystick input.

Current mouse behaviour targets the C64U's 1351-compatible USB mouse path:

- the pointer remains hidden until actual mouse activity;
- clicking FILE / EDIT / SEARCH / FORMAT / PUB opens that menu;
- clicking a visible menu item activates it;
- clicking outside the active menu dismisses it;
- Help can be dismissed with a mouse click.

Mouse click-to-position in document text, drag selection and scrolling are intentionally still pending. They need editor-position mapping and real-hardware validation rather than being approximated in the menu layer.

## Modifier state

The C64 KERNAL SHFLAG byte at $028D exposes Shift, Commodore and Control bits.

The input layer may combine:

- KERNAL buffered characters;
- modifier state;
- narrowly scoped raw matrix information where necessary;
- C64U-specific evidence for modern USB keys when available.

Modifier+cursor/delete commands already avoid relying solely on modifier-specific PETSCII tables because those tables can reverse/transform direction.

## VICE symbolic note

In VICE's stock GTK symbolic map, host Left Ctrl maps to C64 CTRL and Tab maps to Commodore.

Because C64 left/up imply Shift, Tab/Commodore+Left/Up may also trigger the C64's Shift+Commodore character-set behaviour. That is why arrow-based navigation uses C64 CTRL rather than Commodore.

This is a VICE/input-model fact, not a reason to constrain the C64U product to the original keyboard.

## Source characters

Canonical SEE64YOU/1 currently uses a defined 7-bit ASCII repertoire at the wire boundary.

The C64 editor stores native PETSCII and validates/converts deliberately. Unsupported source characters are rejected rather than silently substituted.

This character repertoire is a document/protocol decision, not a conventional-C64 compatibility requirement. A future protocol version may broaden it deliberately.

## Validation rule

Record:

- C64U firmware;
- keyboard type;
- whether the key is native/USB;
- exact observed command.

Do not convert VICE behaviour into a hardware claim.

References:

- Commodore 64 Programmer's Reference Guide, SHFLAG at $028D.
- cc65 conio/cgetc behaviour.
- docs/LOCAL_TESTING.md for current hardware acceptance.
