# See64You TODO

**Only live work queue.** Reprioritised 2026-10-09 following the user-directed decision to use three independently launched, file-based C64U programs. The previous single-owner persistent-runtime MVP is superseded. Earlier code and measurements are evidence only, not the new deliverable.

## Current scope

User scope expanded 2026-10-11: include a separately launched, read-only C64U
browser for published sites. USB keyboard acceptance is deferred indefinitely
at the user's request. Preserve the established three-program file workflow.

## Next priority — first-time user acceptance (2026-10-12)

- [ ] On a backed-up/test C64U and disposable PHP website, follow the actual
  six-step `site/start.php` guide as a new user with physical native-keyboard
  input: enable prerequisites, set up website and HTTPS pairing, enter/import
  the key in Settings, consent/save and verify after restart, create/save
  `/sd/SEE64YOU/FIRST.S64`, publish, and confirm the public page/post.
  Optionally open it with `S64WEB.PRG`. Record screen wording, errors,
  discoverability, file locations and BASIC return. This complete guided
  first-time experience is not yet hardware-accepted; existing component and
  automated hardware checks do not substitute for it.

## Website themes (2026-10-12)

- [x] Retire the broken C64 built-in website preset from setup, theme
  selection and CSS; keep Plain, Simple, the optional C64 font, all
  customisation controls and user-saved themes. Old active C64 selections
  display as Simple without modifying saved configuration. Updated PHP
  regression expectations; full PHP suite and visual checks remain unverified.
- [x] Website theme colour fields now use the 16 named C64 palette
  values. Global settings reject newly submitted non-palette colours;
  existing saved colour values are left on disk and mapped for rendering.
  Custom themes, layouts and font choices remain available.
- [x] Implement opt-in public browser v2 colour triplet (background/text/link);
  legacy browser v1 is preserved for old clients/sites. C64U browser restores
  its previous screen colours on a normal exit. Added PHP/host C regression
  cases. The independent smoke gate passed at 7d72e7480d22:
  PHP site/theme tests, PHP browser tests, live local HTTP tests, native
  browser parser/HTTP tests, and cc65 independent S64WEB.PRG build. Browser
  headroom was 17,235 bytes. No release or deployment.
- [x] Add a GitHub-hosted smoke job to run focused website/browser checks
  independently of the self-hosted integration runner. It builds the full
  eleven-file independent package and uploads a provisional ZIP. At commit
  `17e23529c4076d06069436f75bc35ef7b00db0d4`, GitHub Actions run
  `38275335109` passed PHP unit/HTTP, native C parser/HTTP tests, full cc65
  build and both manifests. Launcher has 1,032 bytes free (8-byte margin
  above the 1 KiB guard), Publisher 1,046, Settings 4,009 and Browser
  17,235. Artifact ID `11759185570` is diagnostic-only, not hardware-accepted.
  The superseded persistent-runtime diagnostics are manual dispatch only.
- [ ] Run the full independent package through VICE and a real C64U; navigate
  between two sites using different C64 palettes and verify colours, URL
  handling, restoration and clean BASIC return. Record machine/package
  verification and exact per-file hashes before any release.
- [x] Website remains the sole interface for editing site colours.
  The user explicitly deferred C64U Editor-side colour editing; do not add
  website settings/network writes to Editor or Publisher unless requested again.

## Browser and cleanup (2026-10-11)

- [x] Integrate the public browser backend from `feature/c64u-public-browser`
  (original commit `558a5e3`). It contains only website wire rendering, not a C64
  executable. Reproduce/fix unreadable whitespace and rejected/mismatched Accept
  negotiation; focused PHP, existing site/theme and actual HTTP tests pass.
