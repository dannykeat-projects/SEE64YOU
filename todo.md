# See64You TODO

**Only live work queue.** Reprioritised 2026-10-09 following the user-directed decision to use three independently launched, file-based C64U programs. The previous single-owner persistent-runtime MVP is superseded. Earlier code and measurements are evidence only, not the new deliverable.

## MVP contract

Editor saves/reopens canonical `.s64` source on SD and exits to BASIC. Publisher launches afresh, selects a local `.s64`, publishes/updates, and downloads remote content to a new `.s64`; it then exits to BASIC. Settings launches independently, loads/validates/saves `/sd/SEE64YOU/SEE64YOU.CFG` and exits to BASIC. No document, history, clipboard or session transfer between programs. C64 Ultimate-only with 16 MiB REU, Ultimate-DOS $01 and network $03 for direct publishing. Preserve direct challenge/HMAC auth, explicit plaintext HTTP consent, filesystem safety and machine restoration.

## M1 — Three-program software candidate (implemented, partial hardware validation)

- [x] Inspect the baseline and reject bare Editor-only linking after its 1,161-byte conventional-RAM overflow; preserve the tested Editor launcher and private code overlays.
- [x] Publisher independent startup, manual local `.s64` filename load under `/sd/`, direct validated on-disk settings load, existing authenticated network services, and BASIC exit. No Editor REU handoff.
- [x] Remote page/post open and save to a **new** user-named `.s64`; refuse an existing destination. Failed local or remote reads disable publish/download and clear partially decoded document content.
- [x] Settings independently probes capabilities, reads/validates/saves canonical `.CFG`, cancels without storing, and exits to BASIC; file readback still gates a successful save.
- [x] Complete cc65 Publisher and Settings link/fit: 2026-10-09 CI 37879998473 measured Publisher 1,414 B free (>=1,024 guard), Settings 17,632 B free. Earlier standalone Editor linking does not fit; the private-overlay Editor instead passes its existing slot guard.
- [x] Build and manifest-validate a candidate package (`make -C client independent-package`) with one Editor launcher, its private module/code assets, and independently launchable Publisher/Settings. No shared document/history/session across launches.
- [x] Host service tests for local documents, disk settings and direct auth/network passed in CI 37879998473; those are component checks, not an integrated runtime test.
- [x] Rebuild exact checkpoint `0b2ad99ebd9419c13d3b1ea62542c8a701834863` locally with cc65; both manifests and all binary guards pass. Publisher has 1,380 B free, Settings 17,632 B, launcher 1,197 B; Editor resident slot margin 584 B. File/config/direct-network host tests, backend tests and failed-open preservation pass. See LOCAL_TESTING for emulator scope and package receipt.
- [x] Actual-package absent-UCI startup/exit smoke passes in VICE 3.9 with standard ROMs from the user's local VICE installation. Publisher's missing-source/DOS/network guards and Settings' failed-save/cancel path pass, followed by an executed BASIC POKE for each program. Pinned Open ROMs remain unsuitable for this conio smoke. Existing return/IRQ/30-swap/30-cycle fixture passes are separate evidence; full UCI file/network integration still needs hardware.
- [x] Confirm the physical C64U file workflow through Editor reopening/edit/save and fresh Publisher update against the matching local test site, including exact metadata and visible browser output. Settings cold persistence and BASIC return pass. Broader manual/UI and interrupted-I/O acceptance remain in M2.
- [x] Reproduce and fix Editor exit corruption: capsule/session teardown reused the validated BASIC-context scratch. Preserve the 321-byte context beyond that scratch; regression, complete package/manifest/fit and packaged VICE smoke pass. Real C64U Editor edit/local save/dirty Save exit now writes CLEAN and returns to responsive BASIC without reset.
- [x] 2026-10-10 local-save fault regressions: retain new-only policy at the second existence check and keep completed `<filename>.tmp` when final rename fails. Both failures reproduced before the fixes; exact temp source reopens afterward. Clean independent package/manifests/guards and packaged VICE startup/BASIC exit pass; Publisher headroom 1,385 B, FILE asset 15,350/15,360 B.
- [x] 2026-10-10 Settings integrity follow-up: reject trailing config bytes and verify exact temporary config before replacing the previous file. Reproduced both failures first; host temp-corruption/read-failure preservation, legacy/profile persistence and final readback tests pass. Complete package/manifests/guards and packaged VICE startup/exit pass; Publisher 1,297 B, Settings 17,544 B spare. Hardware storage acceptance remains pending.
- [x] Resume bounded hardware acceptance against a disposable matching localhost backend (2026-10-10). Authenticated compact listing, publishing, second-item browsing, remote open and new-file download work on firmware 1.1.0. The deployed backend mismatch remains; no deployment or client protocol workaround.
- [x] Hardware-reproduced Editor uppercase filename loss: accept native PETSCII $C1-$DA in text prompts; preserve termination on insert/delete/cancel and empty ENTER. Focused prompt regressions and actual uppercase `S6T00.S64` save pass. FILE overlay 15,341/15,360 B.
- [x] Hardware-reproduced remote Next failure: rebuild the list URL after each request reuses HTTP scratch. First/second compact list items and remote open/download pass. Publisher 1,285 B spare; complete package/manifests/guards, direct-network host tests and packaged VICE startup/exit pass.
- [x] Physical-keyboard Editor reopen/edit/save/BASIC passed in manual cycle 1, preserving exact remote metadata. Reproduced garbled BASIC display from return-context/stub staging in screen RAM. Move only the transient exit stub to the cassette buffer and clear the display through KERNAL before restoring flags/returning. Both ROM-set VICE return regressions, packaged startup/BASIC smoke, focused host return tests and unchanged package/fit guards pass.
- [x] Manual C64U-keyboard Publisher load/auth/publish/BASIC steps passed after the physical Editor edit; exact local metadata/body and remote page matched `Cycle 1. MVP hardware round trip.` This is an edit/update segment, not three complete download/reopen cycles. User reported uncertainty during blocking requests; add immediate Connecting/Publishing status before them. Package/manifests/guards and packaged VICE smoke pass; Publisher 1,234 B spare.
- [x] On refreshed source `95622c1`, physical C64U-keyboard RUN/STOP returns Editor to a clean screen; PRINT 6*7 produces 42. Valid newest CLEAN 90 and CPU-executed live turbo baseline parity (0 before/after) pass.
- [x] Reproduce/fix busy-message URL corruption on hardware: WORK_STATUS aliases WORK_URL. Draw Connecting/Publishing directly without modifying scratch. Real hardware captures both messages and completes auth/publish with exact local/remote bytes; BASIC/IRQ/clock pass. Package/manifests/guards and packaged VICE smoke pass, Publisher 1,213 B spare. Supersede the failed 95622c1 ZIP.
- [x] Corrected candidate physical C64U-keyboard Publisher connect/page browse/open/download to new S6T02.S64/BASIC passed. SD readback proves exact canonical body/title/slug/status/order and unchanged original local source. Editor is awaiting physical reopening/edit/save of that download; repeatability acceptance remains incomplete.

