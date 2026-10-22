# Editor design

The current 8TML Editor uses the existing 61,440-byte REU gap buffer and 16-level history. Broader design goals below are historical context, not an active implementation queue; `todo.md` owns current scope. See the [8TML Editor preview guide](releases/v0.2.0-alpha.1.md) for installation and tested/pending acceptance.

See64You should feel like a small desktop text editor running on a Commodore 64 Ultimate, not like a command-line editor or an original-C64 memory demo.

The 40x25 text interface remains deliberate. The implementation behind it is free to use the C64U's 16 MiB REU, turbo, Ultimate-DOS and other validated platform features.

## Screen

~~~text
 SEE64YOU  FILE EDIT SEARCH FORMAT
----------------------------------------
Document text occupies the main viewport.

The editor is keyboard-first and keeps commands
discoverable through menus rather than requiring
users to memorise everything.
----------------------------------------
Ln 12 Col 8  INS  MODIFIED       POST
~~~

The independent Editor has FILE / EDIT / SEARCH / FORMAT dropdown menus. Publisher runs separately as `S64PUB.PRG`; there is no PUB menu or Ctrl+P launch shortcut. Commodore+M opens the menu bar from the keyboard; arrows navigate and Enter selects. Joystick Fire opens/selects and joystick directions navigate. A 1351-compatible mouse can click the top labels and menu items, click into document text to reposition the cursor, and drag-select source text through the editor's canonical viewport mapping. F8 opens Help; closing it redraws the document and clears unused viewport rows.

## Input model

The native C64 keyboard is the acceptance baseline. Existing USB-keyboard bindings are retained, but physical USB-keyboard acceptance remains pending. Current bindings include:

- arrows: cursor movement;
- Shift+arrows where distinguishable: selection extension;
- Ctrl+arrows: word movement;
- Ctrl+Shift+arrows where distinguishable: wordwise selection;
- Ctrl+A/C/X/V/F/H/S: familiar desktop actions;
- Ctrl+Z: Undo;
- Ctrl+Shift+Z: Redo;
- Home/End, Page Up/Page Down: conventional navigation where firmware exposes them distinctly.

The C64U's native C64 keyboard remains fully usable. Where the C64 keyboard matrix cannot express a PC-style chord cleanly, See64You provides a C64-native equivalent such as MARK mode or function-key commands.

Distinguish implemented bindings from physical-keyboard acceptance; automated key injection does not certify either physical input style.

RUN/STOP and File → Exit always ask before leaving the independent Editor. A saved document shows `Exit to BASIC? Y/N`: Y exits, N or RUN/STOP returns to editing. Unsaved changes retain `S SAVE  D DISCARD  STOP CANCEL`; a failed or cancelled save keeps the Editor open.

## Current reliable selection controls

Because the C64 keyboard uses Shift to obtain left/up cursor movement, Shift+Arrow cannot be the universal selection rule.

The current reliable mapping uses:

- F1: toggle MARK mode;
- movement while MARK is active: extend/shrink selection;
- F3: copy;
- F5: cut;
- F7: paste.

Selected characters render in reverse video.

Ctrl+A/C/X/V are also normalized where firmware/key mapping preserves the expected Control chord. Hardware validation remains authoritative for modern USB Delete/Home/End/Page keys.

## Editing model

The editor core owns:

- logical document;
- cursor;
- selection;
- insert/overwrite state;
- dirty state;
- viewport;
- undo/redo state.

UI rendering reads editor state; it does not become the document model.

## C64U-only storage direction

The current repository already has an REU-backed editor/history path, but its fixed capacities were chosen while ordinary-C64 compatibility was still a product constraint.

That constraint is retired.

The target editor uses the shared 16 MiB REU allocator from C64U-RUNTIME.md.

Large state should live in allocator-managed REU:

- document pages/pieces;
- undo/redo history;
- clipboard;
- search/index scratch;
- preview/format scratch.

Conventional RAM remains for executing code, screen/UI state and small hot working buffers.

## Large-document model

