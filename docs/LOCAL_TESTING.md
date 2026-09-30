# Local validation runbook

This file contains the **current** reproducible validation path. Historical failure investigations and completed diagnostic sequences are kept in Git history, not here. The authoritative unresolved work queue is `todo.md`.

## 1. Full repository validation

From the repository root:

Windows PowerShell:

```powershell
.\tools\validate.ps1
```

WSL/Linux:

```sh
./tools/validate.sh
```

Do not call the run complete if required cc65 stages are skipped. Fix genuine warnings/errors rather than suppressing them.

## 2. Build artifacts

Conventional C64:

```sh
make -C client clean all
```

Expected:

`client/build/see64you.prg`

C64U/editor/publisher package:

```sh
make -C client c64u publisher
```

Keep these four files together:

- `SEE64YOU-C64U.PRG`
- `S64EDIT.PRG`
- `S64PUB.PRG`
- `S64CFG.PRG`

CI should publish the current PRG/package artifacts whenever the relevant build succeeds.

## 3. Stock C64 Ultimate 1.1 setup

Known-good direct-publishing setup:

- Command Interface enabled.
- REU enabled, minimum 128 KiB.
- Package files in `/sd/`.
- Until the Ultimate-DOS chain loader replaces it, SD-backed **software IEC device 8** enabled with `/sd/` selected.
- Conflicting hardware drive 8 disabled.
- Internal companion IEC names omit `.PRG`.

Do not assume a browser-selected/path-import directory is still active for a later DOS operation.

## 4. Editor/publisher chain smoke test

1. Run `SEE64YOU-C64U.PRG`.
2. Enter/edit text.
3. Ctrl+P enters the publisher.
4. Option 1 enters publishing settings.
5. Save + Return returns to publisher with destination/key retained.
6. RUN/STOP returns to the editor with document/editor state intact.
7. Repeat the round trip more than once.

Real firmware-1.1 hardware has passed editor -> publisher -> settings -> publisher -> editor.

## 5. Direct Standalone publishing

The active firmware-1.1 path uses Network Target `$03`; HTTP Target `$06` is not required.

Configure:

- Standalone destination;
- `http://` site URL;
- explicit plain-HTTP consent;
- direct key provisioned through the site's HTTPS `pair.php`.

Normal flow:

1. Save the local canonical `.s64` source.
2. Ctrl+P.
3. Option 2 tests the direct key; expect `STANDALONE CONNECTION READY`.
4. Use T to select Page/Post.
5. Option 3 creates/updates the remote document.
6. Option 4 changes Draft/Published; option 3 applies the changed state.
7. Options 6/7 currently open Page/Post by typed slug after overwrite confirmation.
8. Option 5 deletes the bound remote document after confirmation.
9. RUN/STOP returns to the editor.

Direct mode rejects `https://` because Network `$03` is raw TCP, not TLS. It does not silently downgrade.

Validated baseline on real C64U: direct authentication, exact 4,608-byte upload/download, draft/publish/unpublish, reopen by slug and exact TXT export.

## 6. Local file/storage acceptance

Exercise:

- New/Open/Save/Save As;
- overwrite confirmation;
- exact canonical source reopen;
- failure/cancel paths without stale open handles;
- filename/path handling from a fresh startup rather than relying on previous UI activity.

Publishing settings should survive Save, exit/re-entry and cold application reload.

## 7. Recovery/autosave acceptance

1. Create dirty edits and confirm `SEE64YOU.REC` is created/refreshed.
2. Restart and test Recover.
3. Repeat and test Discard.
4. Confirm successful Save/Save As removes the recovery snapshot.
5. Confirm a deliberate discard removes it.
6. Confirm returning exactly to saved state does not leave stale recovery data.

VICE covers the core storage workflow; final DOS/UI timing remains a real-C64U gate until recorded complete in `todo.md`.

## 8. Keyboard/editor acceptance

At 40x25 verify:

- continuous typing, insertion, Backspace and forward Delete;
- cursor remains visible during typing/navigation;
- Home/End and Page Up/Page Down;
- Ctrl word movement;
- word-wrap on/off and horizontal scrolling;
- MARK selection, Select All, Cut/Copy/Paste;
- undo/redo;
- Find/Find Next/Previous;
- Replace/Replace All;
- Go To Line;
- Bold/Italic/List formatting;
- shifted letters/punctuation and lock behaviour;
- modern USB keyboard versus physical C64 mapping;
- no drawing outside the 40x25 layout.

Record exact firmware/keyboard type for hardware-only failures.

## 9. Large-document C64U acceptance

Use a document materially larger than the conventional-C64 capacity and verify:

- navigation remains responsive;
- edit latency is acceptable at start/middle/end;
- search/replace;
- large clipboard;
- multi-level undo/redo;
- save/reopen;
- publisher/preview transitions preserve source.

Automated VICE already covers a 6,000-byte REU workload and REU capacity/preservation from 128 KiB through 16 MiB. Real-hardware interactive responsiveness is tracked separately in `todo.md`.

## 10. Standalone site browser acceptance

After automated tests, manually confirm when site-facing code changes:

- one-time setup;
- login failure/success;
- page/post draft and publish;
- signed draft preview;
- newest-first post listing;
- pagination/year archives;
- plain/simple/C64 themes;
- keyboard focus/readability;
- upload package from `php tools/package-site.php`.

## 11. Diagnostic PRGs

Diagnostic PRGs are regression tools, not the normal user workflow. Use them only when the corresponding behaviour regresses:

- `network-probe.prg` — Network `$03` DNS/TCP/raw HTTP.
- `settings-io-probe.prg` — isolated Ultimate-DOS transaction/readback.
- `settings-exchange-probe.prg` — settings validation/REU exchange.
- `settings-save-probe.prg` — instrumented real helper save path.
- `settings-load-probe.prg` — read-only load/path diagnosis.
- `recovery-probe.prg` — recovery storage workflow.

Do not re-run old incident sequences simply because they remain in Git history; reproduce the current failure first.

## 12. Relay and WordPress

These are compatibility/deferred lanes.

Relay validation should cover authentication, allowed paths, redirects, header stripping and size limits before relying on it outside a controlled LAN.

WordPress validation eventually covers plugin activation, Application Passwords, permission test, page/post CRUD, publish/unpublish, slug conflicts and exact source retention.

## 13. Reusable hardware automation

New hardware automation should converge on:

- `JC-000/c64-test-harness` for VICE/real-C64U smoke tests;
- `barryw/sim6502` / `u64sim` for deterministic UCI/Ultimate-DOS CI;
- a developer upload/run command for the current package.

Firmware-1.1 automated REST uploads must account for the known `/Temp` attachment leak documented by the external harness. Real hardware remains the final gate for firmware/timing claims.