- [x] Implement/build independent `S64WEB.PRG`: bounded ASCII URL/wire parser,
  public HTTP target `$03`, two centrally allocated 64 KiB PREVIEW regions,
  selected links/paging, visible loading, cancellation and explicit HTTPS refusal.
  Strict/sanitizer host parser/socket fault tests, complete package/manifests and
  browser VICE URL/refusal/BASIC numeric/string checks pass. Real C64U home/page/
  archive/post navigation, paging, delayed loading/cancel, missing-page/stale-link
  refusal, HTTPS refusal and reopening pass. IRQ/drives/turbo 0 parity and original
  SD restoration pass; receipt `client/build/hardware-browser-final-20261011/state.json`.
  Browser headroom 18,054 B; the ten established package files remain byte-identical.
  Latest eleven-file package `client/build/SEE64YOU-independent-browser-20261011.zip`,
  SHA-256 `e9260fae11baf3fc08fb3f9aade1b9da18349c3d7e3bc8ee1a4929f68fc22c06`.
  Interrupted network/power behavior and USB are not claimed by these clean tests.
- [x] Remove 20 obsolete generated CI/validation trees (about 49.5 MiB before
  retained receipts). Archive logs/JSON/document fixtures under local ignored
  `client/build/receipts-archive/`; current packages, ROMs, hardware backups and
  recovery evidence remain. All ten current C64 package files still match the
  accepted ZIP. Permanent SD-root cleanup still waits
  for complete folder containment and verified migration.

## MVP file workflow contract

Editor saves/reopens canonical `.s64` source on SD and exits to BASIC. Publisher launches afresh, selects a local `.s64`, publishes/updates, and downloads remote content to a new `.s64`; it then exits to BASIC. Settings launches independently, loads/validates/saves `/sd/SEE64YOU/SEE64YOU.CFG` and exits to BASIC. No document, history, clipboard or session transfer between programs. C64 Ultimate-only with 16 MiB REU, Ultimate-DOS $01 and network $03 for direct publishing. Preserve direct challenge/HMAC auth, explicit plaintext HTTP consent, filesystem safety and machine restoration.

## Current user-requested setup work (2026-10-11)

- [x] Change Editor text from green to white. Independent package/manifests/guards pass unchanged; VICE checks all 200 body colour cells are white, existing rendering stages pass, and packaged startup/guards/wizard/BASIC-return checks pass. This colour change has not been rerun on physical C64U.
- [x] Complete bounded integrated real-C64U SD-folder containment acceptance and clean up migrated root files.
  - [x] Implement folder defaults for new documents/downloads, `SEE64YOU.REC`, two journal slots and preflight temporary probes. Editor document/autosave target `$02` is isolated from launcher/journal target `$01`. Explicit `/sd/NAME.S64` retains the legacy Save origin; no automatic root fallback hides corruption or I/O errors.
  - [x] Implement Launcher legacy-root-journal refusal with `S64CFG.PRG` guidance and confirmed Settings `M` migration: restore/verify ACTIVE ownership, stage CLEAN, verify exact `S64OLD0.DAT`/`S64OLD1.DAT` backups and retire root slots only afterward. Conflicts, corrupt/unreadable stores and failed verification block migration.
  - [x] Focused host fault/collision/context/recovery regressions and all four packaged VICE startup/BASIC-return checks pass. Complete independent package and both manifests pass unchanged guards: Publisher 1,046 B, Launcher 1,032 B and Settings 4,009 B free. Rejected large journal spikes were reverted; their logs are historical evidence, not live work.
  - [x] Correct the read-only real-C64U context diagnostic: result `a50000` proves target `$02` folder lookups survive primary-directory changes and both absolute/relative opens close. Prior isolated journal diagnostics pass six staging/recovery plus seven retirement cases, including changed-turbo restore before CLEAN; originals restored. These are component gates, not integrated acceptance.
  - [x] Reproduce/fix failed autosave-recovery deletion: preserve the original `SEE64YOU.REC` after failed recovery loading and inhibit autosave until explicit resolution. Focused regression verifies preservation; real-C64U reproduction remains pending.
  - [x] Backed-up integrated C64U Launcher refusal/Settings ACTIVE migration, folder recovery/edit/save, fresh Publisher folder/explicit-root load, authenticated download/collision refusal and fresh reload/publish pass. Exact metadata/body and same-name root source are preserved; CLEAN/BASIC numeric/string/turbo/IRQ/drives and original-file restoration pass. Receipts: `hardware-folder-mvp-20261011` and `hardware-folder-network-20261011` under `client/build/`. REST plain-key input is distinct from physical keyboard testing. Failed REC recovery is host fault coverage; controlled physical power interruption remains unverified and USB is user-deferred.
  - [x] On this C64U, verify absent folder REC and copy the existing canonical root `SEE64YOU.REC` byte-exact into the folder, retaining local and SD backups. Its source remains available to the next Editor recovery prompt; do not claim its content was edited during migration.
  - [x] Install the verified eleven-file package, migrate actual legacy journals through Settings M, verify both `S64OLD*.DAT` originals, and remove 20 identified old SEE64YOU-owned SD-root files only after exact backups. Root source `a test post`, REC and key are retained in the app folder; canonical folder config is unchanged. All package hashes, BASIC numeric command, IRQ/drives/live turbo 0 and remaining unrelated root files pass. Receipt/backups: `client/build/folder-install-20261011/`; SD backup: `/sd/SEE64YOU/legacy-backup-20261011/`. The corrected driver's input-chunking retry completed; no emergency reset or unrelated SD deletion.
  - [x] Latest testable package: `client/build/SEE64YOU-independent-folder-browser-20261011.zip`, SHA-256 `c490286bc711ac01959f3cbdafb9a8f862d182f478c665440ff20b9892585d3e`. Browser and folder gates are recorded separately from physical power-interruption/USB scope.
  - [x] Clean Git archive `3dd56e52982ac619ea259e9c7e45f76bf0d37c83` reproduces all eleven ZIP/installed files exactly. Final installed Editor recovers the migrated original REC and returns to responsive BASIC; exact REC bytes are restored for the next user session. Root files remain absent, config unchanged, IRQ/drives/live turbo 0 pass. Receipts: `folder-package-receipt.json` and `installed-editor-smoke-20261011/state.json` under `client/build/`.
