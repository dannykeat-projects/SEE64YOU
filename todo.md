# SEE64YOU work queue

**Target:** offline C64U `.8tml` Editor, minimal standard Publisher and Settings. [Product scope](docs/PRODUCT.md) pins the 26 October 2026 publishing API 1 test draft. Implementation is on `feature/8tml-standard-core-20261027` / draft PR #66; migration and release acceptance remain incomplete.

## Active Editor priorities

1. **P0 — Save integrity:** preserve existing `.tmp`/`.bak` recovery copies and retain the original through replacement. Host regressions cover collision, backup/install/rollback/cleanup failures. Complete matching SD/USB fit/manifests, VICE and bounded real-C64U disk acceptance before promotion.
2. **P1 — Large-source loading:** measure and batch the per-byte REU writes in the existing source reader; retain byte-exact source, line-ending normalization and bounded memory.
3. **P1 — Editing and saving cost:** benchmark long-distance gap movement and redundant save scans before making a bounded change.
4. **P1 — REU error reporting:** distinguish failed reads from missing characters/search misses and preserve recoverable dirty text.
5. **P2 — Rendering, paste and undo:** use repeatable measurements and fault injection to target redraws and bulk edits.

- [x] Save integrity implementation and focused host fault regressions.
- [x] Save integrity SD and `/USB1/` shipping builds/fit/manifests, both packaged VICE startup/BASIC-return checks, and bounded real-C64U SD native-DOS replacement rollback and recovery-collision acceptance. Exact source/package hashes: `client/build/save-safety-validation.json`; hardware receipt: `client/build/save-safety-hardware-20261029-r2/`. All 44 original files, configuration, DOS contexts, IRQ/turbo/colours, scratch byte and responsive BASIC restored and verified. Physical input, cold power, uncontrolled interruption and real USB-only acceptance remain separate release gates.

## Implemented software

- [x] Preserve PR #64 Editor/input/history/storage improvements through `4e43eef`, plus Editor-only Link work from `e6f4647`; exclude that PR's PHP additions. Fix native Link URL/backslash comparisons and retain extensionless legacy-file behavior outside 8TML mode.
- [x] Run standard `put`/`list` vectors against repository SHA-256/HMAC. Fix the warning promoted by `-Werror` and cc65's reversed CR/LF literals in raw key parsing. Native VICE covers LF/CRLF import, vectors, replies and paths.
- [x] Standard Settings: atomic validated import of endpoint/ID/secret from `8TMLKEY.TXT`, explicit destination tag, no legacy credential fallback/root migration, consent reset, redaction, verified temporary/final persistence, and failed-import preservation. Keep the existing record size.
- [x] Independent Publisher: local file/name, status/max-size, fresh challenge/list/put, streaming trailing signature, explicit create/replace confirmation, URL/warnings, bounded malformed/partial/timeout failures and no automatic retry after an uncertain write. The test draft cannot prevent concurrent overwrites.
- [x] Independent Publisher/Settings capture DOS paths before storage access and restore/verify on exit; block storage when capture fails. Restore original colours; do not alter unrelated turbo/IRQ configuration.
- [x] Exclude legacy JSON/CMS handlers from the independent Publisher link and browser from the ten-file active package. Trace independent Editor callers, exclude unreachable publishing producers, and place formatting in the existing PREP slot using checked return bridges. Keep runtime teardown/recovery and all four assets/manifests.
- [x] Host suite, fixture self-tests, source/storage, Editor stress, native protocol/Link, packaged VICE startup and responsive BASIC checks. Complete SD and `/USB1/` package fit/manifests pass. Logs and exact package bytes are under `client/build/`; these checks do not certify hardware networking.

## P0 â€” Remaining acceptance

