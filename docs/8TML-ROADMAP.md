# SEE64YOU: 8TML direction and migration plan

**Decision date:** 2026-10-20  
**Status:** Approved product direction; migration **not yet implemented**  
**Source of truth for active work:** [../todo.md](../todo.md)  
**Shipping alpha:** [v0.1.0-alpha.1](https://github.com/dannykeat-projects/SEE64YOU/releases/tag/v0.1.0-alpha.1), still uses SEE64YOU/1 and `.s64`.

## 1. Product decision

SEE64YOU will be an offline-first, keyboard-oriented text editor and publishing application for the Commodore 64 Ultimate (C64U). **Portable 8TML documents are the future canonical publishing format.** The editor will support the published 8TML standard, independently of who hosts sites or who maintains T8WN or the VVV browser. A useful offline editor and local files must survive the loss of any external project or service.

The project is **not** a general-purpose HTML browser, WordPress replacement, hosted social service, or competing long-term markup language. Support the C64U's REU, UCI, Ultimate-DOS and permitted turbo capabilities without constraining the design to a stock C64. Maintain tested input, storage, recovery, BASIC-return and hardware restoration behaviour.

### Non-negotiables

- One portable, readable, versioned authoring format: `.8tml`. No required T8WN account, domain, server, browser, API or central registry.
- Users own their content and can back it up, copy it and host it under their own domains.
- Keep the editor fully useful without a network. A failed or absent publishing service must never make local documents unusable.
- Treat static **reading** and authenticated remote **writing** as separate problems. A web server serving `.8tml` does not automatically provide a safe publishing API.
- Preserve all existing user `.s64` documents and the functional v0.1.0-alpha.1 binaries/sites throughout migration. No in-place destructive conversion or silent format reinterpretation.
- Do not make external project's draft proposals contractual until backed by an identifiable specification revision and conformance fixtures.

## 2. Verified baseline vs external dependencies

### Current, released SEE64YOU functionality (legacy)

The alpha has four independently launched programs: `SEE64YOU.PRG` (Editor via internal assets), `S64PUB.PRG` (Publisher), `S64CFG.PRG` (Settings), `S64WEB.PRG` (read-only SEE64YOU-site browser). It saves `.s64`, uses SEE64YOU/1 source/markup and the SEE64YOU-BROWSE/1,2 wire protocol. A separate PHP 8.1+ flat-file site renders HTML and accepts direct C64U HTTP publishing authenticated using challenge/HMAC; that protects integrity/authentication **not confidentiality**. The alpha **does not support 8TML, VVV, or T8WN publishing**. It requires the matching eleven-file package, UCI/Ultimate-DOS and a 16 MiB REU. This is an early alpha with incomplete physical acceptance; see [releases/v0.1.0-alpha.1.md](releases/v0.1.0-alpha.1.md).

The existing C64U SD storage, PETSCII conversion, editor source buffer, formatting, undo, verified save/restore, network transport and security code are candidates for reuse, not automatic replacements.

### External 8TML ecosystem

The external developer described 8TML as a simplified Markdown-like static text format, a VVV browser, optional stylesheets, a possible 8TML-to-HTML converter, self-hosting and a planned publishing API. The discussions establish **intent**, not verified releases or a frozen interoperability contract. As of this decision, upstream specification bytes/version, licensing, charset, exact syntax, URL/path handling, stylesheets and publishing API have **not been pinned and tested in this repository**. Do not invent syntax or call the integration complete. Reference: https://t8wn.com/8tml/ (availability may vary).

### Independence if T8WN stops tomorrow

A document will remain a normal `.8tml` file. We will keep a publicly documented, testable compatibility profile/fixtures (subject to permission to redistribute upstream specification text); our own import/export and validator will not require an online service. For HTML accessibility, a small reproducible local/static conversion tool is a contingency **only if** no maintained compatible converter becomes available. A user should be able to serve source files on any ordinary static host and produce an HTML mirror without depending on T8WN accounts or infrastructure.

Avoid claiming all old machines can request HTTP/HTTPS on every host: networking and TLS vary by client.

## 3. Target boundaries

```text
C64 Ultimate
  Editor (offline, native PETSCII buffer)
     | save/open/export/import with checked charset conversion
     v
  *.8tml  (canonical, portable and recoverable on SD)
     |
     +--> manual file copy / static hosting --> 8TML reader(s)
     |
     +--> optional Publisher using a versioned write API
            | C64U UCI $03 / tested transport
            v
         self-hosted server or compatible provider --> *.8tml
            |
            +--> static HTML converter --> index.html for modern browsers
```

**Editor:** preserve modern keyboard conveniences, predictable save/open/recovery, source formatting commands, large-file handling within measured bounds, and reversible document operations. Keep format rules out of input/drawing code; add a small 8TML document/encoding boundary. No markup WYSIWYG renderer required for the first milestone.

**File format:** content is an 8TML source file, not a PHP-guarded or SEE64YOU-specific envelope. Store local-only metadata in a separate sidecar **only if truly needed**; never leak credentials into a publishable document. Don't unilaterally add nonstandard directives. Keep `.s64` import and a legacy export/restore escape route during transition.

**Publisher:** optional separately launched client. Read file, select destination, authenticate safely and publish/update with explicit overwrite/version intent. Network transport, destination API and file serialization must be separate modules. Never embed a universal service dependency in the editor.

**Hosting/HTML:** first target is ordinary static hosting with a documented converter that turns the same `.8tml` source into HTML; no database or PHP runtime required for read-only pages. Dynamic write/upload API is optional and independently installable. Preserve current PHP site as a legacy compatibility implementation until migration is proven.

**Browser:** freeze new features for `S64WEB.PRG` and keep it runnable for alpha users. VVV and other readers are independently maintained. Implement a new reader only if a demonstrated unsupported user need justifies maintaining it.

## 4. Compatibility work before editing format code

1. Obtain a dated/versioned authoritative 8TML specification and representative source/style fixtures from upstream; record URL, hash, licence or permission and known unanswered questions. Request permission before copying upstream prose/code into this repository.
2. Build a **feature matrix** mapping legacy SEE64YOU Markup v1 to 8TML: headings, emphasis, lists, links, escapes, whitespace, stylesheets/colours, long lines, unknown directives, paths and forms. Mark each as identical / transformable / unsupported / ambiguous, with fixture and expected result. **Do not assume full Markdown compatibility**.
3. Define and verify the public byte contract: encoding, permitted characters, line endings, case, PETSCII mapping (including inverse), BOM, tabs, controls, filenames, slugs, relative URLs and maximum lengths. The current `.s64` wire uses 7-bit ASCII/LF; that is NOT automatically the 8TML specification. Reject unsupported bytes with an actionable error rather than silently dropping/replacing them.
4. Define version detection and update policy. Preserve unknown content on open/save and do not silently rewrite files to newer semantics. If the draft changes incompatibly, pin the last verified profile and continue supporting it while assessing migration.
5. Implement tests that round-trip source **byte-for-byte where defined**, including blank lines, newline endings, escaped characters, PETSCII/ASCII conversions and malformed/missing-markup cases. Add fixtures covering rendering in independent 8TML implementation when available.
6. Verify coexistence with existing .s64: detect by extension/known envelope, import into a **new** `.8tml` with preview/diff and unconverted-element warnings; keep original unmodified. Don't assume `.s64` can simply be renamed.

## 5. Publishing protocol and security

Do not equate 8TML with a single upload mechanism. A complete deployment may be as simple as a user copying source files to a static web folder; direct C64U remote publishing needs separate support.

- First define a narrow provider-neutral operation set: probe/capabilities, list (optional), get source, create/update with precondition, conflict/error codes, and delete only if needed. Work on explicit host paths and stable identifiers. Require idempotency or clear retry rules and no accidental overwrite.
- Test whether T8WN's announced API can support these operations. If it does not materialise or is unsuitable, define a small documented **optional** self-hostable endpoint. Keep file format and endpoint protocol independently versioned. No provider-specific credentials inside `.8tml`.
- Prefer authenticated HTTPS for secrets and private content; C64U firmware-1.1 Network Target `$03` supports raw HTTP/TCP but should not be presented as general-purpose TLS. Preserve the known good dedicated-key challenge/HMAC path as a possible **compatibility adapter** only after assessing replay, body binding, key rotation and server support. HMAC over plaintext HTTP does **not** provide confidentiality or protect against all active network interference.
- Pair/provision sensitive keys in a trusted environment over HTTPS. Never send ordinary website passwords/bearer tokens over C64U plaintext HTTP. Never silently downgrade HTTPS to HTTP. If a secure C64U path is unavailable, offer manual publishing / explicitly trusted local relay / informed limited public-content mode; do not promise universal compatibility.
- Bound memory, request/response sizes, redirects, timeouts and resource use. Validate paths/host allow-lists and prevent traversal/SSRF; publish only explicitly selected files. Fail closed on auth/replay/signature problems and preserve local source.
- Test failed writes, mid-transfer interruption, unchanged remote content on failure, collision handling and hardware state restoration on exit. Do not expand the auth surface during a simple format change.

## 6. Delivery stages (sequential gates, not dates)

### Stage 0 — direction/documentation (this change)
- [x] Record 8TML-first product decision and independence requirement.
- [x] Freeze new legacy browser/site/WordPress features; retain existing source, release and recovery docs.
- [x] Archive the pre-pivot active queue and replace it with migration milestones.
- [x] Add accurate release-specific installation and known-limit instructions.
- [ ] Update the **GitHub Release description** to link the guide and clarify editor entry point, if the available authenticated release-editing tool permits (not the same as editing a repository Markdown file).

**Exit:** current alpha remains installable and unchanged; readers can distinguish current product from target design.

### Stage 1 — documented 8TML source contract
- [ ] Pin versioned upstream spec/fixture/licence evidence; request clarification as needed.
- [ ] Produce feature/encoding matrix and compatibility policy.
- [ ] Add host-side golden fixtures/validators and bounded error cases without touching shipped binaries.

**Exit:** representative fixtures and byte conversion rules are reproducible without a T8WN service. No unverified parser features.

### Stage 2 — offline 8TML editing
- [ ] Minimal change to existing editor to open/create/save/export portable `.8tml`; preserve `.s64` workflow.
- [ ] Add 8TML formatting commands only as verified by the pinned spec; unsupported constructs preserved as source.
- [ ] Safe `.s64` to `.8tml` *copy* migration, with explicit loss/unsupported warnings; no overwrite by default.
- [ ] Rebuild exact matching C64U package with source provenance and hashes.

**Exit:** on real C64U: edit/save/reopen, reboot, round-trip bytes, collision/recovery/error cases, unchanged original .s64 and machine settings all pass. User may already publish `.8tml` manually.

### Stage 3 — independent static presentation
- [ ] Validate served `.8tml` loads in independent VVV/other browser on its stated hardware, or record as unverified if inaccessible.
- [ ] Validate deterministic `.8tml` -> HTML conversion, safe escaping, links/navigation, styles and mobile-friendly output; use a small independent converter only if existing one isn't fit.
- [ ] Publish example multi-page static site from an ordinary host with **no proprietary hosting dependency**.

**Exit:** identical source content usable in supported 8TML clients and modern browsers; offline conversion and independent hosting demonstrated.

### Stage 4 — optional authenticated remote publishing
- [ ] Pin interoperable API or ship a small documented optional self-host endpoint, after security review.
- [ ] Reuse measured C64U UCI transport/auth pieces where practical.
- [ ] Establish safe provisioning, update-conflict behaviour, retries, recoverability and no plaintext password.
- [ ] Test first-time user workflow on disposable host and real C64U.

**Exit:** edit → publish → verify → re-download/edit/update works and preserves source and settings. Any security limitation is prominent and user-approved.

### Stage 5 — simplification and new release
- [ ] Audit runtime callers, build targets, docs and tests; remove dead paths only with clear proof and affected regression/hardware gates.
- [ ] Retire standalone SEE64YOU markup, proprietary browser output, legacy website and WordPress integration **only after** migration/export and rollback are proven and a separately versioned release is available.
- [ ] Clean packaging, onboarding, support matrix and independent hosting docs.
- [ ] Prepare migration notes and an appropriate new versioned release; do not overwrite `v0.1.0-alpha.1` assets/tag.

**Exit:** user can upgrade, keep old files, and revert using preserved binaries/content; no orphaned data.

## 7. Specific retention/archive decision

| Existing source/docs | Decision now | Retirement condition |
| --- | --- | --- |
| Editor, buffer, input, REU, DOS, launcher and recovery | Keep unchanged | None: reuse and adapt safely |
| `.s64`, SEE64YOU/1 and `docs/MARKUP.md` | Frozen legacy; still supported by alpha | Lossless/as-declared migration and compatibility tested |
| Current PHP `site/`, `docs/SITE.md` | Freeze new development, keep deployable | Safe 8TML static publishing and migration/backup verified |
| `S64PUB.PRG`, Settings, UCI and HMAC transport | Keep; assess narrow adapter | New API path passes real-hardware/security tests |
| `S64WEB.PRG`, SEE64YOU-BROWSE/1,2 | Freeze; retain alpha | Independent reader demonstrated and support expectations addressed |
| `wordpress/`, `relay/` | Deferred; preserve source | Explicit evidence no callers or user obligation |
| Pre-8TML `todo.md` | Archive verbatim under `docs/archive/` | Retained for provenance, not a live queue |
| `docs/C64U-RUNTIME.md`, `docs/LOCAL_TESTING.md` | Active safeguards, keep | Update only with verified behaviour |

No code deletion is justified at Stage 0: active alpha binary integration and recovery tests would have to be rerun. Removing it solely because future architecture changes is not a simplification.

## 8. Validation and change control

Before implementation inspect AGENTS.md, current branch/PRs, tested alpha commit/binary hashes, build manifests, live callers and pending changes. Baseline narrow host tests; add fixtures; rebuild; run editor/storage/auth/site regressions relevant to changed paths; run packaged VICE and bounded real-C64U acceptance when hardware is required. GitHub Actions budget may be unavailable, so label each check **passed / failed / blocked / unverified**, never assume CI ran.

Preserve files and live production. Don't commit generated binaries unless release process expects version-matched artifacts; do not replace existing release assets. Keep `main` reconciliation separate: it diverges significantly from the actual alpha development history. No force push, history rewrites, implicit merge/deployment or destructive cleanups. If a migration gate fails, keep the released alpha operational and stop that stage rather than attempting a large rewrite.
