# Product scope and 8TML standard migration plan

**Target product:** SEE64YOU is an offline-first 8TML source editor for the Commodore 64 Ultimate, plus a compact file-selection/publishing companion and Settings using the *standard 8TML publishing API*. It is **not** a website engine, CMS, renderer, or browser. The approved Publisher target includes a numbered 40-column character GUI, local and server browsing, optional compatible mouse control and safe one-file raw-source download-to-edit where a verified server/transport supports it. **These additional GUI/file-workflow capabilities are design, not claims about current binaries.**

**Pinned design input:** user-supplied `8tml-v1.0-specs_26-10-26.zip`: `spec/8tml-1.md` (8TML 1 draft, dated 20 October 2026), `spec/publish-1.md` (publishing API 1 **test draft**, dated 26 October 2026), examples and reference tools. The publishing API may change incompatibly, and the accompanying package says no reference server was implemented at packaging time. The specification files take precedence over website descriptions. The reference tools' redistribution licence is undecided: do not add their code to this repository without permission.

## 1. Product contract

Three independently launched programs, all returning safely to BASIC:

| Program | Responsibility | Required v1 capability |
| --- | --- | --- |
| Editor (`SEE64YOU.PRG`) | Author portable `.8tml` files without an internet connection | New, Open, Save, Save As, edit, search, undo/redo, useful insertion shortcuts including links, and safe recovery |
| Publisher (`S64PUB.PRG`) | Browse local and published remote names, choose a source/target, upload or explicitly replace, optionally download raw source for editing | Numbered PETSCII GUI, optional mouse, bounded selectors, standard publishing API, clear results and collision-safe downloads where independently supported |
| Settings (`S64CFG.PRG`) | Manage the one publishing destination and its dedicated credential | Import/validate a standard key file, save settings safely, test the configured endpoint, and allow replacement of a revoked key |

The Editor is the core product and must work without Publisher, Settings, an account, a network or any running server. Canonical documents are literal ASCII/LF 8TML files; preserve headers, unknown markup and source bytes subject to the existing documented line-ending normalization. Do not add conversion to or from earlier proprietary document formats. Preserve explicit manual saving (no periodic autosave), existing working REU-backed buffer/history, collision protection, recoverable incomplete writes, and machine-state restoration.

**Simple path:** create/open `.8tml` in Editor → save → launch Publisher → select local file → choose or confirm the remote target → explicitly publish → display the returned public URL → read via an independent compatible 8TML browser. **Optional round trip:** browse remote files → download verified raw source to a **new local filename** → exit Publisher → open/edit/save in Editor → return to Publisher → select source and target → deliberately republish. No new document format, embedded publication identity, automatic cross-program handoff or proprietary website. Links use standard 8TML relative URL syntax.

## 2. Deliberately excluded

Remove from the *target product and active package*:

- The bundled SEE64YOU PHP website, CMS, post/page categories, drafts, publication-state metadata, themes, generated HTML, preview, site administration, user/account and pairing flows, and bespoke API.
- WordPress integration, optional proprietary relay/managed service, bulk remote site-management screens, remote deletion, and preview/import/export not needed for ordinary `.8tml` source. A **narrow read-only remote selector and single-file source download** are within the Publisher design only where technically supported.
- The legacy SEE64YOU browser executable/protocol, internal cross-program live-session handoffs, and any abandoned compatibility formats. The external 8TML/T8WN/VVV ecosystem supplies reading.
- Nonessential publisher actions and duplicated documentation, tests, assets and build paths *only after checking actual callers and dependencies*.

Do not replace the removed CMS with another in-repository production server. A **small disposable protocol test fixture** is appropriate for conformance and failure testing while waiting for an independent reference endpoint. Do not make a standalone server, proxy, stylesheet editor, *bulk* download manager, file-management suite, or full document renderer a prerequisite for v1. The standard API's `delete` operation exists but the UI does not need to expose it.

