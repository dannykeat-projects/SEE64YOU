# SEE64YOU product scope

SEE64YOU is an **offline 8TML source editor for the Commodore 64 Ultimate**, accompanied by a small Publisher and Settings program. It creates portable `.8tml` files, not HTML or a proprietary page format. A compatible external 8TML/T8WN browser handles reading pages. SEE64YOU is not a website engine, CMS or browser.

## Programs and workflow

| Program | Purpose |
| --- | --- |
| Editor (`SEE64YOU.PRG`) | Create, open, edit, find, format, undo/redo and explicitly save portable 8TML documents; preserve unknown markup and recovery data |
| Publisher (`S64PUB.PRG`) | Select a local source and remote path, inspect the destination, then explicitly publish or replace it using standard API 1 |
| Settings (`S64CFG.PRG`) | Import/replace a dedicated publishing key, manage one destination and consent to direct HTTP, and save configuration safely |

Each program launches independently and returns to BASIC. The Editor must work without networking, account credentials or any other program. The normal path is **Edit → Save → Publisher → Select source/target → Confirm upload → Read with a compatible external browser**.

Source is literal 8TML 1 draft ASCII/LF, including header and body; preserve source and unknown constructs rather than generating HTML. CR/CRLF input uses the documented LF normalization. The current local source limit is 61,440 bytes, independent of a server's advertised upload limit. No autosave, hidden remote identity, conversion of older formats or automatic Editor/Publisher handoff. Preserve existing recoverable temporary files, failed-write protection, REU-backed buffers/history and machine-state restoration.

## Publisher interface

The initial target is a **small 40×25 PETSCII character interface**, not a GUI framework. Numbered buttons work with native C64 keyboard input; a compatible mouse/joystick may provide the same actions if proven to fit and work on hardware. Typed filenames remain available.

- Home: **1 Local files, 2 Server files, 3 Publish, 4 Target name, 5 Test server, 0 Exit** (target layout).
- Local files: six numbered `.8tml` rows per page, **7 Type name, 8 Previous, 9 Next, 0 Back**. Do not expose settings, recovery or unrelated files as ordinary source. Keep all file paths exact; don't open truncated names.
- Server files: use the **existing authenticated standard list** to choose a remote destination, without rewriting signing/transport code. List selection must not publish or delete.
- Publishing requires a fresh existence check and a **separate explicit create/replace confirmation** showing the effective local source and remote destination. API 1 has no atomic replace protection. Report server warnings and unknown outcomes without automatically retrying uploads.

**Implementation status:** The active draft branch contains an initial numbered keyboard menu and confirmation plus a local file picker. These changes have **not been accepted through a new cc65 build, VICE or real-C64U run**. The server selector is not implemented. Mouse support is optional and not implemented. The shipped packages are not implied to include draft features. See [Publisher UI](PUBLISHER.md) for concrete controls and [todo](../todo.md) for the only active implementation queue.

**Download-to-edit is not part of the first release.** Publishing API 1 offers no authenticated source GET and list entries do not supply raw-source URLs. Consider a single safe new-file download only after confirming source access on an independent compatible host via C64U-supported HTTP and checking whether existing 8TML tooling already provides it. Never fetch rendered HTML as source, invent a proprietary API, silently downgrade HTTPS, overwrite an existing local file or make download a prerequisite for publishing.

## Publishing boundary and security

Implement the **8TML publishing API 1 test draft dated 26 October 2026**, with standard status/challenge/list/put requests, a dedicated 8TML key, single-use challenges and streaming HMAC-SHA256. Remote names are validated site-root-relative paths of at most 64 characters; local and remote names are distinct. Honor standard response limits, authenticated list results, server `max-size`, and all returned errors, URL and warnings.

On the tested C64U, direct publishing uses UCI Network target `$03` and plain HTTP. Source bytes are **not encrypted**; explicit HTTP consent is required. Do not transmit ordinary website credentials, silently downgrade HTTPS, embed secret keys in documents or introduce an in-repository proprietary publishing backend. Provision/revoke keys on the independent server's HTTPS interface. The API is experimental: tests against one server do not prove universal compatibility.

The complete wire contract is in [PROTOCOL.md](PROTOCOL.md). Storage and machine-state constraints are in [C64U-RUNTIME.md](C64U-RUNTIME.md).

## Out of scope

No bundled website, WordPress adapter, CMS concepts (posts/pages, themes, drafts, account management), bespoke relay, client browser, remote deletion, bulk remote file manager, stylesheet editor or new session-transfer system. A small disposable protocol test server may be used for validation only. Remove retained unused code only after identifying real callers and establishing safe package/rollback behavior; never delete user files or recovery copies as part of cleanup.

## Release acceptance

- Create, save, reopen and edit identical source offline on a complete matching SD or selected USB build; preserve all local files and recovery on faults.
- Validate the exact program/assets package, manifests, Editor resident/overlay fit and at least **1,024 bytes measured Publisher and launcher headroom**.
- Confirm standard publishing to a real independent compatible endpoint, create/replace confirmation, signed readback, returned URL, errors/cancellations and compatible external browser navigation.
- Verify real-C64U physical keyboard, cold-power reopen, network, storage paths, cleanup, REU/IRQ/turbo/colours and responsive BASIC return. Mark hardware or USB-only steps unverified until tested with exact binaries. Mouse and raw-source download have separate optional acceptance gates.

See [installation](INSTALL.md), [development and tests](LOCAL_TESTING.md) and the active [work queue](../todo.md). Historical receipts and implementation decisions belong in Git and test artifacts, not the user-facing product description.
