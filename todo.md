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

- [ ] Complete final exact-source package audit after any further code change; retain ≥1,024-byte Publisher/launcher headroom, Editor resident/overlay fit and matching manifests. Keep SD and USB bytes separate.
- [ ] Finish real-C64U Editor/Settings/Publisher acceptance with the exact rebuilt package: physical keys/Link, Save/Open/Save As, cold-power exact reopen, interruption/recovery, standard create/explicit replace/error/cancel, durable credentials and complete restoration. Earlier PR #64 receipts do not certify this package.
- [ ] **Failed/blocked hardware network gate:** diagnostic reached the C64U but the disposable Windows listener was unreachable from it. A narrowly scoped temporary firewall rule was denied; no rule was created. Network diagnostic exited to responsive BASIC, preserved DOS contexts/IRQ/turbo/colours/config/drives and removed its temporary files. Retry only with a reachable test endpoint.
- [ ] **Hardware Editor timeout:** SD package was backed up/read back exactly, then launcher exceeded the harness's 70-second window with Turbo Control Off. Editor later reached its main screen. Clean exit and restoration of all 44 original files, DOS paths, configuration, drives, IRQ, turbo, colours and responsive empty BASIC were verified in `client/build/standard-editor-hardware-20261027-r2/`. The interrupted harness did not serialize original scratch byte827; that byte is unverified. Do not describe this incomplete editing test as passed.
- [ ] **Blocked independent interoperability:** public `https://t8wn.com/@keato/` serves raw source, but no standard endpoint/key file is available locally. Obtain the account's dedicated publishing key through its HTTPS interface; never paste it into chat. Test a separate disposable remote file, then verify reported URL/relative links in an external compatible browser. A local fixture is not this gate.
- [ ] Review the remaining non-shipping diagnostic/legacy build portion of `tools/validate.sh`; the complete host portion and current shipping builds are separate from the unrun full historical matrix.

## P1 — Retire obsolete product paths after acceptance

- [ ] Finish the keep/retire caller inventory for PHP CMS, page/post/draft metadata, WordPress, relay, old browser/managed publishing, installers and test dependencies. Do not delete by path name alone.
- [ ] Once replacements pass required hardware/interoperability gates, remove obsolete code/assets/tests and stale server instructions incrementally. Preserve user files, credentials/recovery data, shared safety code and existing release assets.
- [ ] Complete exact release/package checksum and rollback audit. No merge, deployment or release while required gates remain failed, blocked or unverified.

## Existing unresolved limits

- [ ] Investigate slow large-document operations and startup verification with bounded timing evidence; do not combine speculative performance rewrites with protocol migration.
- [ ] Resolve USB transfer/readback reliability and real USB-only acceptance on the actual firmware before recommending USB installation.
- [ ] Confirm draft evolution, independent-server support and validator redistribution licensing; do not invent private conditional-write extensions.
