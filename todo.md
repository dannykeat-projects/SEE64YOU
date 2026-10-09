# See64You TODO

**Only live work queue.** Reprioritised 2026-10-09 following the user-directed decision to use three independently launched, file-based C64U programs. The previous single-owner persistent-runtime MVP is superseded. Earlier code and measurements are evidence only, not the new deliverable.

## MVP contract

Editor saves/reopens canonical `.s64` source on SD and exits to BASIC. Publisher launches afresh, selects a local `.s64`, publishes/updates, and downloads remote content to a new `.s64`; it then exits to BASIC. Settings launches independently, loads/validates/saves `/sd/SEE64YOU/SEE64YOU.CFG` and exits to BASIC. No document, history, clipboard or session transfer between programs. C64 Ultimate-only with 16 MiB REU, Ultimate-DOS $01 and network $03 for direct publishing. Preserve direct challenge/HMAC auth, explicit plaintext HTTP consent, filesystem safety and machine restoration.

## M1 — Bounded implementation, Publisher fit is first acceptance gate

- [ ] Confirm current PR head and measured standalone Publisher baseline (reported 641 bytes spare); trace actual build/link and current stored-file paths.
- [ ] Replace Publisher Editor-handoff startup with local `.s64` selection/loading in the existing Editor SD context; reject invalid source before publishing.
- [ ] Load/validate publishing configuration from the canonical file on each fresh Publisher launch, without an REU settings bridge or inter-program chain.
- [ ] Keep the existing direct auth, remote list/open/save/update services. Download remote source to a **new** local `.s64`, refusing an existing filename; do not risk existing documents on cancellation/partial receive/storage failure.
- [ ] **Hard stop:** build the full real Publisher (UI, file I/O, config, auth, network and wire format) with at least 1024 bytes RAM headroom and existing stack/overlay bounds. No stubs, guard relaxation or large redesign. If it cannot fit after one focused reduction, report exact map and stop to reassess.
- [ ] Finish independent Settings candidate: direct disk load after capability probe, save/readback verification, cancel without disk mutation, exit to BASIC. The initial additive Settings target was committed at `43e3d3c` but has NOT been compiler/hardware-verified.
- [ ] Preserve Editor file round-trip/bootstrap/restoration. On the independent candidate, replace any misleading integrated Publisher transition with clear instructions to save and launch Publisher separately.
- [ ] Package three independently launchable PRGs plus only genuinely needed assets. Retain managed package as labelled fallback until new hardware acceptance.
- [ ] Add focused regression tests for local source, missing/invalid CFG, remote wire round-trip, refusal to overwrite, incomplete network and failed write/close/rename, exit and restoration. Run host, complete cc65 fit, VICE and full affected validators; record package hash and source commit.

**M1 done:** a complete software-integrated, measured candidate, not separately passing slices. No hardware requested until this gate passes.

## M2 — Bounded hardware acceptance

- [ ] On backed-up SD and 16 MiB REU C64U: Editor create/save `TESTPAGE.s64` → BASIC; Publisher launch/open/publish → BASIC; Publisher remote open/save NEW `FETCHED.s64` → BASIC; Editor reopen/edit → BASIC; Publisher update.
- [ ] Verify browser rendering, exact source/metadata, Settings cold persistence and cancel behaviour, network/storage failure preservation, responsive BASIC, IRQ/drives/temporary machine settings, and three complete manual cycles.
- [ ] Record firmware, tested package hash, precise outcomes and known limitations in canonical docs. Dan performs ordinary 40×25 visual and keyboard acceptance.

## M3 — Cleanup and delivery AFTER M2

- [ ] Identify and remove only unused inter-program adoption/transition, redundant overlays and dead tests/targets. Preserve essential launcher, journal/recovery and independent product code. No indiscriminate deletion.
- [ ] Consolidate README/ARCHITECTURE/C64U-RUNTIME/LOCAL_TESTING/SETUP-UX; move stale details to Git history, keep one live queue.
- [ ] Validate clean-checkout package reproducibility, source/metadata round-trips and final hardware smoke. Publish installation/recovery instructions and checksums; do not merge/deploy until authorised and mandatory gates pass.

## Validation snapshot

- Source inspection: confirmed original Publisher still requires Editor handoff; settings direct disk loader exists.
- Independent Settings candidate: source committed; build/headroom/VICE/hardware **unverified**.
- Complete independent Publisher: **not implemented**.
- Existing persistent-runtime diagnostic evidence remains historical, not proof of new MVP.

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
