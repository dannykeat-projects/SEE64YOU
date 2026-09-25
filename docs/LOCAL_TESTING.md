# Local validation runbook

Use this before merging the current implementation branches. The repository has deliberately kept editor, publishing and C64U-storage work in reviewable dependency chains; local validation is the next integration gate.

## Current pull-request chains

Editor:

```
#1 Search / Replace / Go To Line
 └─ #9 Word Wrap / horizontal viewport
     └─ #11 source-formatting command core
```

Publishing and C64U platform:

```
#3 bounded UCI HTTP transport
 ├─ #4 Standalone List / Open
 ├─ #5 trusted-LAN HTTPS relay
 │   └─ #6 WordPress authentication
 │       └─ #7 WordPress document CRUD
 │           └─ #8 publishing destination / remote identity state
 └─ #10 Ultimate-DOS local filesystem layer
```

Standalone web hardening:

```
#2 unconfigured authentication / setup hardening
```

All of these PRs were individually mergeable when this runbook was added. Do not treat that as runtime validation.

## 1. Create a local integration branch

Start from current `main` and merge the open branches locally in dependency order. Resolve integration conflicts locally; do not rewrite an upstream feature branch merely to make the integration branch compile.

A practical order is:

1. #1, #9, #11
2. #2
3. #3
4. #4 and #10
5. #5, #6, #7, #8

If a later PR already contains its parent commits, merge only what Git still considers new.

## 2. Run the full repository validator

Windows PowerShell:

```powershell
.\tools\validate.ps1
```

Unix-like shell:

```sh
./tools/validate.sh
```

The useful local environment should provide PHP, a host C compiler and cc65/`cl65`. Do not accept a validator run as complete if the cc65 stage is skipped.

Fix compiler warnings that indicate real C/cc65 portability problems rather than suppressing them.

## 3. Build and smoke-test under VICE

Build:

```sh
make -C client clean all
```

Run `client/build/see64you.prg` in VICE.

Check at minimum:

- normal insertion, newline, Backspace, Delete and insert/overwrite behavior;
- Home, End, Page Up/Down, word movement and preferred-column vertical movement;
- MARK selection, Select All, Cut/Copy/Paste and visible reverse-video selection;
- Find, Find Next, Find Previous, Replace, Replace All and Go To Line;
- F8 Word Wrap toggle, including horizontal scrolling of a line longer than 40 characters;
- RUN/STOP exits the shell and cancels bounded prompts while a prompt is active;
- formatting source transformations do not corrupt cursor/selection state;
- Ctrl+Z / Ctrl+Shift+Z undo and redo typing, deletion, paste, Replace All and formatting without stale cursor/selection state;
- screen/status rendering does not write outside the 40x25 layout.

Record any keyboard mapping that differs from the host tests rather than changing editor behavior by guesswork.

## 4. Validate on C64 Ultimate hardware

The target-hardware pass is authoritative for UCI behavior.

Confirm:

- UCI interface detection;
- separate Ultimate-DOS and HTTP-target capability detection;
- Ultimate-DOS open/read/write/close;
- file-existence probe, rename and delete;
- 512-byte write chunking works with real files;
- no stale open file remains after errors;
- HTTP handle lifecycle works after reset/relaunch;
- modern USB keyboard and physical-C64 keyboard mappings both remain usable;
- dirty edits create `SEE64YOU.REC`, restart offers recovery, recovered text matches exactly, and Save/discard removes the recovery file.

Do not relax the current direct-HTTPS safety gate merely because an `https://` URL parses. TLS must be demonstrated on target before direct remote credentials are allowed.

## 5. Standalone-site integration

Run the PHP site locally and repeat the complete authenticated lifecycle after branch integration:

- one-time setup;
- failed login and successful login;
- page/post list;
- open;
- create draft;
- publish;
- update;
- unpublish;
- delete;
- malformed/oversized source rejection;
- 8 KiB body boundary;
- signed-32-bit `order` boundary;
- exact source round trip.

Then test the C64 client against the site over the intended trusted-LAN route.

## 6. Relay and WordPress integration

Start the secure local relay with a controlled HTTPS destination and verify that it refuses:

- an invalid relay key;
- disallowed paths;
- redirects;
- oversized requests/responses;
- unsafe forwarded headers.

Install the See64You plugin on a disposable WordPress test instance and exercise:

- Application Password configuration;
- connection/permission test;
- page and post listing;
- open;
- create;
- update;
- publish/unpublish;
- delete/Trash;
- slug collision;
- insufficient permissions;
- exact source retained in See64You metadata.

Run those tests both directly where appropriate and through the relay path intended for real remote publishing.

## 7. Merge gate

Do not merge the stack simply because individual PRs are conflict-free. Merge after:

- the full validator passes with cc65 enabled;
- VICE smoke testing passes;
- integration regressions found during testing are fixed;
- C64U-specific UCI behavior needed by the feature is verified;
- WordPress-dependent PRs have passed on a real test WordPress instance.

After the first integrated validation pass, update `todo.md` with actual results and then rebase/retarget dependent PRs as needed.
