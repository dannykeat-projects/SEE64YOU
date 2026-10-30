# Publisher interface and file workflow

**Scope:** one small independently launched C64 Ultimate Publisher for standard 8TML source. This is a target design, not a description of completed software. Preserve existing standard API 1 status/challenge/list/put, dedicated-key Settings, HTTP consent and explicit replacement protection. No CMS, browser, remote deletion, proprietary server, graphics framework or cross-program handoff.

## Release priorities

1. **First:** finish the independent Editor and standard Publisher acceptance (including exact-package real-C64U tests; PR #67 loading optimisation has separate acceptance and is excluded from the current alpha). Do not merge or promote unverified binaries.
2. **Next:** a compact numbered character interface and a practical local file selector. Use a few screens and existing routines, not a reusable GUI framework.
3. **Then:** display/select names from the existing standard authenticated server list, with manual remote name still available.
4. **Later, optional:** download a single file for editing, but only when independent raw-source retrieval and supported C64U transport have been demonstrated. Download is **not a blocker** for the first usable release.

Mouse input is a convenience, not a dependency or release blocker if its implementation would violate memory-fit requirements. Native keyboard must always work. Only reuse a minimal part of the Editor's existing mouse/event logic; no large shared UI library.

## Interface

On the 40x25 C64 screen, use PETSCII line-drawing characters, reverse-video highlighting, two or three short information rows, and numbered actions. Drawing and hit-testing use the same button coordinates. Keep display truncation separate from the actual stored filename/URL.

Target home screen:

~~~text
+--------------------------------------+
|         SEE64YOU PUBLISHER           |
+--------------------------------------+
| LOCAL    index.8tml                 |
| SERVER   t8wn.com                   |
| TARGET   index.8tml                |
+--------------------------------------+
| [1] LOCAL FILES    [2] SERVER FILES  |
| [3] PUBLISH        [4] TARGET NAME   |
| [5] TEST SERVER    [0] EXIT          |
+--------------------------------------+
| Ready - press number or select       |
+--------------------------------------+
~~~

Press an on-screen number to activate that action; optional C64U-mapped 1351 mouse click selects its text rectangle, or arrows/Return and optionally joystick/Fire operate the focused entry. RUN/STOP cancels or safely exits. In a dedicated filename-entry prompt, digit keys type characters rather than invoke actions.

**Current incremental state:** PR #66 has a numbered-key home screen (1 Local, 2 Browse remote, 3 Publish, 4 Test, 0 Exit), numbered upload confirmation and a six-row paged local `.8tml` selector with manual-name fallback. Directory commands use a separately linked module, preserving Editor fit. Authenticated server selection is implemented; new remote controls await exact-package hardware acceptance. Local folder traversal and optional mouse are not implemented. Packet-stream host fixtures, complete SD/USB cc65 fit/manifests and VICE startup/exit checks pass. REST-assisted real-C64U SD tests pass numbered source selection/filtering and independent-server create/replace/cancel, three-identity confirmation, byte-exact readback, size-limit and invalid-key refusal. Publisher retains at least 1,024 bytes of headroom. Physical number keys, cold-power/interruption and real USB-only operation remain unverified. The target layout above is not the implemented menu.

## Local and server selectors

Keep one six-row, numbered visual selector with actions: rows 1-6 select; 7 Type name/path; 8 Previous page; 9 Next page; 0 Back. Use clear empty/loading/error states, return to the same selection on cancellation, and show the complete chosen path before publishing. Never treat clipped display text as a filesystem name.

**Local:** start in `SEE64YOU/docs/`, show ordinary .8tml sources and navigable directories, suppress settings/program/recovery files from the default list, and allow safe parent navigation within the chosen mount. Ultimate-DOS directory iteration needs bounded OPEN_DIR/READ_DIR support, including error/EOF tests. Preserve existing DOS target/path capture, restore and verified source loading. Keep typed-name fallback.

**Server:** parse the current standard signed list response (file: SIZE NAME) using the existing transport/parser, with a bounded callback/window; do not duplicate signing or assume a short list. Six rows plus manual path and paging are sufficient. Selecting a remote file offers **1 Use as target / 0 Back** in the first release. It does not write anything. Fresh-list collision check immediately before a separate **1 Confirm / 0 Cancel** upload/replace action is still required, and the server's unconditional-put race remains visible.

Mouse support must fit without weakening the existing required 1,024-byte Publisher and launcher headroom or other Editor/machine-state constraints. If optional mouse code does not fit, ship the keyboard-first selector and investigate a smaller input adapter separately.

## Download-to-edit, after the initial release

API 1 defines no download operation: a list entry contains name and size, not a guaranteed raw-source URL. An upload response URL is not proof that arbitrary listed files are downloadable. Before implementation, verify a documented exact raw 8TML URL, content type/bytes and reachable HTTP transport on an independent server. Do not guess URL paths, fetch rendered HTML as source, silently downgrade HTTPS, send publishing secrets in public GET requests, or build a custom proxy.

When this is proven, the optional server-file action may become **1 Use as target / 2 Download source / 0 Back**. Save only to a **new** explicitly confirmed local filename, check size (local 61,440-byte cap), source bytes, completeness, transfer errors, staged storage and collision/recovery protection. The Editor then opens and edits the downloaded file as an ordinary local document. The user manually republishes, confirming the target; never store proprietary remote identity in the 8TML source. An unsupported server simply disables Download with an explanation.

## Acceptance

Start with narrow host unit/fixture tests; then full matching SD/USB build/fit/manifest and VICE responsive BASIC-return checks. Real C64U acceptance verifies physical number keys, paths and cancellation, optional configured mouse, byte-exact source, server listing and create/replace, failure paths, and preservation of files/settings, DOS, REU, colours, IRQ/turbo and BASIC. Keep unfinished hardware gates marked unverified or blocked, never passed. Only perform source download/Editor round-trip acceptance if that feature is independently proven and implemented.

**Actionable work lives in [todo.md](../todo.md); broader scope is in [PRODUCT.md](PRODUCT.md).**

## Current remote selector

Home **2 Browse remote** uses the authenticated standard list. Six exact remote targets are available per page; **1–6** select, **7** sets a typed name, **8/9** change pages, **0** or RUN/STOP returns. Listing failure offers manual entry. Selecting changes only the target; Publish still checks existence again and requires confirmation. API 1 has no delete operation. Paging repeats a bounded authenticated list request and can reflect concurrent remote changes. Physical input and exact-package C64U acceptance remain unverified.