- [x] Rebuild/audit exact ten-file SD/USB packages for `45d206934b7ed5841004e86fcf85d1ae077789fe`; hashes in `client/build/standard-validation-45d2069.json`. Only Publisher and its package manifest differ from prior validated bytes. Publisher headroom2047/2041, launcher1070/1056 bytes; both manifests, full host suite, native protocol VICE and both three-program VICE startup/exit checks pass.
- [ ] Finish physical keyboard/Link, cold-power exact reopen, uncontrolled interruption/recovery and external compatible-reader navigation with the exact package; real USB-only acceptance also remains unverified. REST-assisted Editor/Settings/Publisher SD results below do not certify these gates.
- [x] REST-assisted shipping Editor/SD acceptance: Help/exit cancellation, selection/copy/cut/paste/replacement, 35-second no-autosave check, exact-source basename save, recovery/reopen/edit/save, native DOS collision/filename/CRLF and injected write/rename-failure recovery passed. Receipt: `client/build/standard-editor-hardware-20261027-registers/`. All 44 original files, DOS paths, scratch byte827 and captured IRQ/turbo/colours/config/drives/empty BASIC restored. Temporary `C64U Turbo Registers` control followed durable ACTIVE â†’ apply â†’ verified restoration to original Off â†’ durable CLEAN (`client/build/temporary-register-control-20261027.json`). Physical keys, cold power and uncontrolled interruption are separate gates.
- [x] Real-C64U standard Settings: CRLF key-file import, redaction, explicit HTTP consent, verified save/relaunch, invalid-import exact-record preservation, replacement consent reset and cancellation passed using only the public vector credential. Receipt: `client/build/standard-settings-hardware-20261027/`. All original files, DOS contexts and captured machine values restored; no real account credential used.
- [x] Direct real-C64U TCP/HTTP reaches T8wn: read-only API1 status passed. The earlier PC-local fixture timeout remains explained by explicit inbound Python Block rules; no firewall/VPN changes were made, and that optional fixture route is not needed for the successful independent-server tests.
- [x] Independent T8wn host interoperability: standard key parsed privately, API1/HMAC status, challenge/list and disposable create/explicit replacement passed; repository C key parser/signer/SHA-256 accepted by the server. Exact-source HTTP readback and relative index link passed; max-size16384. Private receipt: `client/build/private-t8wn/interoperability-c.json`. This is not shipping Publisher UI acceptance.
- [x] Legacy Publisher diagnostic fit: keep pure configuration validation resident, place the verified REU writer in RUNTIME and use the existing CONTENT return bridge. CONTENT/NETWORK/RUNTIME measure15130/15022/14810 bytes within15360; resident headroom1302 bytes exceeds1024. Asset manifest and boundary tests pass. Generated bridge ABI passes30 VICE swaps with32-bit arguments/results and caller restoration; this is not real-hardware acceptance of legacy transitions.
- [x] Explicit credential-free host command `sh tools/validate.sh --host-only` passes and reports its scope; the default full matrix is preserved. Pin shell scripts to LF for Windows/WSL. Fix the VICE fixture's stale generated bridge names.
- [ ] **Failed historical native matrix:** the next non-shipping managed Editor build exceeds BSS by612 bytes, MODEL_CODE by2489, FILE_RODATA by10 and PUBLICATION_CODE by747. Later historical targets remain unrun. The independent Editor/package is separately validated; do not weaken guards, add another runtime or claim the full matrix passed. Retire or reconcile this legacy target after replacement acceptance.

- [x] Fix verified public-URL reply defect: allow `@` in URL paths while retaining authority credential rejection. The original C64U run safely reported uncertainty after the server committed exact source; no write was automatically retried. At `45d2069`, shipping Settings real-key import/consent/persistence and Publisher create/explicit replace, cancellation of both and byte-exact independent T8wn readback pass. Receipt: `client/build/standard-live-publisher-fixed-hardware-20261027/`. All44 original files, DOS contexts, IRQ/turbo/colours, full configuration values, drives and empty BASIC restored. Separate failure-case receipt `client/build/standard-live-publisher-failures-hardware-20261027/` passes server-size-limit refusal, invalid-key refusal, unchanged remote bytes and complete restoration, including verified scratch827. The16385-byte source loaded in91.87 seconds with Turbo Control Off; the earlier70-second harness wait failed and remains recorded. Physical-input and external-reader gates remain separate.

## P1 â€” Retire obsolete product paths after acceptance

- [x] Trace active/legacy compile, validation and release-package callers; see the keep/retire boundaries in [architecture](docs/ARCHITECTURE.md). Legacy release packaging still emits a PHP site ZIP; shared configuration layout, crypto helpers and recovery remain required.
- [ ] Once replacements pass required hardware/interoperability gates, remove obsolete code/assets/tests and stale server instructions incrementally. CI package count and standard test dependencies are corrected; the historical native overlay failure remains explicit. Preserve user files, credentials/recovery data, shared safety code and existing release assets.
- [ ] Complete exact release/package checksum and rollback audit. No merge, deployment or release while required gates remain failed, blocked or unverified.

## Existing unresolved limits

- [ ] Investigate slow large-document operations and startup verification; measured16385-byte Publisher source load91.87 seconds with Turbo Control Off. Do not combine speculative performance rewrites with protocol migration.
- [ ] Resolve USB transfer/readback reliability and real USB-only acceptance on the actual firmware before recommending USB installation.
- [ ] Confirm draft evolution, independent-server support and validator redistribution licensing; do not invent private conditional-write extensions.
