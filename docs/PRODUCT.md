# Product scope and 8TML standard migration plan

**Decision (27 October 2026):** SEE64YOU is a small, reliable, offline-first 8TML source editor for the Commodore 64 Ultimate, plus a minimal publisher and settings program using the *standard 8TML publishing API*. It is **not** a website engine, CMS, renderer, or browser. This is the **target design and migration plan**, not a claim that the current code or shipped programs already conform.

**Pinned design input:** user-supplied `8tml-v1.0-specs_26-10-26.zip`: `spec/8tml-1.md` (8TML 1 draft, dated 20 October 2026), `spec/publish-1.md` (publishing API 1 **test draft**, dated 26 October 2026), examples and reference tools. The publishing API may change incompatibly, and the accompanying package says no reference server was implemented at packaging time. The specification files take precedence over website descriptions. The reference tools' redistribution licence is undecided: do not add their code to this repository without permission.

## 1. Product contract

Three independently launched programs, all returning safely to BASIC:

| Program | Responsibility | Required v1 capability |
| --- | --- | --- |
| Editor (`SEE64YOU.PRG`) | Author portable `.8tml` files without an internet connection | New, Open, Save, Save As, edit, search, undo/redo, useful insertion shortcuts including links, and safe recovery |
| Publisher (`S64PUB.PRG`) | Upload one local file to a compatible 8TML publishing endpoint | Discover destination limits, authenticate, choose remote filename, show existing names, create or explicitly replace, show URL/errors |
| Settings (`S64CFG.PRG`) | Manage the one publishing destination and its dedicated credential | Import/validate a standard key file, save settings safely, test the configured endpoint, and allow replacement of a revoked key |

The Editor is the core product and must work without Publisher, Settings, an account, a network or any running server. Canonical documents are literal ASCII/LF 8TML files; preserve headers, unknown markup and source bytes subject to the existing documented line-ending normalization. Do not add conversion to or from earlier proprietary document formats. Preserve explicit manual saving (no periodic autosave), existing working REU-backed buffer/history, collision protection, recoverable incomplete writes, and machine-state restoration.

**A simple end-to-end user journey:** create/open a local file → save `index.8tml` → launch Publisher → select configured destination and remote name → publish → display the returned public URL → use an independent compatible 8TML browser to read the file. Links within pages use standard 8TML relative URL syntax; SEE64YOU does not supply a browser.

## 2. Deliberately excluded

Remove from the *target product and active package*:

- The bundled SEE64YOU PHP website, CMS, post/page categories, drafts, publication-state metadata, themes, generated HTML, preview, site administration, user/account and pairing flows, and bespoke API.
- WordPress integration, optional proprietary relay/managed service, remote site management screens, remote deletion, and preview/import/export functions not needed to edit or publish a normal `.8tml` file.
- The legacy SEE64YOU browser executable/protocol, internal cross-program live-session handoffs, and any abandoned compatibility formats. The external 8TML/T8WN/VVV ecosystem supplies reading.
- Nonessential publisher actions and duplicated documentation, tests, assets and build paths *only after checking actual callers and dependencies*.

Do not replace the removed CMS with another in-repository production server. A **small disposable protocol test fixture** is appropriate for conformance and failure testing while waiting for an independent reference endpoint. Do not make a standalone server, proxy, stylesheet editor, remote download manager, folder manager, or full document renderer a prerequisite for v1. The standard API's `delete` operation exists but the UI does not need to expose it.

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

## 4. Simple Publisher interface and collision policy

Keep the normal path to a few screens: **Choose local file → Choose destination/name → Publish → Result**. When a name already exists, display the destination, path and a distinct **Replace existing file?** confirmation; cancellations and errors must not touch local source. A simple bounded file-name list is for collision awareness, not a general site browser. Default to the local basename when valid, with an editable remote path. Keep Settings separate and persistent.

The test draft's `put` replaces unconditionally and has **no create-only or conditional-revision operation**. Checking `list` then uploading cannot eliminate races or stale overwrites. Do not advertise stronger guarantees and do not add proprietary preconditions to the standard request. Require affirmative confirmation for an observed existing path; if a concurrent update is important, raise a proposal with the standard author for a later protocol revision. After timeout/disconnect, the outcome may be unknown: report this clearly and let the user inspect the remote file, rather than retrying blindly.

Unpublished drafts are just **local files**; the standard endpoint exposes uploaded files publicly. Remote retrieval remains possible by ordinary HTTP through other tools/browsers, but a client-side download/open/edit workflow is **deferred**. Uploading `.8ss` stylesheets and other permitted file types is also deferred unless it falls out essentially free from the same safe file handling. Do not tie `.8tml` editing to server-supported extensions.

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

