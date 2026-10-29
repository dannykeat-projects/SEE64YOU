# Publisher: character GUI and source file workflow

**Status: approved target design; not implemented in the current standard Publisher.** This specification defines screens, controls, file selection, publishing and optional download-to-edit behavior. It is not a user guide for released binaries. The source format is 8TML 1 (20 October 2026 draft); publishing is API 1 (26 October 2026 test draft). The independent Editor, Publisher and Settings remain separate programs, and the external T8WN/8TML ecosystem remains responsible for browsing/rendering.

## Character GUI and selection

**Decision:** use a native C64 40×25 PETSCII character-based GUI (no bitmap graphics framework). Draw simple framed panels, reverse-video highlighted rows/buttons and consistent two-column numbered action buttons. All primary actions are activated by **pressing their displayed digit `0`–`9` or clicking anywhere in the corresponding character-cell hit rectangle**. Arrow keys + Return and optionally joystick direction/Fire also work. Use the native C64 keyboard as the acceptance baseline. A 1351-compatible mouse is optional: the app must remain completely usable without one. The mouse wheel is not required. No hidden mouse-only features. A selected row may be highlighted, but its actions remain explicit.

Screen prototype (40 columns maximum; visual style uses PETSCII graphics and the C64 palette):

```text
+--------------------------------------+
|         SEE64YOU PUBLISHER           |
+--------------------------------------+
| LOCAL FILE                           |
|   index.8tml                         |
| SERVER                               |
|   t8wn.com                           |
| REMOTE TARGET                        |
|   pages/index.8tml                   |
+--------------------------------------+
| [1] LOCAL FILES    [2] SERVER FILES  |
| [3] PUBLISH        [4] TARGET NAME   |
| [5] TEST SERVER    [0] EXIT          |
+--------------------------------------+
| Ready - press number or click        |
+--------------------------------------+
```

Main screen digit mapping never changes: `1` local browser, `2` server browser, `3` publish, `4` remote path editor, `5` connection test, `0` exit with existing machine-state restoration. No separate 'reload settings' home button: Publisher loads stored settings at launch, with an optional refresh after returning from a subview only if an actual requirement is proven. An additional action should displace an existing button only for a real user need.

Local and remote browsers use the **same selection component**, displaying six selectable rows per page. A row is chosen by pressing its number `1`–`6`, clicking the row, or highlighting and pressing Return. The action meanings in the footer are identical on both browsers: `7` type a filename/path, `8` previous page, `9` next page, `0` back. `8`/`9` can be disabled (visually distinct and without effect) when no previous/next page exists. The footer remains visible during scrolling. Show the current folder/path and page, truncating long names only for display; show the full path in a confirmation/details view.

```text
+--------------------------------------+
| LOCAL FILES                          |
| /sd/SEE64YOU/                        |
+--------------------------------------+
| [1] pages/                           |
| [2] index.8tml                       |
| [3] about.8tml                       |
| [4] contact.8tml                     |
| [5] sample.8tml                      |
| [6] notes.8tml                       |
+--------------------------------------+
| [7] TYPE FILE      [8] PREVIOUS      |
| [9] NEXT           [0] BACK          |
| Page 1 of 3                          |
+--------------------------------------+
```

The six rows may represent directories (local; selecting enters the directory) or files (local: load, remote: open an action panel). A visible `..`/parent item is navigable as a selectable row rather than a hidden gesture. The browser should preserve its selection/page and folder when returning from a cancelled action, and should not reset state after recoverable errors. An empty folder/list displays an explicit empty message, with Type/Back still available. Very long directory/remote names are not allowed to wrap over action rows.

**Remote file action panel** after selecting a server file, without automatically modifying any local or remote files:

```text
+--------------------------------------+
| SERVER FILE                          |
| pages/news.8tml                      |
| 1,248 BYTES                          |
+--------------------------------------+
| [1] USE AS TARGET                    |
| [2] DOWNLOAD SOURCE                  |
| [0] BACK                             |
+--------------------------------------+
```

- `1` **Use as target** sets only the remote destination; does not download, upload or alter the loaded local source. Return to Publisher with selected path shown.
- `2` **Download source** goes to a confirmation showing the verified raw-source URL/host, proposed new local filename and existing-file collision status. The source is fetched only after explicit confirmation. If no verified raw-source URL or supported transport exists, disable this button and show a clear reason, not a simulated success or an unsafe guessed URL.
- `0` Back returns to the same server-list page. The selected full path is visible before any consequential action.

