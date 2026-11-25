# SEE64YOU product scope

SEE64YOU is an **offline source editor for the Commodore 64 Ultimate**, accompanied by a small Publisher and Settings program. The Editor supports portable `.8tml` documents, literal `.8ss` stylesheets and general text files. A compatible external 8TML/T8WN browser handles reading pages. SEE64YOU is not a website engine, CMS or browser.

## Programs and workflow

| Program | Purpose |
| --- | --- |
| Editor (`SEE64YOU.PRG`) | Create, open, edit and explicitly save 8TML, 8SS and plain-text sources; preserve unknown content and recovery data |
| Publisher (`S64PUB.PRG`) | Select a local source and remote path, inspect the destination, then explicitly publish or replace it using standard API 1 |
| Settings (`S64CFG.PRG`) | Import/replace a dedicated publishing key, manage one destination and consent to direct HTTP, and save configuration safely |

Each program launches independently and returns to BASIC. The Editor must work without networking, account credentials or any other program. The normal path is **Edit → Save → Publisher → Select source/target → Confirm upload → Read with a compatible external browser**.

Source is literal ASCII/LF text. 8TML requires a draft-1 header and body; 8SS and general text have no imposed header. Preserve unknown constructs rather than generating HTML. CR/CRLF input uses the documented LF normalization. The current local source limit is 61,440 bytes, independent of a server's advertised upload limit. No autosave, hidden remote identity, conversion of older formats or automatic Editor/Publisher handoff. Preserve existing recoverable temporary files, failed-write protection, REU-backed buffers/history and machine-state restoration.

## Editor formats

Use **one existing text-editing engine**, not three editors. File → New presents **1 8TML Document**, **2 8SS Stylesheet**, **3 Plain Text**, **0 Cancel**, in the existing 40×25 keyboard-first interface. The chosen New-file type determines the initial template and default Save As extension. Opening a named file determines its type from the effective suffix; displayed filename/type should be unambiguous.

| Type | New file | Open / save |
| --- | --- | --- |
| `.8tml` | Existing editable 8TML 1 header and body; default `.8tml` | Retain required 8TML header validation, unknown-source preservation and current source/format shortcuts |
| `.8ss` | Start with an empty editable stylesheet unless a pinned 8SS specification establishes a required template; default `.8ss` | Edit literal stylesheet source without treating it as 8TML; preserve unknown constructs. Confirm actual 8SS syntax before adding special validation or commands |
| General text | Start empty; default `.txt` | Allow explicitly named safe text extensions, e.g. `.md`, `.ini`, `.csv`, plus explicitly opened extensionless text; do not require an 8TML header or interpret/render Markdown |

**Filename rule:** A new general-text file entered as `notes` saves as `notes.txt`; `README.md` stays `README.md`. An explicit `.8tml` or `.8ss` suffix selects that format's rules, even when entered in Save As. Do not silently convert, change source bytes or bypass required format checks on a rename. Reject invalid/mismatched formats clearly *before* overwriting anything. Open retains explicit `.tmp`/`.bak` recovery handling and must associate recovered copies with the underlying source type; no silent fallback to a different file or mode. Keep file/path length checks, reserved application files, directory boundaries and collision protection.

**Text boundary:** Continue using the existing REU-backed editing/history, ASCII-to-C64 character mapping, LF storage (normalizing incoming CR/CRLF), 61,440-byte limit, manual saves, temporary/backup files and journal/recovery safeguards. Empty general text files are valid. Reject unsupported binary/control-byte/encoding input safely; arbitrary filename extensions do **not** imply arbitrary Unicode, PETSCII or binary import. Do not claim byte-for-byte preservation across line-ending conversion. All existing editing actions remain shared; 8TML-only Format commands should be hidden/disabled or clearly limited outside 8TML, without adding a second formatter.