1. Reuse network, UCI, HTTP, SHA-256/HMAC and local file-reading paths where practical; replace bespoke JSON request/response machinery with bounded standard ASCII protocol handling. Verify memory fit and streaming semantics before deleting shared helpers.
2. Implement `status → challenge → list/put` and standard key-file import/validation. Make failure/cancel leave source, settings and hardware unchanged or recoverable.
3. Reduce Publisher to one guided create/replace action with clear result/URL/error and no unused secondary commands. Keep the three independent BASIC entry/exit flows; don't introduce another launcher/handoff framework.
4. Avoid Editor feature expansion during protocol work. Keep existing working link shortcut, selection and explicit-save behaviour if validated and compatible.

### Phase D — retire old product surfaces, with proof

After tracing callers and proving the new route, remove obsolete PHP site/CMS and associated tests/assets, `relay/` (if unreferenced), WordPress paths, bespoke `source_json` and page/post state, former browser packaging/protocol code, abandoned remote management, duplicated docs and shipping installers. Candidate paths include `site/`, `relay/`, `client/src/browser/`, `client/src/publisher/wordpress*`, `preview*` and `source_json*`; **path names alone are not deletion authority**. Some platform 'managed' modules and tests may still protect Editor state, BASIC return or machine restoration: inspect before changing.

Replace deleted server-integration tests with small standard protocol tests instead of keeping a dormant CMS just to satisfy a test suite. Remove transitive build dependencies and stale references rather than hiding them. Do not edit old release tags/assets or wipe user documents, recovery files, storage or credentials. Old site deployments, if any, require separate explicit deletion authorisation; repository cleanup is not live-site removal.

### Phase E — acceptance and release

Gate implementation in this order:

1. **Static/host:** spec cases, vectors, fuzzed line/size/name bounds, exact source preservation, unknown-markup preservation, settings validation, parser fallbacks and write-failure isolation.
2. **Native/build:** targeted C tests and regressions, full independent SD/selected USB-root package builds, manifest/inventory integrity, Publisher+launcher at least 1,024 bytes free, Editor buffer/overlay fit, correct program launches and BASIC return in VICE.
3. **Real C64U:** physical keyboard, source create/save/open/link/reopen, byte-exact cold restart, bounded network upload/create/explicit replace/error/cancel, stored credential persistence, socket cleanup and original REU/turbo/DOS/IRQ/colours restoration, verified storage hashes and rollback. Existing hardware receipts for legacy packages do **not** certify rebuilt packages. Investigate known USB copy-integrity issues before executing transferred files.
4. **Interoperability:** same local source through the Publisher to a **real independent implementation** of the pinned standard, then ordinary HTTP reading and link navigation by a compatible 8TML browser; check response MIME/paths and warnings. A mock endpoint passing is not external conformance. If the reference server is not yet available, explicitly mark this gate **blocked**, not passed.
5. **Release:** one concise README, installation guide and Editor/Publisher/Settings usage; publish only binaries and matching dependencies actually required. Run checks against exact release bytes, record hash/build/ref and leave prior published releases untouched. No deployment/merge before the required gates pass.

Retain safety checks while removing unused product code. Tests that cease to apply because their feature was intentionally removed should be deleted or replaced, not silently suppressed. Report gates as **passed, failed, blocked or unverified** and make the release claim only what has actually been demonstrated.

## 6. Open questions that do not block the minimal plan

- **Specification churn / licensing:** confirm future publishing API revisions and permission to redistribute its validator. Version-pin each tested implementation; do not claim universal compatibility while the API remains a test draft.
- **Overwrite race:** no standard conditional `put` today. Document the risk; propose a standard extension only if independently supported, never add private API semantics.
- **Reference-server availability:** keep a tiny test double for software work; independent live publishing remains blocked until a real endpoint exists.
- **Styling, downloads and site management:** defer pending real user need. The Editor can already preserve typed 8TML markup and relative links without hosting features.
- **Very large documents / performance:** maintain the current 61,440-byte local ceiling and address proven editing/I/O latency separately, without slowing the protocol cutover.

## 7. Definition of done

A fresh user can install the matching three-program C64U package, create and reopen a portable `.8tml` file offline, import a standard publishing key, upload that same file to an independent standard endpoint, navigate to the reported URL using an external compatible browser, and safely replace it with deliberate confirmation. Interrupted/invalid operations never corrupt local files, stored credentials or machine settings. There is no custom SEE64YOU website, CMS, browser or proprietary publishing API in the shipped product, and remaining code/tests/docs describe only maintained behaviour.

**Tracking:** actionable remaining steps live only in [`todo.md`](../todo.md). This document records architecture, boundaries, sequencing and acceptance, not progress history.