- [x] Implement linked website/C64U guidance through first page/post without changing program/file boundaries: six-step no-JS browser walkthrough, three-step independent Settings wizard, private key import/manual entry, explicit HTTP consent, verified saving and exact Editor/Publisher instructions.
- [x] Review and validate the previously retained local allocator extraction/test-tool changes before committing them; keep obsolete runtime builds out of MVP results.
- [x] Independent package/manifests/guards, existing host/PHP/HTTP tests and packaged VICE wizard required-field/key/consent/failed-save/back/basic/exit checks pass. Publisher 1,060 B free; Settings 15,740 B; launcher 1,141 B; Editor resident margin unchanged at 473 B.
- [x] Bounded real-C64U wizard acceptance using remote plain keys: first entry/required-site refusal/cancel without config write; key import; explicit HTTP consent; verified save; relaunch/rerun/cancel byte parity; fresh Publisher authentication; rebuilt Editor launcher startup/exit and BASIC strings. Original SD files restored exactly, drives/IRQ/clock/config baseline checked, live turbo 15 restored. Manual key entry is VICE-only.
- [x] Clean archive 8da6fda reproduces all ten hardware-tested files. Latest package: `client/build/SEE64YOU-independent-setup-wizard-20261011.zip`, SHA-256 `4bc2be58cbf18e10fbf8ed67eb51550fd405fee69115dc92c769035e9babe14e`; receipt records source, per-file parity and restored hardware scope.
- [ ] Physical wizard key input and a new guided cold power cycle remain unverified. Browser navigation is tested; user first-publication usability is not claimed from step navigation alone. USB remains user-deferred; broader prior hardware gaps remain in M2.

## M1 â€” Three-program software candidate (implemented, partial hardware validation)

- [x] Inspect the baseline and reject bare Editor-only linking after its 1,161-byte conventional-RAM overflow; preserve the tested Editor launcher and private code overlays.
- [x] Publisher independent startup, manual local `.s64` filename load (now folder-relative, with explicit root paths), direct validated on-disk settings load, existing authenticated network services, and BASIC exit. No Editor REU handoff.
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
- [x] Corrected candidate physical C64U-keyboard Publisher connect/page browse/open/download to new S6T02.S64/BASIC passed. SD readback proves exact canonical body/title/slug/status/order and unchanged original local source. Physical Editor reopen/edit/save and fresh Publisher update also pass: exact body `Cycle 2. Cycle 1. MVP hardware round trip.`, unchanged metadata, clean BASIC, CLEAN 92, IRQ/clock and live turbo 0. One guided manual full round trip is complete; second physical download to S6T03.S64 passed exact readback; Editor edit/update remains pending. Three-cycle acceptance remains incomplete.