**Publisher boundary:** Publisher selects `.8tml`, `.8ss` and `.txt` source and uploads it through the existing standard API. T8wn acceptance has been demonstrated for 8SS and plain text, and for one ordinary 8TML document published through the installed release package on 25 Nov 2026; the reported 8TML upload failure remains under investigation. Host `.md` uploads were rejected with `bad-type`; Markdown and other safe text suffixes remain local. Save As `.txt` allows publishing their literal text without rendering or conversion. Server limits and accepted remote types remain endpoint-specific. External-reader stylesheet application is a separate acceptance gate. No new remote protocol, viewer, live preview, syntax highlighting, encodings or visual stylesheet designer.

**Completion criteria:** Create/open/edit/save/reopen each type, including empty plain text, explicit `.md` and a recoverable `.8ss.tmp`/`.bak`. Verify format-specific rejection and unknown-source retention, current unsaved-document protections, safe Save As collisions and interrupted writes, no changes to existing 8TML fixtures, and exact SD/USB package, memory-fit, host/VICE and physical-C64U acceptance. Work items and evidence belong only in [todo](../todo.md).

## Publisher interface

The initial target is a **small 40×25 PETSCII character interface**, not a GUI framework. Numbered buttons work with native C64 keyboard input; a compatible mouse/joystick may provide the same actions if proven to fit and work on hardware. Typed filenames remain available.

The implemented numbered menu provides local files, remote targets, Publish, Test and Exit. Publication requires a fresh existence check and explicit create/replace confirmation. API 1 has no atomic replacement protection; report warnings and uncertain outcomes without automatic retries.

Controls and validation limits live in [Publisher controls](PUBLISHER.md); remaining work lives only in [todo](../todo.md).

**Download-to-edit is not part of the first release.** Publishing API 1 offers no authenticated source GET and list entries do not supply raw-source URLs. Consider a single safe new-file download only after confirming source access on an independent compatible host via C64U-supported HTTP and checking whether existing 8TML tooling already provides it. Never fetch rendered HTML as source, invent a proprietary API, silently downgrade HTTPS, overwrite an existing local file or make download a prerequisite for publishing.

## Publishing boundary and security

Implement the **8TML publishing API 1 test draft dated 26 October 2026**, with standard status/challenge/list/put requests, a dedicated 8TML key, single-use challenges and streaming HMAC-SHA256. Remote names are validated site-root-relative paths of at most 64 characters; local and remote names are distinct. Honor standard response limits, authenticated list results, server `max-size`, and all returned errors, URL and warnings.

On the tested C64U, direct publishing uses UCI Network target `$03` and plain HTTP. Source bytes are **not encrypted**; HTTP is enabled by default on key import and can be blocked in Settings. Do not transmit ordinary website credentials, silently downgrade HTTPS, embed secret keys in documents or introduce an in-repository proprietary publishing backend. Provision/revoke keys on the independent server's HTTPS interface. The API is experimental: tests against one server do not prove universal compatibility.

The complete wire contract is in [PROTOCOL.md](PROTOCOL.md). Storage and machine-state constraints are in [C64U-RUNTIME.md](C64U-RUNTIME.md).

## Out of scope

No bundled website, WordPress adapter, CMS concepts (posts/pages, themes, drafts, account management), bespoke relay, client browser, remote deletion, bulk remote file manager, visual stylesheet designer, binary-file importer or new session-transfer system. A small disposable protocol test server may be used for validation only. Remove retained unused code only after identifying real callers and establishing safe package/rollback behavior; never delete user files or recovery copies as part of cleanup.

## Release acceptance

- Create, save, reopen and edit identical source offline on a complete matching SD or selected USB build; preserve all local files and recovery on faults.
- Validate the exact program/assets package, manifests, Editor resident/overlay fit and at least **1,024 bytes measured Publisher and launcher headroom**.
- Confirm standard publishing to a real independent compatible endpoint, create/replace confirmation, signed readback, returned URL, errors/cancellations and compatible external browser navigation.
- Verify real-C64U physical keyboard, cold-power reopen, network, storage paths, cleanup, REU/IRQ/turbo/colours and responsive BASIC return. Mark hardware or USB-only steps unverified until tested with exact binaries. Mouse and raw-source download have separate optional acceptance gates.

See [installation](INSTALL.md), [development and tests](LOCAL_TESTING.md) and the active [work queue](../todo.md). Historical receipts and implementation decisions belong in Git and test artifacts, not the user-facing product description.