**Current constraint:** Editor's existing launcher still verifies/caches Publisher and Settings PRG entries as part of the legacy three-module manifest even though those binaries are launched separately. No live application state transfers. Do not rewrite launcher/cache machinery merely to save that preload before acceptance; remove it in M3 if unused. The package is an MVP **candidate**, not a release.
## M2 — Bounded hardware acceptance

2026-10-10 continuation: independent Settings save/relaunch/cancel, Editor
uppercase canonical file save, fresh Publisher local load/auth/publish,
second-item remote browsing/open and new-file download passed with REST plain
key injection against an isolated matching localhost site. Exact source/metadata,
existing-download refusal, CLEAN 92, IRQ/clock/BASIC and original SD restore
passed. The user's enabled REU/turbo-register baseline was retained. Editor
download reopening/edit/save and fresh Publisher update subsequently passed with
the user's physical Ctrl+O. The browser displayed the exact updated body, with
remote metadata preserved. After a user-confirmed physical power cycle, fresh
Settings loaded the exact saved config, passed readiness and cancelled unchanged.
A missing local source after a successful load cleared the document and blocked
publish/download; local and remote bytes stayed unchanged. Final CLEAN 88 (older
ACTIVE 87), BASIC/IRQ/clock/drives, persistent prerequisite baseline and exact
original SD restoration passed. REST plain-key injection is not full physical
keyboard or three-manual-cycle acceptance.