**Publish confirmation** is a dedicated panel with full validated remote path, local source/size, endpoint and whether this is a new upload or replacement. `1` confirm / `0` cancel; **never publish on file-row click**. Keep the existing fresh authenticated list/existing-file check before final confirmation, and warn that API 1 has no atomic conditional overwrite. An ambiguous write is reported as `UNKNOWN - CHECK SERVER`, never automatically retried.

**Download result** shows the saved local filename and instructs the user to exit Publisher and open it in `SEE64YOU.PRG`. After Editor saves its edits, the user launches Publisher again, uses `1` to select local source and `2` to choose remote target, then `3` to publish with explicit replacement confirmation. Do not invent an automatic program handoff or persistent remote identity tag in `.8tml` source.

**Text input mode:** pressing `7` on file lists, or `4` on home, opens a dedicated typed-name prompt. Digits there are characters, not navigation shortcuts; `Return` accepts and `RUN/STOP` cancels. In every other menu/picker, digits activate visible controls. Document this clear and consistent mode switch. Use the shortest informative labels that fit 40 columns, never a screen of one-letter command hints.

**Control implementation:** the Editor (`client/src/input/input.c`) already installs cc65's `mouse_static_stddrv` and handles `MOUSE_BTN_LEFT` click edges, mapping x/8,y/8 to screen columns/rows; it also supports joystick events. The independent Publisher (`standard_main.c`) currently uses blocking `cgetc()` and does not initialize mouse/joystick. Extract/reuse only the *minimal* needed polling/pointing logic and draw/hit-test table; avoid dragging unrelated Editor keybindings or large UI code into the Publisher. Match visual button rectangles and hit-test rectangles from the **same** button metadata, with consistent response for click, digit and Enter. Mouse initialization must be optional, checked and properly uninstalled before BASIC return; never assume an attached USB mouse is mapped to 1351 by the firmware. Test a real C64U-compatible mouse as a distinct acceptance gate. Measure linked PRG growth to retain >=1,024 bytes Publisher and launcher headroom.

**Accessibility/reliability:** highlight the focused row or button with reverse video, keep colour contrast, use stable status/footer area, provide clear busy/failure/cancel responses, avoid blocking keyboard controls merely because the mouse driver is missing. Draw only changed rows where beneficial; never redraw continuously for an idle pointer. Never treat button press or click as confirmation of an already successful network operation.

## Server listing, raw-source URL and transport limitations

The current standard publishing API defines `GET ?op=status`, signed `POST ?op=challenge`, signed `POST ?op=list` with `file: SIZE NAME` lines, and signed `POST ?op=put`. **There is no standard authenticated GET/source-download operation and list rows do not supply public source URLs.** Do not infer an arbitrary URL by concatenating the API endpoint, key ID, or remote name, and do not introduce a private API variant.

A successful `put` returns `url:`; it may identify a usable *public source*, but this is not guaranteed for every server or every item in `list`. For downloads, resolve only an explicitly known raw-source URL: one supplied by the user or reliably obtained from a documented server capability / returned publication URL with verified mapping. An optional public source base URL in Settings is a **potential** solution if proven necessary; do not change the fixed settings record or add configuration solely on speculation. A remembered recent publish URL covers only that actual item. If URL discovery is unavailable, tell the user **This server lists files but has no known raw download address** and leave source and local disk unchanged. Test actual T8WN URL mapping and HTTP behavior before adopting any provider-specific shortcut.

The C64U's proven UCI Network `$03` route is **unencrypted HTTP**. It does not implement general TLS/HTTPS, and the service may only expose its public files via HTTPS or redirect HTTP to HTTPS. Refuse TLS-only download; do not follow redirects to arbitrary hosts, downgrade HTTPS, send key material to a public download URL, build a proxy, or pretend the feature works universally. Public raw-source GET should use bounded HTTP read, accept only validated **8TML source**, reject HTML rendering/other file content, check status and framing, and require a complete response. Public downloading must not use publishing HMAC credentials.