- [x] 2026-10-10 file wait feedback and BASIC string-memory fix: show Loading/Saving before file operations without corrupting Publisher scratch. Reproduce invalid STREND above MEMSIZ after large-PRG exit; reclaim only that invalid workspace. All three packaged programs pass executed BASIC string PRINT in VICE and real C64U; valid equality workspace stays unchanged. Package/manifests/guards pass: Publisher 1,098 B spare, Settings 17,488 B, launcher 1,141 B, Editor resident margin 473 B. Existing turbo/REU integrity path was measured; a CRC trial showed no improvement and was discarded. Physical file-message observation remains pending.

**Current constraint:** Editor's existing launcher still verifies/caches Publisher and Settings PRG entries as part of the legacy three-module manifest even though those binaries are launched separately. No live application state transfers. Do not rewrite launcher/cache machinery merely to save that preload before acceptance; remove it in M3 if unused. The package is an MVP **candidate**, not a release.

2026-10-10 bounded session closed: a773871 file-feedback package, SHA-256
1ac8f6bba353deafbcf4b8eb992b4b67e4dcd22ef9616c0307c115b4aea5d06d.
All ten files matched hardware; newest valid CLEAN 100 and live turbo 0 passed.
Original SD files restored with exact hashes; drives/IRQ/clock/BASIC and enabled
REU/turbo baseline passed. Candidate is no longer staged. Second download
S6T03.S64 is captured locally for the next guided edit/update; do not restart
the restored=true staging helper without preparing a new backed-up session.
## M2 â€” Bounded hardware acceptance

**Final checkpoint (2026-10-11):** three functional cycles and bounded faults
passed. Product package 7aada00 / SHA-256
308ffc6ffd412dae3341b671019e79ff8a338044d61f0a2e60e4a52ab2a4cc9c.
Clean archive 72f1fd5 reproduces all ten files exactly. Final CLEAN 94/live turbo
15 passed before original SD restoration; original hashes and drives/IRQ/clock/
BASIC/user configuration and live 15 passed afterward. Candidate is no longer
staged. USB, uncontrolled physical interruption and broader UI remain unverified.

- [x] Hardware-reproduced missing wait feedback in Publisher remote listing/open: direct Loading... precedes first/next requests and confirmed open without altering aliased scratch. Build/package/manifests/guards and packaged VICE checks pass; Publisher 1,060 B spare. Real C64U captures show each message before successful responses. S6T04.S64 download and third reopen/edit/update are exact. Automated checks are labelled separately from physical-keyboard scope.

2026-10-11 continuation on unchanged a773871 package: second guided full round
trip completed using the physical C64U keyboard. S6T03.S64 reopened promptly;
the first interrupted attempt left the file unchanged and is not counted.
The focused repeat saved exact body `Cycle 3. Cycle 2. Cycle 1. MVP hardware
round trip.` with unchanged metadata. Fresh Publisher load/auth/update and
Loading/Connecting/Publishing messages were user-confirmed; exact backend
source and HTTP 200 rendered HTML match. Clean BASIC/string input, IRQ/clock
and valid CLEAN 90 passed at the intermediate checkpoint. The third cycle and
Editor Loading/Saving were subsequently verified. USB remains deferred.
Fresh SD backups: client/build/hardware-mvp-resume-20261011/; original files
are restored. A controlled load-only/BASIC SYS check captured and restored live
15 with CLEAN 94, separating application restoration from the runner reset.

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

