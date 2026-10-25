# Editor

The Editor is a keyboard-first, offline 8TML source editor for C64 Ultimate. It preserves the complete `.8tml` header and body rather than rendering HTML or reconstructing unknown markup. New source starts with a minimal editable version/title header. The document capacity is currently 61,440 bytes in a REU-backed gap buffer, with 16 levels of history.

## User interface

The 40×25 screen provides File, Edit, Search and Format menus. Commodore+M opens the menu bar; arrow keys navigate, Return selects. F8 opens Help and closes back to the document. The standalone Publisher runs separately; the Editor has no publishing menu.

F1 toggles MARK selection; F3 copies, F5 cuts, F7 pastes. Ctrl+S saves and Ctrl+Shift+S uses Save As where the keyboard mapping supports those chords. Ctrl+O opens; Ctrl+F searches; F2/F4 repeat the search; Ctrl+H or F6 replaces; Ctrl+G goes to a line; Ctrl+Z / Ctrl+Shift+Z undo/redo; Ctrl+W toggles visual word wrap. See [input](INPUT.md) for C64/USB mapping and physical acceptance limitations.

Format shortcuts insert literal 8TML constructs such as headings, list markers, bold and italic. **Format → Link** (Ctrl+K where mapped) inserts `[text](destination)` as ordinary source text. Mark up to 80 characters first to use the selection as the link label; otherwise enter a label. Supply a relative `.8tml` path or an `http://` / `https://` URL. With no selection, using Link when the cursor is inside a simple existing link changes its URL, preserving the label. RUN/STOP cancels prompts, and Undo reverses the completed link edit. The native URL prompt currently accepts up to 254 characters; longer links must be entered as source. Other markup can still be typed directly. Visual word wrap never changes stored line endings. Unknown metadata and markup are preserved, not interpreted or discarded.

RUN/STOP and File → Exit require confirmation even when the document is saved. A dirty document offers Save / Discard / Cancel; failures keep the Editor open.

## Persistence and safety

The Editor saves on explicit command only; **no periodic autosave or idle-time disk write** occurs. Existing `SEE64YOU.REC` snapshots can be offered for recovery, and a failed recovery read must not destroy them. Unsaved new text is not automatically backed up. Save/Save As use checked storage and collision safeguards; completed temporary files may remain recoverable after a failed rename.

Open and Save As accept a basename such as `notes`; `.8tml` is added automatically when no extension is supplied. Explicit filenames are kept, including `.8tml.tmp` when opening a recoverable temporary. Save uses the current filename. The same basename rule applies when Publisher loads a local source file.

The existing 16 MiB REU provides working storage, but the current fixed document/history capacities remain. Resource expansion, new allocator designs and large clipboard work should be considered only after measurements show they are required. Preserve responsive incremental redraws, existing source, reversible operations and clean machine-state restoration.

Copy/cut/paste supports the current 61,440-byte document/clipboard limit; an over-capacity paste is refused before replacing a selection. Undo history remains bounded, so a large edit may not be undoable. Help and menu dismissal repaint the covered document rows, including blank rows below short documents.

An interrupted REU paste or undo/redo can leave partial changes. The Editor reports failure, redraws the current text, marks possible changes unsaved and discards unreliable history. This is not automatic rollback: inspect the text and save a separate copy before continuing. A failed operation that changes no bytes, such as an over-capacity paste, retains the existing document.

For setup and a full first-document example see [installation](INSTALL.md).