An existing draft PR, [#64](https://github.com/dannykeat-projects/SEE64YOU/pull/64), contains useful Editor, Link, packaging, and publication-safety work alongside legacy PHP page/post and custom-protocol code. **Review it before merging:** carry forward verified Editor/safety improvements that still fit this scope; do not merge or deploy legacy-only additions merely because they are already implemented. Its software receipts are evidence for that branch, not automatic validation of a new protocol or package. It remains draft/unmerged pending its own hardware gates.

## 3. Exact standard publishing contract for the initial client

Implement the 26 October test draft as written; do **not** invent a SEE64YOU protocol dialect:

- `GET ?op=status` (no key): verify `status: ok`, `api: 1`, supported `auth` and `max-size`.
- `POST ?op=challenge&id=ID` with zero-length body: obtain a single-use 32-hex challenge for the HMAC route.
- `POST ?op=list&id=ID&size=0&c=CHALLENGE`: signed request; parse bounded `file: SIZE NAME` lines to identify existing names and allow explicit replacement confirmation.
- `POST ?op=put&id=ID&name=NAME&size=N&c=CHALLENGE`: upload exactly N local bytes followed by 64 lowercase hex HMAC digits, with HTTP `Content-Length: N+64`. Read the returned `status`, `name`, `size`, `url` and any `warning` lines. Never report success solely because the socket closed.

Use HTTP/1.0, a `Host` header, ASCII `key: value` responses, LF lines and the draft's error/status codes. Bound lengths (the spec caps individual reply lines at 255 characters), counts, total response bytes, request URLs and timeouts. Ignore *unknown* keys, not invalid mandatory values. Validate `max-size` and document size before upload; the example 16,384-byte server limit is **not** a universal limit. A larger local document stays editable even if the chosen server declines it. Handle servers converting content and reporting `warning` or a changed `size`; do not silently claim a byte-exact server copy.

Remote names are site-root-relative paths of at most 64 characters, using only `a-z`, `0-9`, `-`, `_` and `.` in segments separated by `/`; no leading slash, empty, `.` or `..` segments. The server lowercases uppercase names. Show the effective destination and preserve the distinction between the local filename and remote path. Never infer an existing remote identity from an unrelated local file.

**HMAC (mandatory for the direct C64U HTTP route):** reuse the existing SHA-256/HMAC primitives but change the signed message to the draft's exact bytes:

    8tml-publish/1 LF OP LF ID LF CHALLENGE LF NAME LF SIZE LF CONTENT

The signature is appended **after** content so it can be computed while streaming; do not copy the file into a second buffer. `NAME` is empty for `list` and `SIZE` is canonical decimal, without leading zeroes. Test both `put` and `list` against the draft's published vectors. Challenges are single-use, including when the request fails. Do not reuse or automatically retry a signed write after an ambiguous network failure.

**Settings key file:** parse `8tml-publish: 1`, `url:`, `id:` and 64-hex-digit `key:`; ignore unknown fields as the standard requires. Reject invalid ID, key, host/path or unsafe storage; avoid secrets in logs, UI, test fixtures and error messages. Keep keys separate from documents; safely replace/revoke credentials in local configuration. The server provisions/revokes keys using its own HTTPS interface, **not** our Settings program. Do not transmit a reusable key or `Authorization: Bearer` over direct HTTP.

**Transport boundary:** on tested C64U firmware the intended direct route is UCI Network target `$03` and plain HTTP with HMAC; HTTP target `$06` was unavailable. Files travel unencrypted, so explain this precisely and preserve explicit user consent. An HTTPS forward proxy / bearer-key mode is permitted by the draft, but **deferred** on C64U: sending bearer credentials in plain HTTP to a proxy exposes the key on that link. Never silently downgrade an HTTPS URL or use a website login password.

## 4. Publisher file workflow and GUI

**[Publisher design and controls](PUBLISHER.md) is the concise interface specification.** A first numbered-key screen and numbered upload confirmation have been committed, retaining typed filenames and legacy letter shortcuts, but are **not compiled or hardware accepted**. Local/remote file selectors and optional mouse are not implemented. Download-to-edit is explicitly **post-initial-release**, contingent on proving independent raw-source access.

- **Home 40×25 PETSCII screen:** framed panels show local source, server and remote path. Numbered buttons are **1 Local files, 2 Server files, 3 Publish, 4 Target name, 5 Test server, 0 Exit**. Press the number or click the matching character-cell button; arrows/Return and optional joystick/Fire also work. Mouse is optional; the native keyboard remains fully supported.
- **Shared six-row file picker:** rows **1–6** select, **7** types a file/path, **8/9** page backward/forward, **0** returns. Local picker uses Ultimate-DOS directory entries to show `.8tml` files and navigable folders. Remote picker uses the standard authenticated `list` response for validated name/size. Preserve complete paths for confirmation, avoid unbounded RAM, permit a manual-name fallback, and never mistake a stale or truncated list for complete state.
- **Remote selection (initial release):** selecting a server item offers **1 Use as target, 0 Back**. Selecting a row never uploads, deletes or downloads. An optional **2 Download source** action can be evaluated after the first release; full validated remote names remain distinct from local filenames.
- **Publish:** show local source/size, destination/remote path and whether a file exists; only explicit numbered confirmation uploads/replaces. Re-check the authenticated list just before confirmation, disclose unconditional `put` race, preserve HTTP consent and report uncertain writes without automatic retries.
- **Later optional download-to-edit (not a release gate):** publishing API 1 has **no download operation**, and `list` supplies no public source URL. Only enable download for a verified raw `.8tml` source URL reachable over supported C64U HTTP, never inferred from a filename or assumed from a rendered HTML page. Save to a **new** collision-safe local name after complete checked transfer, then let the separately launched Editor open/edit/save it. HTTPS-only or inaccessible sources display an honest unavailable message; do not invent a custom protocol or proxy.
- **Consistent input:** visible digits activate controls except within dedicated filename text prompts, where digits are typed. Mouse/joystick remain enhancements, never prerequisites. RUN/STOP cancels or returns safely; restore DOS path, machine settings and responsive BASIC.

Standard endpoint status/challenge/list/put and its HMAC rules stay unchanged. Do not turn this into a remote CMS, file deletion UI or second 8TML browser. The actionable implementation sequence and acceptance gates live only in [`todo.md`](../todo.md).

## 5. Migration and removal strategy

### Phase A — freeze scope and inventory (no destructive deletion)

1. Compare `main`, draft PR #64, active package manifests, readme/install/protocol guidance, current tests and work queue. Establish which PR #64 changes are independent Editor improvements versus legacy CMS-only work; do not discard validated unrelated work.
2. Trace calls from the three standalone program entrypoints, linked objects, overlays and loaded assets, Makefile targets, package generator, test harness, release scripts and site/relay integrations. Measure current Publisher/launcher headroom and Editor fit. Produce a concise keep/retire table with caller evidence.
3. Pin this specification date in tests/documentation (or record externally supplied fixture hashes) without bundling unlicensed reference code. Keep current working builds and immutable published releases recoverable.

### Phase B — specify and prove a standard test boundary

1. Write self-contained fixtures from the published standard, including the HMAC vectors and positive/negative cases for `status`, `challenge`, `list` and `put`. Use a minimal disposable endpoint stub, or a genuine reference endpoint when available.
2. Cover malformed/oversized ASCII replies, unknown fields, HTTP error mapping, name normalization/traversal, wrong auth, expired/replayed challenges, partial request/response, size limits, conversion warnings, content-length and remote atomicity.
3. Use the draft as the sole new protocol contract; do not port page/post JSON semantics or legacy revision extensions.

### Phase C — adapt Publisher and Settings without regressing Editor

1. Retain the already implemented standard `status/challenge/list/put`, HMAC, source serializer, Settings key import and explicit raw-HTTP consent. Preserve independent Editor/Publisher/Settings execution and BASIC return.
2. Add the numbered PETSCII GUI and optional compatible mouse/joystick adapter, then a bounded Ultimate-DOS local directory selector. Keep typed-name fallback, capture/restore DOS contexts and measure program memory fit.
3. Extend the **existing** authenticated list parser to expose bounded validated remote paths/sizes through a shared selector. Maintain a fresh collision check, explicit create/replace confirmation, warning/URL presentation and no retry of ambiguous writes.
4. **After the first usable release**, consider a one-file download only if real independent servers supply verifiable raw-source URLs reachable over C64U-supported HTTP. Where supported, use staged/validated new-file writes and existing recovery safeguards; otherwise do not add a download UI.
5. Validate host/VICE and real C64U physical number keys, paths, network, create/replace and cancellation, plus optional mouse only if it fits. Preserve the hardware and restoration gates. Download-edit-republish is a later independent acceptance task, not an initial release prerequisite.
### Phase D — retire old product surfaces, with proof

After tracing callers and proving the new route, remove obsolete PHP site/CMS and associated tests/assets, `relay/` (if unreferenced), WordPress paths, bespoke `source_json` and page/post state, former browser packaging/protocol code, abandoned remote management, duplicated docs and shipping installers. Candidate paths include `site/`, `relay/`, `client/src/browser/`, `client/src/publisher/wordpress*`, `preview*` and `source_json*`; **path names alone are not deletion authority**. Some platform 'managed' modules and tests may still protect Editor state, BASIC return or machine restoration: inspect before changing.

Replace deleted server-integration tests with small standard protocol tests instead of keeping a dormant CMS just to satisfy a test suite. Remove transitive build dependencies and stale references rather than hiding them. Do not edit old release tags/assets or wipe user documents, recovery files, storage or credentials. Old site deployments, if any, require separate explicit deletion authorisation; repository cleanup is not live-site removal.

### Phase E — acceptance and release

Gate implementation in this order:

1. **Static/host:** spec cases, vectors, fuzzed line/size/name bounds, exact source preservation, unknown-markup preservation, settings validation, parser fallbacks and write-failure isolation.
2. **Native/build:** targeted C tests and regressions, full independent SD/selected USB-root package builds, manifest/inventory integrity, Publisher+launcher at least 1,024 bytes free, Editor buffer/overlay fit, correct program launches and BASIC return in VICE.
3. **Real C64U:** physical keyboard, current numbered GUI activation and file selection once implemented, source create/save/open/link/reopen, byte-exact cold restart, bounded upload/create/explicit replace/error/cancel, stored credential persistence, socket cleanup and original REU/turbo/DOS/IRQ/colours restoration, verified storage hashes and rollback. Mouse is optional and one-file download is a separate post-release gate. Existing hardware receipts for legacy packages do **not** certify rebuilt packages. Investigate known USB copy-integrity issues before executing transferred files.
4. **Interoperability:** upload to a **real independent implementation** of the pinned standard, browse/list its files, and verify source readback and link navigation through an external compatible 8TML browser; check response MIME/paths/bytes/warnings. An optional later raw-source download/edit/republish cycle has its own gate and must not delay an otherwise accepted release. A mock endpoint passing is not external conformance. If the reference server is not yet available, explicitly mark this gate **blocked**, not passed.
5. **Release:** one concise README, installation guide and Editor/Publisher/Settings usage; publish only binaries and matching dependencies actually required. Run checks against exact release bytes, record hash/build/ref and leave prior published releases untouched. No deployment/merge before the required gates pass.

Retain safety checks while removing unused product code. Tests that cease to apply because their feature was intentionally removed should be deleted or replaced, not silently suppressed. Report gates as **passed, failed, blocked or unverified** and make the release claim only what has actually been demonstrated.

## 6. Open questions that do not block the minimal plan

- **Specification churn / licensing:** confirm future publishing API revisions and permission to redistribute its validator. Version-pin each tested implementation; do not claim universal compatibility while the API remains a test draft.
- **Overwrite race:** no standard conditional `put` today. Document the risk; propose a standard extension only if independently supported, never add private API semantics.
- **Reference-server availability:** keep a tiny test double for software work; independent live publishing remains blocked until a real endpoint exists.
- **Raw-source discovery:** the test draft's `list` contains names/sizes, not a source GET URL. Verify source availability and C64U-compatible transport with real servers; disable Download when unavailable. Bulk site management, deletion and styling tools stay out of scope.
- **Very large documents / performance:** maintain the current 61,440-byte local ceiling and address proven editing/I/O latency separately, without slowing the protocol cutover.

## 7. Definition of done

A fresh user can install a matching three-program C64U package, create/reopen portable `.8tml` offline, import a standard publishing key, select local and remote filenames with compact numbered controls, publish to an independent standard endpoint, read using an external compatible browser and explicitly replace remote source. A six-row selector improves this first-release workflow; mouse is an optional enhancement. Raw-source downloads are a separately verified post-release improvement, not part of the initial definition of done. Interrupted/invalid operations never corrupt local files, stored credentials or machine settings. There is no custom SEE64YOU website, CMS, browser or proprietary publishing API in the shipped product, and remaining code/tests/docs describe only maintained behaviour.

**Tracking:** actionable remaining steps live only in [`todo.md`](../todo.md). This document records architecture, boundaries, sequencing and acceptance, not progress history.