- [x] On backed-up SD and 16 MiB REU C64U: Editor create/save → BASIC; Publisher launch/open/publish → BASIC; Publisher remote open/save NEW `.s64` → BASIC; Editor reopen/edit/save → BASIC; fresh Publisher update.
- [x] Verify browser rendering, exact source/metadata, Settings physical cold persistence/cancel, missing-source guards, existing-download refusal, responsive BASIC, IRQ/drives and persistent prerequisite baseline.
- [x] Further real-hardware failure checks: truncated source clears the document and blocks publish/download; trailing-byte config is not adopted and cancel preserves it; controlled disposable-backend HTTP 503 rejects auth and blocks publish. Exact local/remote bytes and final original SD/machine restoration pass. No code changes needed.
- [ ] Complete three manual cycles, native/USB keyboard and 40×25 visual acceptance, interrupted network/storage failure preservation and live temporary turbo restoration. Persistent configuration parity alone does not prove live turbo-register restoration.
- [ ] On disposable files, verify failed final document rename leaves the exact latest canonical `<filename>.tmp` recoverable; copy it before retrying. Existing delete-then-rename replacement does not preserve both versions atomically. Include configuration rename/readback failures in storage acceptance.
- [x] Record firmware, tested package hash, precise outcomes and known limitations in canonical docs; provide the latest package and concise manual checklist.

2026-10-09 bounded run: C64 Ultimate firmware 1.1.0 / FPGA 122 / core 1.49,
16 MiB REU. Publisher local load, authentication and cancelled local-open
preservation passed. Remote listing failed on the live backend mismatch;
publish/update/download, three cycles, Settings cold persistence, reopen,
physical keyboard and broader failure cases remain unverified. Hardware stopped
before any remote write. Original SD files/journals/recovery were restored with
exact hashes; drives/IRQ/clock and persistent settings checked. Preserve the
user's newly enabled turbo baseline. BASIC screen staging debris still requires
a clear-screen before typing. See LOCAL_TESTING for the tested package identity.

## M3 — Cleanup and delivery AFTER M2

- [ ] Identify and remove only unused inter-program adoption/transition, redundant overlays and dead tests/targets. Preserve essential launcher, journal/recovery and independent product code. No indiscriminate deletion.
- [ ] Consolidate README/ARCHITECTURE/C64U-RUNTIME/LOCAL_TESTING/SETUP-UX; move stale details to Git history, keep one live queue.
- [ ] Validate clean-checkout package reproducibility, source/metadata round-trips and final hardware smoke. Publish installation/recovery instructions and checksums; do not merge/deploy until authorised and mandatory gates pass.

## Validation snapshot

- Branch `feature/c64u-publisher-chain`, draft PR #61, never merged/deployed.
- Exact earlier software CI: [37879998473](https://github.com/dannykeat-projects/SEE64YOU/actions/runs/37879998473) — independent Publisher+Settings linked and passed guard; source/config/direct network host tests passed; combined independent package built, Editor overlay manifest and module manifest validated. Full workflow red **only on the old managed Publisher overlay** later in the job.
- Memory: Publisher BSS `$C679` / 1,414 B spare to `$CC00`; Settings BSS `$871F` / 17,632 B spare; Editor resident BSS `$89B8` before `$8C00` overlay slot; launcher BSS `$C752` / 1,197 B spare to `$CC00`. Editor slot margin is 584 B under its existing slot-overlap rule, not a claimed 1 KiB Editor margin.
- No complete hardware or emulator integration test has been recorded for the independent package. Run exact-head software checks again after changes. Preserve the fallback until hardware acceptance.
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
