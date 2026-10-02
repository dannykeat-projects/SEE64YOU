# Editor design

See64You should feel like a small desktop text editor running on a Commodore 64 Ultimate, not like a command-line editor or an original-C64 memory demo.

The 40x25 text interface remains deliberate. The implementation behind it is free to use the C64U's 16 MiB REU, turbo, Ultimate-DOS and other validated platform features.

## Screen

~~~text
 SEE64YOU  FILE EDIT SEARCH FORMAT PUB
----------------------------------------
Document text occupies the main viewport.

The editor is keyboard-first and keeps commands
discoverable through menus rather than requiring
users to memorise everything.
----------------------------------------
Ln 12 Col 8  INS  MODIFIED       POST
~~~

The exact menu presentation can evolve through hardware testing, but normal editing must stay readable and responsive at 40 columns. FILE / EDIT / SEARCH / FORMAT / PUB now open real dropdown menus. Commodore+M opens the menu bar from the keyboard; arrows navigate and Enter selects. Joystick Fire opens/selects and joystick directions navigate. A 1351-compatible mouse can click the top labels and menu items. F8 remains the integrated Help screen.

## Input model

Modern USB keyboard users are first-class:

- arrows: cursor movement;
- Shift+arrows where distinguishable: selection extension;
- Ctrl+arrows: word movement;
- Ctrl+Shift+arrows where distinguishable: wordwise selection;
- Ctrl+A/C/X/V/F/H/S: familiar desktop actions;
- Ctrl+Z: Undo;
- Ctrl+Shift+Z: Redo;
- Home/End, Page Up/Page Down: conventional navigation where firmware exposes them distinctly.

The C64U's native C64 keyboard remains fully usable. Where the C64 keyboard matrix cannot express a PC-style chord cleanly, See64You provides a C64-native equivalent such as MARK mode or function-key commands.

The requirement is not “behave like an original C64”; it is “make both physical input styles intuitive on C64U”.

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

See64You remains source-oriented. Formatting commands insert/remove the frozen See64You Markup v1 syntax from MARKUP.md.

Supported source conveniences include:

- Bold: **...**
- Italic: *...*
- Heading 1/2: # / ##
- List item: -
- Link: [label](url)

Formatting operations must remain transactional from the user's perspective: either the whole source transformation succeeds or the document remains unchanged.

The link formatter was previously kept out of one build because of conventional-C64 BSS headroom. That reason is no longer a product constraint. Reintroduce useful formatting UI when it fits the C64U-native runtime and is worth the interface complexity.

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
- C64U USB-mouse/1351 menu pointing and click behaviour;
- large-document navigation/edit latency;
- deep undo/redo;
- large clipboard;
- recovery;
- module transitions preserving exact editor state.
