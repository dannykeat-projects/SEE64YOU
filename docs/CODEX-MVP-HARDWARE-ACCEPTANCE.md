# Codex handover — SEE64YOU MVP hardware acceptance

**Read this before using the C64U.** This is an execution handover, not a new feature plan. Work on `maintenance/mvp-scope-reduction` in `dannykeat-projects/SEE64YOU` (previous checkpoint `b45aed46cf41624340325bfbb7e1603ceaf05d90`; **fetch and record the actual branch HEAD before starting**, since the branch may advance). Use the local `C:\site\SEE64YOU` checkout only after verifying its remotes, worktree, active branch and uncommitted changes. Never reset or overwrite another developer's work.

## Goal and scope

Establish whether a *new user* can configure the existing flat-file PHP website, write a canonical `.s64` document on the Commodore 64 Ultimate, publish and update it, and navigate two differently coloured SEE64YOU sites using the independent Browser. Work only with the existing **eleven-file** package containing four user programs: `SEE64YOU.PRG` (Editor), `S64PUB.PRG` (Publisher), `S64CFG.PRG` (Settings), and `S64WEB.PRG` (optional Browser). Every launch is independent and exits to working BASIC. No managed cross-program runtime or speculative features.

Read `AGENTS.md`, `todo.md`, `docs/LOCAL_TESTING.md`, `docs/C64U-RUNTIME.md`, `docs/SETUP-UX.md`, `docs/FIRST-PAGE.md` and the site-side `start.php` instructions. **Follow the user instructions for the end-to-end test**, not an internal shortcut that bypasses wizard prompts or publishes directly via an API.

## Safety and authorisation boundaries — mandatory

1. **Observe before writing:** check C64U reachability, firmware, UCI/DOS/REU capability and whether the hardware is already in use. Do not interrupt an active user session. If hardware is unavailable or cannot be safely isolated, complete host/VICE work and report hardware tests **BLOCKED** instead of guessing results.
2. **Back up first, with exact hashes:** inventory existing SD app folder, legacy root SEE64YOU config/journals/recovery, candidate document names, installed binaries, temporary files and unrelated SD files. Save a timestamped receipt/backups *outside* the C64U's changing test paths, and verify reads/hashes. Capture drive settings, IRQ, CPU-executed live turbo, configuration, BASIC responsiveness and restore prerequisites. Follow existing proven harness procedures; distinguish physical hardware values from REST RAM-shadow reads.
3. **No destructive testing on user files or a live site.** Use uniquely named temporary document files and a dedicated, unconfigured disposable PHP site with test credentials; for palette testing create **two independently configured disposable site instances** reachable from the C64U. Do not overwrite the user's live site or public posts. Never put keys/passwords/real tokens in logs, Git commits, screenshots or issue comments; redact them in receipts.
4. Do not delete or migrate legacy root files, overwrite existing SD documents, modify unrelated Ultimate settings, physically remove SD, or force a cold power-off merely for this test. If fresh-install testing would require these operations, use a fully isolated test medium/configuration or mark the step **BLOCKED**. Follow Settings migration only against verified expendable fixtures.
5. Preserve originals, recoverable `.tmp`, `SEE64YOU.REC`, config and journal state. Restore all temporary changes after each automated hardware session and verify byte-for-byte restoration of originals and machine settings. If a fault interrupts cleanup, stop and report exact recovery steps; never overwrite ambiguous source/recovery state.
6. No merge to `main`, deployment, GitHub Release or broad cleanup. Commit **only bounded, reviewed fixes** to the maintenance branch after rerunning their relevant tests, checking the remote head and preserving unrelated work. Do not silently weaken memory/integrity/restore guards. Stop and seek a go/no-go review if a fix requires another major loader/runtime redesign or two bounded attempts make no progress.

## A. Baseline software validation (no hardware)

From the current branch in a clean or safely isolated worktree, record exact SHA, compiler/tool versions and each command's exit code:

```sh
make -C client independent-package
sh tools/validate.sh
php site/tests/run.php
php site/tests/browser.php
php site/tests/themes.php
sh site/tests/http.sh
php tools/package-site.php
python3 tools/test_independent_programs_vice.py --rom-dir LOCAL_ROM_DIRECTORY
python3 tools/test_basic_return_vice.py --rom-dir LOCAL_ROM_DIRECTORY
```

Use real local ROM paths and LF scripts where required. The broad `validate.sh` may include older managed-runtime gates; investigate a failure and classify **MVP shipping path / historical-only** instead of concealing it. Verify the exact eleven installed files, both manifests, current conventional-RAM and overlay-fit limits, the packaged PHP output, and SHA-256 per file plus ZIPs. The GitHub-hosted maintenance-branch job creates **provisional** client/site ZIPs and SHA256SUMS; confirm it actually passed or record the failure. CI candidates are not hardware-approved releases.

**Gate A:** build + relevant host/PHP/VICE checks pass, with any historical-only failures documented. If this gate fails, do not stage unvalidated binaries on hardware.

## B. Fresh-user workflow using real C64U (main release gate)

Use real C64U firmware-1.1 baseline, enabled UCI, 16 MiB REU, Ultimate-DOS, network target `$03`, working SD and a disposable PHP 8.1+ site reachable from the C64U. Don't claim HTTP target `$06` or TLS support. The user-visible walkthrough is in `docs/FIRST-PAGE.md`.

