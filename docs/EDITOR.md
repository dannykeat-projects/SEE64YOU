# Editor

The Editor is a keyboard-first, offline source editor for C64 Ultimate. One editing engine supports `.8tml` documents, literal `.8ss` stylesheets, and general text (default `.txt`, with safe explicit extensions such as `.md`). New 8TML source starts with an editable version/title header; new 8SS and text files start empty. All formats share the 61,440-byte REU-backed gap buffer and 16 levels of history.

## User interface

The 40×25 screen provides File, Edit, Search and Format menus. File → New prompts **1 8TML**, **2 8SS**, **3 Text**, or **0 Cancel**. The current file type appears on the top bar. Commodore+M opens the menu bar; arrow keys navigate, Return selects. F8 opens Help and closes back to the document. The standalone Publisher runs separately; the Editor has no publishing menu.

Click in the text to place the cursor. Hold the left mouse button and drag across character cells to select text. Moving the cursor with an arrow key clears a mouse selection. With MARK active, the arrow keys extend or shrink the selection.

With word wrap enabled, a word that would cross the right margin moves to the next screen row. An unbroken word wider than the screen splits at the margin. These visual breaks leave the saved text unchanged.

F1 toggles MARK selection; F3 copies, F5 cuts, F7 pastes. Ctrl+S saves and Ctrl+Shift+S uses Save As where the keyboard mapping supports those chords. Ctrl+O opens; Ctrl+F searches; F2/F4 repeat the search; Ctrl+H or F6 replaces; Ctrl+G goes to a line; Ctrl+Delete deletes back to the previous Ctrl+Left word boundary as one undoable edit, or deletes the active selection; Ctrl+Z / Ctrl+Shift+Z undo/redo; Ctrl+W toggles visual word wrap. See [input](INPUT.md) for C64/USB mapping and physical acceptance limitations.

Format shortcuts insert literal 8TML constructs such as headings, list markers, bold and italic. **Format → Link** (Ctrl+K where mapped) inserts `[text](destination)` as ordinary source text. Mark up to 80 characters first to use the selection as the link label; otherwise enter a label. Supply a relative `.8tml` path or an `http://` / `https://` URL. With no selection, using Link when the cursor is inside a simple existing link changes its URL, preserving the label. RUN/STOP cancels prompts, and Undo reverses the completed link edit. The native URL prompt currently accepts up to 254 characters; longer links must be entered as source. Other markup can still be typed directly. Format shortcuts apply to 8TML only; 8SS and general text remain literal source. Visual word wrap never changes stored line endings. Unknown metadata and markup are preserved, not interpreted or discarded.

RUN/STOP and File → Exit require confirmation even when the document is saved. A dirty document offers Save / Discard / Cancel; failures keep the Editor open.

## Persistence and safety

The Editor saves on explicit command only; **no periodic autosave or idle-time disk write** occurs. Recovery snapshots live at the package root: `SEE64YOU.REC` (8TML), `SEE64YOU.8SS.REC` (8SS) and `SEE64YOU.TXT.REC` (text). A failed read must not destroy them. If multiple snapshot types exist, the Editor refuses an ambiguous recovery instead of choosing one. Unsaved new text is not automatically backed up. Save/Save As use checked storage and collision safeguards; completed temporary files may remain recoverable after a failed rename.

**Open requires an exact filename**, including extension where present; it never guesses another suffix. File → Save As adds `.8tml`, `.8ss`, or `.txt` to an extensionless name according to the current file type. An explicit safe suffix (such as `README.md` or `notes.ini`) is kept; explicit `.8tml` or `.8ss` selects that format's validation rules. Invalid 8TML content cannot be saved under an 8TML suffix. Opening a `.tmp` or `.bak` file preserves its underlying type, but a temporary copy requires Save As to a normal filename. Publisher loads `.8tml`, `.8ss` and `.txt`; its extensionless-filename convenience still appends `.8tml`. Other text suffixes remain local; Save As `.txt` to publish their literal text.

Replacement saves retain the original as `<filename>.bak` until the new file is installed. A failed final rename attempts to restore the original and preserves the completed `<filename>.tmp`. If either recovery file already exists, “Recover .tmp/.bak first” blocks writing. Inspect those copies and save recovered text under a new name before removing them explicitly. For a recovery filename longer than the Editor's 33-character path limit, first copy it to a shorter name using an external file manager. See [runtime](C64U-RUNTIME.md) for failure and recovery details.

The existing 16 MiB REU provides working storage, but the current fixed document/history capacities remain. Resource expansion, new allocator designs and large clipboard work should be considered only after measurements show they are required. Preserve responsive incremental redraws, existing source, reversible operations and clean machine-state restoration.

Copy/cut/paste supports the current 61,440-byte document/clipboard limit; an over-capacity paste is refused before replacing a selection. Undo history remains bounded, so a large edit may not be undoable. Help and menu dismissal repaint the covered document rows, including blank rows below short documents.

An interrupted REU paste or undo/redo can leave partial changes. The Editor reports failure, redraws the current text, marks possible changes unsaved and discards unreliable history. This is not automatic rollback: inspect the text and save a separate copy before continuing. A failed operation that changes no bytes, such as an over-capacity paste, retains the existing document.

If a document REU read reports failure, `REU ERR Y=exit` blocks further editing and saving. Press Y to return to BASIC through normal machine-state restoration; N or RUN/STOP leaves the error prompt active. Existing source and recovery files are retained. Relaunch and reopen a known-good disk copy; unsaved REU changes are not automatically recovered. The error remains latched even if a later transfer succeeds. This handles reported transfer failures; it does not detect silent REU corruption or prove hardware fault detection.

At startup, **Recover snapshot? Y/N** treats RUN/STOP like **N** and attempts to delete the snapshot. Press **Y** to recover it and save the recovered source explicitly.

For setup and a full first-document example see [installation](INSTALL.md).
