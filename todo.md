# See64You TODO

This is the **only live work queue**. Updated 2026-10-08 after the MVP/simplification review of code head `34e75807c54a9f36772245539e6eeefa7e51d4be`.
Read the current source and PR head before acting; recorded measurements are scoped evidence, not current-build certification.

## Objective and continuation instructions

Deliver a usable C64 Ultimate-only, 16 MiB REU MVP before expanding features:
**write → local save/reopen → publish → browser display → remote reopen → update → return to editing → safe exit**.

Continue on `feature/c64u-publisher-chain`, draft [PR #61](https://github.com/dannykeat-projects/SEE64YOU/pull/61). Inspect Git status, active PRs, AGENTS.md, this queue, docs/C64U-RUNTIME.md, docs/ARCHITECTURE.md and docs/LOCAL_TESTING.md first. Preserve unrelated work and recheck remote head before writing. Do not merge/deploy merely because CI passes.

The 2026-10-08 documentation change adopts the reviewed plan; it does not validate a new package or complete implementation tasks. Historical troubleshooting belongs in Git history, durable contracts/evidence in canonical docs. Do not create another live plan or copy this queue into a new handover file.

## Current checkpoint

- `make -C client c64u-package` still uses the whole-PRG managed-handoff package. The persistent-runtime candidate is separate diagnostic/build work, not yet the accepted product.
- Proceed with one persistent Editor-owned runtime and callable Publisher/Settings overlays. Retain the proven Launcher → Editor bootstrap and original BASIC-return path; do not redesign the launcher as resident.
- Recorded software evidence includes a 30-cycle persistent lifecycle test and focused real-code gates for authenticated save/delete, document writer, Preview, staged remote list/open, Publisher UI and Settings storage/UI slices. The combined harness still uses some placeholders; separate passing gates do not prove complete product integration.
- Last recorded Editor + Publisher resident margin: **1,340 bytes**; PNETWORK: **14,397 / 15,360 bytes**; projected allocation: **14 / 16**. These are partial-build measurements: remeasure the complete linked Settings/UI/package.
- Persistent Settings control/rendering work has advanced beyond the first measured slices. Inspect current `persistent_settings_*` source and fit helpers rather than restarting extraction from an old checkpoint.
- Prior hardware evidence proves components, including earlier upload/reopen and Launcher → Editor → BASIC. The current complete persistent-runtime user workflow remains **unverified on hardware**.
- Fallback reference: code `e5ffd0f`, checkpoint `6ccf818`. It is not an end-to-end working MVP: hardware isolated whole-PRG Publisher application-state loading/validation failure. Keep fallback/tests until replacement acceptance; do not resume that separate debugging track by default.
- Config is authoritative at `/sd/SEE64YOU/SEE64YOU.CFG`, with existing verified legacy-root migration/fallback. Journals remain at SD root. Do not relocate journals during MVP work.

**Active blocker:** finish wiring and packaging the real persistent Publisher/Settings path, then prove it on hardware. Next action: validate the preservation handover with cc65/VICE and confirm fit before wiring the real Publisher/Settings loop.

2026-10-09 prerequisite audit of `9609a657c80926047d4b7b7ebe7df1525fd6fa77`
(verified draft PR #61 head): Editor Publish still calls the success-only
`persistent_runtime_fit_mode.c` placeholder. No complete persistent candidate is
packaged. A new host regression using real Editor/buffer/history/staging code
reproduces source and undo-payload overwrite during an incomplete remote-open
receive, without any commit. See LOCAL_TESTING.md for the command and gate scope.
Hardware acceptance is blocked; no SD files or machine settings were touched.

2026-10-09 preservation implementation (handover; C64 validation pending): isolated allocator
scratch plus verified backup/rollback makes the expanded host regression pass.
The supplied audit files were preserved and extended. Nearby REU buffer/history/
clipboard gates pass; required host validation now includes the regression in both
validators. No complete package or new measured fit exists. Next: run cc65 Publisher
fit and staged-open/Publisher/lifecycle VICE gates, review the new wrapper/stack
cost, then resolve integration and the separate Settings-screen timeout. See the
canonical runtime contract and testing commands for exact scope. No merge, deployment or hardware testing is part of this handover.

## MVP scope and non-negotiable acceptance

One supported C64U baseline, one Standalone PHP site/destination, existing page/post source format and keyboard-first operation. Manual one-time hardware prerequisites and existing Basic Settings are acceptable; a new setup wizard is not required.

- Installation starts one clearly identified package through SEE64YOU.PRG.
- Website/key import/authentication test/save works; settings reload after cold restart; networking is optional for offline editing.
- New/Open/Save/Save As round-trip canonical content exactly. Core navigation, punctuation, deletion, clipboard and undo/redo work with USB and native C64 keyboards. Preserve existing editing features.
- Publish, list/open and update preserve source/metadata. The first demonstration may use a page; release acceptance covers existing pages/posts. Validate any exposed draft/status/delete actions without expanding their UI.
- Failed/cancelled network or storage operations do not silently lose current source. Specifically test remote-open failure after staging begins; the original staging path reused document/history REU regions.
- Publisher/Settings return preserves editor state; dirty exit supports Save/Discard/Cancel; temporary machine state restores/verifies before CLEAN and responsive BASIC return.
- Existing autosave/recovery must not corrupt saved source or interrupt continuous typing.
- Preserve authentication, explicit plaintext-HTTP consent, transactional writes, bounds/integrity/provenance checks and current persisted formats. Never silently downgrade HTTPS.

## M1 — Complete one MVP candidate (first implementation priority)

- [ ] Trace actual runtime/build paths and list the smallest remaining Publisher/Settings integration gaps. Reuse existing implementations; do not duplicate serializers, document models, stores or auth.
- [ ] Fix destructive remote-open staging before hardware: The baseline `persistent_document_stage.c` wrote response headers into live undo payload storage and body bytes into the live document. Preserve both until a complete validated replacement is ready; cover incomplete receive, cancellation, malformed response and DMA failure. Host fix and expanded `persistent_open_preservation_test.c` now pass locally and are wired into both validators; keep this item open until cc65/VICE validation and fit checks pass.
- [ ] Finish essential website/key/test/save/cancel Settings and real Publisher document actions. Defer profiles/advanced expansion. Keep resident orchestration small; return to it before switching overlays and respect pointer lifetimes.
- [ ] Move production-needed services out of `client/diagnostics/persistent_*` only as they enter the product; have tests and product link the same implementation. Keep fixtures/mock responses separate.
- [ ] Build a complete candidate with actual UI/services/assets, correct preload and byte-matched manifests. Retain fallback until M2; maintain one active feature roadmap.
- [ ] Measure the full package: at least 1 KiB resident headroom; existing stack/overlay guards; no single-digit permanent overlay squeeze; allocator at most 16 entries. No guard relaxation or capacity increase merely to fit deferred scope.
- [ ] Exercise the real packaged MVP path together without application-service/UI placeholders. Identify remaining hardware/network mocks explicitly; keep focused regressions and the 30-cycle lifecycle gate.
- [ ] Make release validation distinguish passed/failed/blocked/unverified. Missing mandatory tools/checks must not yield an unqualified release pass. Keep optional diagnostics separate without dropping required coverage.

**Done:** an identified candidate package (commit + checksum), complete software path and measured fit, ready for a bounded hardware demonstration.

## M2 — Hardware-prove the MVP

- [ ] Run affected tests and required full validation before hardware; record scope and exact commit. Use existing hardware procedures/backups/restoration, not a new harness project.
- [ ] On the exact candidate: launch → edit → local save/reopen → publish → browser display → remote reopen → edit/update → return → clean exit. Use disposable test content and compare canonical source/metadata.
- [ ] Confirm at least three complete manual cycles, settings cold persistence, offline editing, failed/cancelled open/save/publish preservation, and dirty exit/recovery. Preserve the existing 30-cycle emulator gate; cycle counts are not reliability statistics.
- [ ] Finish required capability guidance, USB/native input and ordinary file/menu acceptance; confirm no fresh module SD load on normal cached mode switches. Verify exact machine restoration and responsive BASIC.
- [ ] Have Dan perform concise routine visual/input acceptance: C64 40×25 editing/Publisher/Settings/error states; browser desktop and 390px display, keyboard navigation, no-JS readability, console/network errors.

**Done:** Dan can repeat the complete workflow using the package and short instructions. Record hardware/firmware, package hash, observed outcomes and limits in canonical evidence. Component tests do not substitute for this gate.

## M3 — Remove superseded machinery after M2

- [ ] Remove obsolete inter-mode adoption/return/application-state-transfer callers, then unused dependencies. Audit `managed_editor_publisher_*`, Publisher handoff/adoption and related services; do not delete entire families blindly.
- [ ] Retain required Launcher → Editor/bootstrap/BASIC-return parts of `module_*`, `transfer_capsule*`, `basic_return*` and lifecycle/session helpers.
- [ ] Retire software-IEC chain/companion resident-loader paths and conventional buffer/history forks only when surviving builds/tests no longer need them. Move useful regression tests to the canonical path first.
- [ ] Remove resolved tracing, unused targets/probes and obsolete workflow entries after identifying any unique regression value. Consolidate duplicated build definitions only where drift is demonstrated; no new build framework.
- [ ] Audit each deletion with caller search, build/workflow/conditional inclusion and generated veneers/manifests/link maps. Run affected gates and inspect the diff.
- [ ] Consolidate documentation in place: README = product/install; ARCHITECTURE = overview; C64U-RUNTIME = detailed contract/evidence; LOCAL_TESTING = current procedures; SETUP-UX = implemented flow and labelled future intent. Remove stale one-off plans after preserving unique active tasks here; retain licences/reference evidence.
- [ ] Run full required validation and a final hardware smoke on the cleaned package.

**Done:** one supported runtime/package path, purposeful retained diagnostics, no stale references and no lost regression coverage. Small cc65 source files are not clutter when required for linking/overlay placement.

## M4 — Deliver the accepted MVP

- [ ] Confirm clean-checkout reproducibility and package manifest/checksum identity with tested bytes; expose one clear primary C64U artifact.
- [ ] Supply concise installation/use/recovery instructions, known limits and evidence table.
- [ ] Follow explicit merge/deployment authorization and existing validation/rollback rules; keep repository, CI, hardware and deployed-site state distinct.

**Done:** reproducible usable MVP delivered; then reassess deferred work by user value.

## Deferred until MVP acceptance

These remain product intentions, not permission to remove existing functionality.

- Full first-run wizard, broader Basic/Advanced separation, profiles and integrated diagnostics/export/logging.
- New hardware automation/framework integration (JC-000/c64-test-harness) and sim6502/u64sim lane; use existing procedures now.
- Large-document paged/piece model, allocator-backed variable history/clipboard and pressure reclamation; large-document/deep-history stress beyond the demonstrated MVP limits.
- Further CPU-heavy profiling/acceleration and editor conveniences; do not regress current responsiveness.
- Editor appearance preference: first-run normal C64 colours; discoverable Dark Mode = black background/white text; persisted boolean using existing settings, collision-free shortcut (Ctrl+D only if still free), readable selection/cursor. No separate theme subsystem.
- Joystick/mouse enhancements and extended input tuning; preserve existing behaviour.
- Journal relocation and additional storage migrations; automatic update staging/verification.
- Alternate UCI windows, optional HTTP $06/TLS capability research, trusted-LAN relay and WordPress acceptance. Preserve source/security contracts; no silent transport downgrade.
- Website theme manual acceptance (C64 glyphs, no-JS lifecycle, 390px and legacy-browser/CSS/font fallback per LOCAL_TESTING); no additional theme work before MVP.
- Optional spell-check and future firmware facilities only when measured user value justifies them.

## Stop rules and reporting

No further broad architecture replacement by default. If required integration cannot meet guards, remove deferred scope first; if a broad redesign is still necessary, stop and reassess simpler delivery or shelving the client. Do not weaken safety to claim progress.

Two bounded implementation sessions without reducing the same blocker trigger a diagnosis review before more coding; this is not automatic abandonment.

Every session reports: newly working user action or necessary blocker resolved; exact branch/commit/package; passed/failed/blocked/unverified checks; one next blocker and smallest execution checklist. Do not equate commit counts, lines removed or component tests with product completion.
