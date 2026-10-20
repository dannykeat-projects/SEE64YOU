# SEE64YOU — active 8TML-only implementation queue

**Decision (2026-10-20):** Adopt portable 8TML as the **only** future document format even if T8WN development ceases. No `.s64` documents need to be retained; **no `.s64` support, import, export, conversion, migration or compatibility testing**. Detailed direction, security, staged acceptance and retirement gates: [docs/8TML-ROADMAP.md](docs/8TML-ROADMAP.md).

**Shipped version remains legacy:** `v0.1.0-alpha.1` uses `.s64`, SEE64YOU/1, standalone PHP and the proprietary reader protocol. **Do not claim or advertise 8TML support yet.** Keep the tagged historical alpha release and accurate documentation, but its `.s64` format is not a compatibility target for future builds. Reuse hardware safety code selectively. User guide: [docs/releases/v0.1.0-alpha.1.md](docs/releases/v0.1.0-alpha.1.md).

Earlier work queue and detailed hardware/release receipts are [archived verbatim](docs/archive/TODO-before-8TML-pivot-2026-10-20.md); test evidence remains in [docs/LOCAL_TESTING.md](docs/LOCAL_TESTING.md). Archive is reference material, not active scope.

## Priority 0 — prevent regressions
- [x] Adopt and document 8TML-only clean break; freeze legacy browser/site/WordPress.
- [x] Preserve tagged alpha and historical documents; establish a clean-break implementation roadmap and release instructions.
- [ ] **Release metadata:** edit GitHub `v0.1.0-alpha.1` description to explicitly name `SEE64YOU.PRG` (not internal `S64EDIT.PRG`), clarify its `.s64`/not-8TML status and link to the correct installation instructions. Do not change its tag/assets.
- [ ] Before runtime edits: establish branch/PR baseline and run narrow existing source, storage, network, PHP, packaging and real-hardware gates as relevant. Existing v0.1.0-alpha.1 physical input/cold-power acceptance gaps remain explicitly unverified.
- [ ] Reconcile development branch with older divergent `main` only via reviewed history-safe integration; do not merge implicitly.

## Priority 1 — prove 8TML format conformance
- [ ] Obtain and pin authoritative 8TML draft/version, licence/redistribution permission and golden examples (incl. stylesheets), independent of future T8WN uptime.
- [ ] Map See64You markup/content and PETSCII↔target-byte charset, newline, URL, link, colours and unknown syntax behaviour; mark ambiguities explicitly.
- [ ] Add host-side source read/write, charset and malformed-input conformance fixtures before modifying editor runtime.

## Priority 2 — deliver useful offline 8TML Editor
- [ ] Make `.8tml` the **only** new/open/save file format; remove `.s64` document serialization and old metadata from the new active build after caller tracing.
- [ ] Add conservative verified 8TML formatting conveniences without WYSIWYG scope expansion.
- [ ] Validate only native 8TML save/reopen, byte integrity, error/recovery, collision refusal, BASIC return and machine restoration. **No old-file conversion.**
- [ ] On C64U check save/reopen/collision, recoverability, BASIC return, resource/memory fit and machine restoration.

## Priority 3 — independently hosted reading
- [ ] Validate interoperable `.8tml` pages and stylesheet handling against independent VVV/reference reader; never assume a working prototype.
- [ ] Verify or provide a deterministic safe source-to-HTML static converter; demonstrate own-domain/ordinary-file hosting with no account.
- [ ] Document portability/backups and check readable output on modern browsers.

## Priority 4 — optional remote publishing
- [ ] Agree on a versioned provider-neutral authenticated publishing API or implement an independently self-hostable alternative.
- [ ] Audit and reuse verified C64U Network `$03` and challenge/HMAC pieces where applicable; keep credentials/provisioning secure and never imply plaintext HTTP provides confidentiality.
- [ ] Prove create/update/conflict/download failure and recovery cases on disposable server plus real hardware.
- [ ] Provide a manual upload fallback before any fully integrated network publishing is required.

## Priority 5 — retirement and next release
- [ ] Trace/remove obsolete `.s64`/SEE64YOU/1/site/browser/markup/WordPress from new active build paths after relevant build/recovery/C64U tests. Preserve original tagged release; **no migration gates**.
- [ ] Publish a new versioned, clearly labelled 8TML-compatible release with updated install guide, hashes, supported syntax and known limitations.

**Stop rules:** 8TML only; no old `.s64` compatibility/migration, no unnecessary runtime rewrite, no changes to historical release tag/assets, no unvalidated safety-code removal and no unverified test claims. Work on one bounded milestone at a time.