1. On backed-up, isolated test storage, install the complete matching eleven-file package under `/sd/SEE64YOU/`. Record readback hashes; do not replace an existing installation until its backup/restore is demonstrably safe.
2. Create/configure a fresh disposable website through the normal site setup guide. Pair over **HTTPS** and use a dedicated test publishing key. Note that direct C64U HTTP+HMAC protects request integrity/authentication, **not the confidentiality of documents**; do not transmit sensitive test content.
3. Launch `S64CFG.PRG` as a new user. Navigate the wizard **as documented**, import or type the dedicated test key, explicitly accept the plain HTTP warning if it fits the disposable test, save, return to BASIC and relaunch to verify persisted settings. Distinguish a save-success screen from a real Publisher connection.
4. Launch `SEE64YOU.PRG`, create an easily identifiable public *test* document (use a unique new `.s64` filename, not an existing `FIRST.S64`), save, reopen to confirm bytes, then return to BASIC.
5. Fresh-launch `S64PUB.PRG`, load the exact local file, connect/authenticate, ensure **published**, publish it and verify ordinary HTML and body from another modern browser. Verify slug/type/title/body and that normal site navigation works.
6. Through Publisher's documented remote list/open path, download the canonical source to another **new** filename; ensure existing destination files are refused. Exit, reopen/edit/save in Editor, then reload/update in Publisher. Verify remote metadata and generated HTML without touching the original local file.
7. After each independent program exits, execute numeric **and string** BASIC commands, verify drives/IRQ/live turbo/clock, exact filesystem state and durable CLEAN journal/recovery behaviour. Restore the user's original assets/settings using verified backups.

**Codex may drive keys by an existing bounded REST/FTP harness** and inspect screen RAM/screenshots; that is **automated hardware acceptance**, not proof of physical keyboard feel or independent human usability. A genuine physical-keyboard cold-setup check remains **UNVERIFIED** unless a person actually performs it. Remote reset/relaunch does not count as a physical power-cycle.

**Gate B:** automated C64U workflow succeeds end to end on the *same exact binaries* as Gate A with no missing source, false success, wrong remote content or damaged settings. Mark any human-only observational gates separately.

## C. Independent Browser and dual-site colours

1. Configure **two** separate disposable PHP site instances with clearly distinguishable allowed C64 background/text/link colour triplets. Confirm plain normal-browser HTML is unchanged in purpose. Use website administration for colour changes, not Editor or Publisher.
2. Test HTTP `Accept: application/x-see64you-browser` (legacy v1) and `Accept: application/x-see64you-browser; version=2` (v2 mandatory colour record); malformed records must be rejected. Avoid touching production data.
3. Launch `S64WEB.PRG` on real C64U and browse site A, follow text/link navigation including a cross-site link to site B, and inspect both palette presentations. Confirm link selection/number navigation/paging, legacy-v1 defaults, return to A if relevant, and restoration of the **original** C64 screen colours upon normal exit.
4. Exercise known bounded 404, stale-link, delayed response/cancellation and HTTPS refusal cases on the disposable site. Preserve/readback SD, IRQ, drives, BASIC return and turbo.

Existing automated baseline (on disposable fixtures; inspect script guards first):

```sh
php tools/create_browser_test_site.php client/build/browser-test-site
python3 tools/test_browser_c64u.py --host C64U_IP \
  --site-url http://WEB_HOST/SEE64YOU/client/build/browser-test-site/ \
  --output client/build/NEW-browser-hardware-receipt
```

That existing harness does **not** by itself prove two-site colours. If needed, add only narrowly scoped fixture support/tests; do not redesign the browser or protocol.

**Gate C:** correct site/link/palette behaviour and machine restoration on exact matched hardware package.

## D. Limited negative checks and cleanup

Run only existing bounded **non-destructive** tests for partial/short HTTP body, missing document, destination collision, invalid/trailing config, auth refusal, recovery refusal and safe cancellation. Do not force actual SD removal or physical power loss. Never assert delete-then-rename is fully atomic: a completed temporary may survive while the destination is missing. Avoid running storage-fault diagnostics unless their fixture path preconditions and backups have been individually verified.

Fix only reproducible MVP defects. After a fix, repeat affected host/VICE and C64U acceptance with the **newly rebuilt matching package**; prior hardware passes do not transfer automatically to different binaries. Then restore original files and machine state with exact readbacks.

## E. Evidence and handback — required report

Produce one redacted `client/build/` or other ignored local receipt directory with logs, exact tested commit, package file hashes, PHP/site version, firmware/core/REU, input mechanism, C64 screen evidence, disposable site URL(s), per-step outcome, known limitations and cleanup/restore results. Do **not** commit secrets, user configuration, backup files or large raw binary receipts.

Report a short matrix with statuses **PASS / FAIL / BLOCKED / UNVERIFIED** for:
- software/VICE and release ZIP verification;
- first-run Settings wizard / persistence;
- edit/save/reopen and BASIC return;
- publish / modern HTML / remote download / update;
- dual-site Browser v1/v2 colours and cross-site links;
- negative cases and machine/file restoration;
- *physical* keyboard usability, physical cold restart and uncontrolled interruption (normally **UNVERIFIED/DEFERRED**).

Give the first blocking defect and smallest next fix; list exact commits and test commands run. Update only the authoritative `todo.md` and affected canonical docs. Do not report beta release readiness until all **automatable** release gates pass and remaining human-only observations are explicitly separated. If major redesign is necessary, recommend **shelving or reassessment**, not another feature program.

## After acceptance (not part of this hardware test)

Only *after* Gate A/B/C passing: measure unused Editor-launcher Publisher/Settings preloads and old managed-runtime targets, trace callers, remove provably dead code in small steps and revalidate. No replacement with Compunet, CML or new test frameworks without a demonstrated benefit.