Do not simply stretch the existing flat gap buffer until every operation becomes a huge REU move.

Benchmark a paged/piece-based design with goals including:

- logical positions larger than old 16-bit assumptions;
- small hot page/cache buffers;
- bounded local edits;
- efficient sequential streaming to storage/network;
- no full-document copy for ordinary save/publish operations;
- predictable recovery.

A piece table, paged gap buffer or similar model should be selected through measurement rather than ideology.

The product does not need to support absurd document sizes merely because 16 MiB exists. It does need to stop imposing limits that only existed for original-C64 compatibility.

## Undo/redo

The current C64U implementation has a fixed 16-level REU history backend. That is migration state, not the final product limit.

Target behaviour:

- allocator-backed variable history;
- consecutive typing coalescing;
- transactions for multi-character commands;
- preserve history while memory is available;
- reclaim oldest history only under explicit memory pressure;
- never sacrifice active unsaved source for history.

Ctrl+Z / Ctrl+Shift+Z remain the primary modern bindings.

## Clipboard

Clipboard becomes allocator-backed rather than preserving the old small fixed-capacity design.

Copy/cut/paste should be limited by available application memory and sensible operation bounds, not by legacy base-C64 buffer sizes.

## Recovery

Recovery has two layers:

- fast REU session/history state;
- durable SD recovery surviving reset/power loss.

The existing SEE64YOU.REC path and transactional save logic remain useful during migration.

Automatic disk recovery waits for at least three seconds without input, so a short pause between words does not start synchronous disk I/O. The minimum interval between snapshot attempts remains 30 seconds; the first pending snapshot can run after the quiet period. Continuous input postpones autosave. Explicit Save remains available at any time.

A recovered document remains dirty until explicitly saved. Successful Save/Save As or deliberate discard clears stale durable recovery.

## Word wrap and viewport

Word Wrap is enabled by default. Ctrl+W toggles it.

When Word Wrap is off, one logical document line occupies one screen row and the editor maintains horizontal viewport state.

Soft wrap is visual only and never inserts/removes canonical newline characters.

## Search and navigation

Current search/navigation behaviour includes:

- Ctrl+F: Find;
- F2: Find Next;
- F4: Find Previous;
- Ctrl+G: Go To Line;
- Ctrl+H or F6: Replace / Replace All;
- wrapped search.

Replace All must continue to preflight/allocate before committing changes so failure never leaves a partially transformed document.

As document positions grow, search/navigation APIs must stop assuming 16-bit position types where that would become a real limit.

## Markup

See64You remains source-oriented. The independent Editor retains complete 8TML 1 source, including its editable header and unknown markup. Existing formatting commands insert/remove the matching literal 8TML syntax; they do not render a page or rewrite metadata. The historical Markup v1 guide does not define the new file format.

Supported source conveniences include:

- Bold: **...**
- Italic: *...*
- Heading 1/2: # / ##
- List item: -

Formatting operations must remain transactional from the user's perspective: either the whole source transformation succeeds or the document remains unchanged.

Links and other 8TML constructs can be typed directly as source. There is no link-formatting command in the current Editor.

## Performance

Use the central runtime performance governor rather than editor-specific turbo writes.

Potential CPU_INTENSIVE work includes:

- large Find/Replace All;
- indexing;
- source transforms;
- preview generation.

Routine typing/rendering should remain incremental even at high turbo. Extra CPU is not a reason to redraw the entire screen for each keystroke.

## Memory pressure

The editor cooperates with the shared REU allocator.

When memory is tight, disposable caches and oldest history are reclaimed before canonical source.

Any operation that cannot obtain enough memory must fail clearly before destructive mutation.

## Acceptance

Real C64U hardware remains the final gate for:

- typing latency;
- cursor visibility;
- modern USB keyboard mapping;
- native C64 keyboard mapping;
- joystick port-2 navigation/menu control;
- C64U USB-mouse/1351 menu pointing, text positioning and drag selection;
- large-document navigation/edit latency;
- deep undo/redo;
- large clipboard;
- recovery;
- module transitions preserving exact editor state.
