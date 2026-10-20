# SEE64YOU — active 8TML migration queue

**Decision (2026-10-20):** Adopt portable 8TML as our long-term publishing standard even if T8WN development ceases. Detailed direction, security, staged acceptance and retirement gates: [docs/8TML-ROADMAP.md](docs/8TML-ROADMAP.md).

**Shipped version remains legacy:** `v0.1.0-alpha.1` uses `.s64`, SEE64YOU/1, standalone PHP and the proprietary reader protocol. **Do not claim or advertise 8TML support yet.** Preserve its eleven-file client and PHP site packages, binaries, URLs, keys and hardware safety. User guide: [docs/releases/v0.1.0-alpha.1.md](docs/releases/v0.1.0-alpha.1.md).

Earlier work queue and detailed hardware/release receipts are [archived verbatim](docs/archive/TODO-before-8TML-pivot-2026-10-20.md); test evidence remains in [docs/LOCAL_TESTING.md](docs/LOCAL_TESTING.md). Archive is reference material, not active scope.

## Priority 0 — prevent regressions
- [x] Adopt and document 8TML-first strategy; freeze new development of legacy browser/site/WordPress.
- [x] Preserve released alpha and original source/docs; produce migration roadmap and release-specific instructions.
- [ ] **Release metadata:** edit GitHub `v0.1.0-alpha.1` description to explicitly name `SEE64YOU.PRG` (not internal `S64EDIT.PRG`), clarify its `.s64`/not-8TML status and link to the correct installation instructions. Do not change its tag/assets.
- [ ] Before runtime edits: establish branch/PR baseline and run narrow existing source, storage, network, PHP, packaging and real-hardware gates as relevant. Existing v0.1.0-alpha.1 physical input/cold-power acceptance gaps remain explicitly unverified.
- [ ] Reconcile development branch with older divergent `main` only via reviewed history-safe integration; do not merge implicitly.

## Priority 1 — prove 8TML format conformance
- [ ] Obtain and pin authoritative 8TML draft/version, licence/redistribution permission and golden examples (incl. stylesheets), independent of future T8WN uptime.
- [ ] Map See64You markup/content and PETSCII↔target-byte charset, newline, URL, link, colours and unknown syntax behaviour; mark ambiguities explicitly.
- [ ] Add host-side source read/write, charset and malformed-input conformance fixtures before modifying editor runtime.

## Priority 2 — deliver useful offline 8TML Editor
- [ ] Add `.8tml` new/open/save/export as true source bytes; keep the current `.s64` path supported.
- [ ] Add conservative verified 8TML formatting conveniences without WYSIWYG scope expansion.
- [ ] Add non-destructive `.s64` conversion with warnings; verify exact byte preservation where supported.
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
- [ ] Trace and measure unused code/build/docs; archive/remove old site/browser/markup/WordPress only once migration, rollback and relevant regression tests pass.
- [ ] Publish a new versioned, clearly labelled 8TML-compatible release with updated install guide, hashes, supported syntax and known limitations.

**Stop rules:** no broad runtime rewrite to pursue format conformance, no changes to an existing release tag/assets, no removal of working alpha paths without demonstrated replacement, no unverified success claims. Work on one bounded milestone at a time.
