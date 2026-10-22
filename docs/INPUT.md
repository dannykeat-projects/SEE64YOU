# Keyboard and pointing controls

The **native C64 keyboard is the acceptance baseline**; modern USB-keyboard keys work only where C64U firmware maps them distinctly. VICE key injection does not establish physical-keyboard usability.

The Editor is keyboard-first. Commodore+M opens File/Edit/Search/Format menus; arrows navigate and Return selects. F8 displays Help; RUN/STOP cancels or invokes exit confirmation in the Editor.

| Action | Key |
| --- | --- |
| Toggle selection | F1 (MARK mode) |
| Find next / previous | F2 / F4 |
| Copy / Cut / Paste | F3 / F5 / F7 |
| Replace | F6 or Ctrl+H |
| New / Open | Ctrl+N / Ctrl+O |
| Save / Save As | Ctrl+S / Ctrl+Shift+S |
| Undo / Redo | Ctrl+Z / Ctrl+Shift+Z |
| Find / Go to line | Ctrl+F / Ctrl+G |
| Select all | Ctrl+A |
| Bold / Italic / list marker | Ctrl+B / Ctrl+I / Ctrl+L |
| Toggle visual word wrap | Ctrl+W |

Native-keyboard alternatives: Commodore+F1 New, Commodore+F3 Open, Commodore+F5 Save and Commodore+F7 Save As. Ctrl+Left/Right moves by word; Ctrl+Up/Down moves by page. C64 left/up are shifted cursor keys, so Shift+Arrow cannot reliably mean text selection: use F1 MARK mode. Ctrl/Commodore+Delete may provide forward delete where distinguishable.

Joystick port 2 directions can move/navigate and Fire can open/select menus. A 1351-compatible mouse via the C64U may select menu items, position the cursor and drag-select text. Wheel support is not assumed. These optional input paths must not replace keyboard operation.

## Source bytes and implementation

The current file boundary accepts checked ASCII with LF line endings; unsupported bytes are rejected instead of silently substituted. The Editor uses native PETSCII internally. Preserve that conversion in the source/storage layer rather than embedding it in input or screen code.

The KERNAL modifier state at `$028D` exposes Shift, Commodore and Control, but combinations may be ambiguous. Narrow hardware-tested adapters are preferable to assumptions about PC keycodes. Validate exact keys on the C64 Ultimate, recording firmware, keyboard type and observed action. See [local testing](LOCAL_TESTING.md).