- [x] On backed-up SD and 16 MiB REU C64U: Editor create/save â†’ BASIC; Publisher launch/open/publish â†’ BASIC; Publisher remote open/save NEW `.s64` â†’ BASIC; Editor reopen/edit/save â†’ BASIC; fresh Publisher update.
- [x] Verify browser rendering, exact source/metadata, Settings physical cold persistence/cancel, missing-source guards, existing-download refusal, responsive BASIC, IRQ/drives and persistent prerequisite baseline.
- [x] Further real-hardware failure checks: truncated source clears the document and blocks publish/download; trailing-byte config is not adopted and cancel preserves it; controlled disposable-backend HTTP 503 rejects auth and blocks publish. Exact local/remote bytes and final original SD/machine restoration pass. No code changes needed.
- [x] Three functional round trips complete: two guided physical C64U-keyboard cycles, third automated after physical Ctrl+O as requested. Exact source/metadata/browser output, Editor Loading/Saving and Publisher Loading/Connecting/Publishing captured. Reject a short HTTP body and block saving/publishing partial content; source/config/remote page unchanged. Controlled load-only then BASIC SYS proves application-entry/exit live turbo 15, valid CLEAN 94, BASIC strings and IRQ/clock; prior baseline 0 also passed.
- [x] Bounded real-SD diagnostic rejects selected rename/read transport calls: exact latest document temp is retained and reopens; failed config verification preserves prior profile; failed final config rename leaves exact config temp recoverable. Copy both before cleanup. This is not physical SD removal; delete-then-rename can leave the destination absent and does not preserve both versions atomically.
- [ ] USB keyboard is explicitly user-deferred. Broader physical storage/power interruption and 40Ã—25 visual acceptance beyond the captured/tested paths remain unverified.
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

## M3 â€” Cleanup and delivery AFTER M2

- [ ] Identify and remove only unused inter-program adoption/transition, redundant overlays and dead tests/targets. Preserve essential launcher, journal/recovery and independent product code. No indiscriminate deletion.
- [ ] Consolidate README/ARCHITECTURE/C64U-RUNTIME/LOCAL_TESTING/SETUP-UX; move stale details to Git history, keep one live queue.
- [ ] For public GitHub availability, create versioned downloadable client ZIP (all eleven independent-package files) and separate standalone PHP website ZIP as GitHub Release assets after first-time acceptance, with exact build commit, SHA-256 and installation checklist. The build and CI candidate artifact already exist; avoid duplicate packaging machinery.
- [x] Clean archive 72f1fd5 reproduces all ten tested package files; manifests/guards, exact source/metadata round-trips and bounded hardware smoke pass. Latest package/receipt/checklist and README installation/recovery guidance are available. No merge/deploy; deferred hardware scope remains explicit.

## Validation snapshot

- Branch `feature/c64u-publisher-chain`; PR #61 merged into `integration-validation-2026-09-24` at user request. Website deployment is not requested; public GitHub/CC0 publication preparation remains outstanding.
- Exact earlier software CI: [37879998473](https://github.com/dannykeat-projects/SEE64YOU/actions/runs/37879998473) â€” independent Publisher+Settings linked and passed guard; source/config/direct network host tests passed; combined independent package built, Editor overlay manifest and module manifest validated. Full workflow red **only on the old managed Publisher overlay** later in the job.
- Memory: Publisher BSS `$C679` / 1,414 B spare to `$CC00`; Settings BSS `$871F` / 17,632 B spare; Editor resident BSS `$89B8` before `$8C00` overlay slot; launcher BSS `$C752` / 1,197 B spare to `$CC00`. Editor slot margin is 584 B under its existing slot-overlap rule, not a claimed 1 KiB Editor margin.
- Current bounded acceptance: three functional hardware round trips, working messages, short-response rejection, real-SD injected transport faults and turbo 0/15 pass. VICE checks absent-UCI startup/refusal and executed numeric/string BASIC only. USB and uncontrolled physical interruption remain unverified; preserve fallback and do not merge/deploy.
## Deferred until MVP acceptance

These remain product intentions, not permission to remove existing functionality.

- Separately developed public-site browser (`feature/c64u-public-browser`): explicitly outside the three-program MVP and its acceptance/package. Test it later, as directed by the user on 2026-10-11.

- Broader Basic/Advanced separation, expanded profiles and integrated diagnostics/export/logging; the bounded first-publication wizard is now implemented above.
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
