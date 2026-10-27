# SEE64YOU work queue

**Target:** offline C64U `.8tml` Editor, minimal standard Publisher and Settings. [Product scope](docs/PRODUCT.md) pins the 26 October 2026 publishing API 1 test draft. Implementation is on `feature/8tml-standard-core-20261027` / draft PR #66; migration and release acceptance remain incomplete.

## Implemented software

- [x] Preserve PR #64 Editor/input/history/storage improvements through `4e43eef`, plus Editor-only Link work from `e6f4647`; exclude that PR's PHP additions. Fix native Link URL/backslash comparisons and retain extensionless legacy-file behavior outside 8TML mode.
- [x] Run standard `put`/`list` vectors against repository SHA-256/HMAC. Fix the warning promoted by `-Werror` and cc65's reversed CR/LF literals in raw key parsing. Native VICE covers LF/CRLF import, vectors, replies and paths.
- [x] Standard Settings: atomic validated import of endpoint/ID/secret from `8TMLKEY.TXT`, explicit destination tag, no legacy credential fallback/root migration, consent reset, redaction, verified temporary/final persistence, and failed-import preservation. Keep the existing record size.
- [x] Independent Publisher: local file/name, status/max-size, fresh challenge/list/put, streaming trailing signature, explicit create/replace confirmation, URL/warnings, bounded malformed/partial/timeout failures and no automatic retry after an uncertain write. The test draft cannot prevent concurrent overwrites.
- [x] Independent Publisher/Settings capture DOS paths before storage access and restore/verify on exit; block storage when capture fails. Restore original colours; do not alter unrelated turbo/IRQ configuration.
- [x] Exclude legacy JSON/CMS handlers from the independent Publisher link and browser from the ten-file active package. Trace independent Editor callers, exclude unreachable publishing producers, and place formatting in the existing PREP slot using checked return bridges. Keep runtime teardown/recovery and all four assets/manifests.
- [x] Host suite, fixture self-tests, source/storage, Editor stress, native protocol/Link, packaged VICE startup and responsive BASIC checks. Complete SD and `/USB1/` package fit/manifests pass. Logs and exact package bytes are under `client/build/`; these checks do not certify hardware networking.

## P0 — Remaining acceptance

- [x] Audit exact ten-file SD/USB inventories and hashes against `client/build/standard-validation-9f7082a.json`; independent application/build sources are unchanged by the CI/harness/documentation follow-up. Retain ≥1,024-byte Publisher/launcher headroom and matching assets/manifests; any later application change requires renewed validation.
- [ ] Finish real-C64U Editor/Settings/Publisher acceptance with the exact rebuilt package: physical keys/Link, Save/Open/Save As, cold-power exact reopen, interruption/recovery, standard create/explicit replace/error/cancel, durable credentials and complete restoration. Earlier PR #64 receipts do not certify this package.
- [x] REST-assisted shipping Editor/SD acceptance: Help/exit cancellation, selection/copy/cut/paste/replacement, 35-second no-autosave check, exact-source basename save, recovery/reopen/edit/save, native DOS collision/filename/CRLF and injected write/rename-failure recovery passed. Receipt: `client/build/standard-editor-hardware-20261027-registers/`. All 44 original files, DOS paths, scratch byte827 and captured IRQ/turbo/colours/config/drives/empty BASIC restored. Temporary `C64U Turbo Registers` control followed durable ACTIVE → apply → verified restoration to original Off → durable CLEAN (`client/build/temporary-register-control-20261027.json`). Physical keys, cold power and uncontrolled interruption are separate gates.
- [x] Real-C64U standard Settings: CRLF key-file import, redaction, explicit HTTP consent, verified save/relaunch, invalid-import exact-record preservation, replacement consent reset and cancellation passed using only the public vector credential. Receipt: `client/build/standard-settings-hardware-20261027/`. All original files, DOS contexts and captured machine values restored; no real account credential used.
- [ ] **Failed/blocked hardware network gate:** both local ports8767 and80 timed out during UCI target `$03`, TCP-open command `$07` (`S64_UCI_TIMEOUT`), before any HTTP write or fixture request. No HMAC/parser claim follows from this failure. A restricted firewall rule was denied and none was created. Socket-only status trace: `client/build/standard-network-hardware-port80-trace-20261027/`. Diagnostics returned to responsive empty BASIC, restored captured state and removed temporary files; listeners stopped. Retry only after TCP reachability is established.
- [ ] **Blocked independent interoperability:** public `https://t8wn.com/@keato/` serves raw source, but no standard endpoint/key file is available locally. Obtain the account's dedicated publishing key through its HTTPS interface; never paste it into chat. Test a separate disposable remote file, then verify reported URL/relative links in an external compatible browser. A local fixture is not this gate.
- [ ] **Failed historical native matrix:** individual cc65 module checks passed, then the non-shipping legacy Publisher overlay failed because `NETWORK_CODE` exceeds its slot by 572 bytes after shared standard Settings validation was added. Later historical targets remain unrun. Independent shipping programs are separately validated; do not weaken guards or claim the full historical suite passed.

## P1 — Retire obsolete product paths after acceptance

- [x] Trace active/legacy compile, validation and release-package callers; see the keep/retire boundaries in [architecture](docs/ARCHITECTURE.md). Legacy release packaging still emits a PHP site ZIP; shared configuration layout, crypto helpers and recovery remain required.
- [ ] Once replacements pass required hardware/interoperability gates, remove obsolete code/assets/tests and stale server instructions incrementally. CI package count and standard test dependencies are corrected; the historical native overlay failure remains explicit. Preserve user files, credentials/recovery data, shared safety code and existing release assets.
- [ ] Complete exact release/package checksum and rollback audit. No merge, deployment or release while required gates remain failed, blocked or unverified.

## Existing unresolved limits

- [ ] Investigate slow large-document operations and startup verification with bounded timing evidence; do not combine speculative performance rewrites with protocol migration.
- [ ] Resolve USB transfer/readback reliability and real USB-only acceptance on the actual firmware before recommending USB installation.
- [ ] Confirm draft evolution, independent-server support and validator redistribution licensing; do not invent private conditional-write extensions.
