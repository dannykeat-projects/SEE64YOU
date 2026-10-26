# SEE64YOU work queue

**Target:** offline C64U `.8tml` Editor + minimal standard 8TML Publisher + Settings; no SEE64YOU CMS/browser. The canonical [product scope and migration plan](docs/PRODUCT.md) defines decisions, protocol and acceptance. **This is a plan, not yet implemented or validated.** Existing code and released binaries still support legacy features.

## P0 — Standard API cutover

- [ ] Reconcile current `main` with [draft PR #64](https://github.com/dannykeat-projects/SEE64YOU/pull/64): preserve verified Editor/link/input/safety changes where applicable, identify and avoid merging legacy CMS-only work, and respect its outstanding hardware gates.
- [ ] Inventory call paths and package/release dependencies before deletion: Editor/Publisher/Settings entrypoints, all `client/src/publisher/*`, browser/relay/site code, `client/Makefile`, manifests, tests, package generator and docs. Capture measured ROM/RAM/REU headroom and a keep/retire decision for each active path.
- [ ] Pin the supplied **26 October 2026 8TML publishing API 1 test draft** as the conformance target; add independent HMAC `put`/`list` test vectors and request/response/name/error/size fixtures. Do not vendor reference tools without licensing clearance.
- [ ] Implement bounded HTTP/1.0 standard protocol handling (`status`, `challenge`, `list`, `put`) with trailing streaming HMAC-SHA256, single-use challenges, server-advertised size cap, strict local/remote path separation and useful failures/warnings. Do not mix legacy JSON/API semantics into it.
- [ ] Replace custom Settings pairing/config UX with safe import and durable storage of a standard `8tml-publish: 1` key file (`url`, `id`, `key`); test invalid input, redaction, persistence, key replacement and raw-HTTP consent.
- [ ] Reduce Publisher to **choose file → remote name/site → publish/explicit replace → URL/result**. Use `list` for occupied-name awareness, clearly disclose the standard's absent conditional-write protection, and never blindly retry an uncertain write.
- [ ] Exercise Publisher and Settings against a small disposable standard-protocol endpoint; add missing-error, timeout, partial-write, malformed-reply, HMAC, quota, warning and collision regressions. Independent-server conformance is a **separate** gate.

## P1 — Remove obsolete product and package paths

- [ ] After tracing callers and successfully validating replacements, retire PHP CMS/HTML/themes/posts/pages/drafts/admin/legacy API, WordPress, obsolete relay/managed publishing and their tests/assets. Keep only a minimal disposable protocol test fixture as needed.
- [ ] Remove superseded Publisher handlers and protocols (remote delete/download/list-as-manager/preview/legacy JSON) **only** where not needed for the standard file-name check or shared safety. Preserve common code and rollback behaviour.
- [ ] Verify/remove obsolete bundled browser code and tests; keep browser interoperability as an external test. Preserve any active launcher/preload/overlay references until fit/manifests and real-hardware checks pass.
- [ ] Simplify build/package targets and program inventory; remove unused companion files after caller tracing and matching SD/USB-root build/manifest checks. Preserve the three independent programs and BASIC return.
- [ ] Update README, INSTALL, EDITOR, PROTOCOL, ARCHITECTURE, RUNTIME and release packaging to describe *what users install and do*, not development history. Remove stale CMS directions only when no longer shipping it. Keep prior release assets unchanged.

## P0 release gates — no unverified claims

- [ ] Host/source tests: 8TML preservation, exact HMAC vectors, bounded response parsing, auth/replay, path/name limits, `max-size` mismatch, interrupted upload, warnings, no unintended overwrite and settings write recovery.
- [ ] Native/VICE: full current source/Editor regression, SD and explicitly configured USB-root complete builds, correct manifests/assets, Editor fit and ≥1,024-byte Publisher/launcher headroom, clean BASIC return on normal and failure exits.
- [ ] Real C64U: physical keyboard and link insertion, Save/Open/Save As, cold-power exact reopen, interruption/recovery, standard upload/create/explicit replace/error/cancel with the exact built package, credential persistence, network cleanup, storage readback hashes and complete hardware-state restoration.
- [ ] Independently hosted standard endpoint and external 8TML/T8WN/VVV browser: upload same `.8tml` source, open its public URL, follow a relative link. **Blocked if no real reference-compatible endpoint exists; a stub does not pass this gate.**
- [ ] Exact built release/package audit, checksum/rollback evidence, concise current documentation and release limits. **No merging/deploying unvalidated program/packaging changes.**

## P2 — Only if justified by evidence

- [ ] Investigate long-document Editor operation latency with a bounded reproduction; preserve correctness, history, recovery and responsiveness. Do not combine a speculative rewrite with the publishing migration.
- [ ] Resolve C64U USB-only transfer corruption/readback concerns on the actual firmware/hardware before declaring USB installation supported.
- [ ] Ask standard maintainers about atomic create/conditional replace and redistribution licensing; don't invent private protocol extensions.
- [ ] Consider stylesheet upload, remote download, additional destinations or richer link helpers only after a working standard end-to-end release demonstrates real need.

**Scope guard:** no CMS, custom website, WordPress, alternate document formats, SEE64YOU browser, page/post and draft/published metadata, production server or mandatory proxy. No destructive repo/data cleanup based on filenames alone. Keep all verified but unrelated Editor improvements from PR #64; do not resurrect retired features for compatibility with software that has no established users.