For server browsing reuse the `list` parser and request signing; expose an optional bounded entry callback rather than reparsing responses in UI. Each entry contains validated path and size. The server response has a bounded total length already, but a site may return more rows than fit conventional RAM. Implement finite page-window or bounded REU storage only after measuring memory; never silently treat a truncated/partial list as complete. If limits are exceeded, show an error and preserve manual remote-path entry. List before every overwrite confirmation: a prior browse result is **not** a freshness guarantee. API1 `put` remains unconditional; no conditional overwrite or revision protection exists. Never blindly retry an ambiguous upload.

Local browsing uses documented Ultimate-DOS commands **OPEN_DIR `$13` / READ_DIR `$14`** with target `$01` or `$02` as appropriate. `client/src/platform/dos.c` currently lacks wrappers. Directory entry = attributes byte (DIR bit 4) plus filename. Handle empty directory, EOF, malformed entries, hidden/system files, devices/mounts and long paths. Start at configured app folder; show `.8tml` files and navigable folders, allow safe parent traversal, maintain explicit absolute/relative selection. Capture/restore/verify the original DOS path on cancellation, error and program exit; never enter application BIN/manifest files as source. Share the selector's **presentation and navigation logic** with remote browsing, not provider-specific state or protocols.

## Download integrity and recovery

- Never overwrite an existing local document, configuration, recovery record, `.tmp` or `.bak` without specific safe behavior; default to requiring a **new** local filename, suggest basename, offer Save As when occupied.
- Require a resolved URL that actually returns the raw `.8tml` source and is reachable over supported transport; reject unexpected redirects, credentials in URL authority, HTML, invalid framing or unsupported encodings.
- Enforce the local 61,440-byte source ceiling, the shared 8TML source validator, ASCII/LF rules, bounded buffers, and correct `Content-Length`/actual-byte consistency when applicable. Preserve source text and unknown markup (any established CRLF→LF normalization must be explicit). Check a server-provided list size when reliable, but do not substitute that for actual transfer verification.
- Stream to safe scratch or staged local file without holding a second complete document in conventional RAM; only install final file after **complete** source validation and successful close/readback. Reuse existing storage recovery and rename safety rather than creating a new protocol-specific writer. Failures preserve original files and existing recovery temporaries; new incomplete scratch must be dealt with deterministically and visibly.
- Close socket/DOS handles and restore the original DOS directories, colours, IRQ/turbo and responsive BASIC. Preserve credentials; no secret may appear in status, traces or fixtures.
- **No automatic edit handoff**; independent Editor opens the downloaded file on the next launch. Do not add hidden per-document publishing metadata or an automatic remote-identity mapping.

## Code boundaries

- `client/src/publisher/standard_main.c`: replace current L/N/P/T/R `cgetc()` flow with digit-driven GUI event dispatch and character-cell hit-testing; keep rendering/picker separate from protocol, and preserve source load and machine-state restoration.
- `client/src/publisher/standard_transport.c`, `.h` and `standard_reply.c`: signed status/challenge/list/put; list currently scans `file:` for a single name using `output->exists`; add callback/window, preserve all bounds/errors and fresh challenges. Add public GET as a **separate read-only transport** only if resolvable raw source URL is proven.
- `client/src/input/input.c`, `.h`, `client/src/menu.c`: inspect existing Editor mouse/joystick events, click coordinate conversion, driver lifecycle, selection highlighting; share or extract only minimal reusable input event plumbing without making Editor code a Publisher dependency.
- `client/src/platform/dos.c`, `.h`, `dos_path.c`: implement small tested OPEN_DIR / READ_DIR wrapper and preservation/restoration path.
- `client/src/storage/local.c`, `.h`: reuse file/validation/write/recovery semantics for safe downloaded source; do not bypass `.tmp`/`.bak` rules.
- `client/src/document.c`, `buffer_reu.c`, `client/src/platform/network.c`, `client/Makefile`, fit/manifest tools, tests. Check `AGENTS.md`, `docs/PRODUCT.md`, `docs/ARCHITECTURE.md`, `docs/C64U-RUNTIME.md`, `docs/PROTOCOL.md`, `docs/INSTALL.md`, `todo.md`.

**Tracking and acceptance:** Keep the only actionable implementation queue in [todo.md](../todo.md); product architecture/scope is in [PRODUCT.md](PRODUCT.md), existing protocol behavior in [PROTOCOL.md](PROTOCOL.md) and runtime hardware/safety contracts in [C64U-RUNTIME.md](C64U-RUNTIME.md). Do not change releases or claim that planned mouse, selectors or downloads work until matching tests and real-C64U acceptance are recorded.
