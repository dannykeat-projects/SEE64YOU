# SEE64YOU — current MVP validation runbook

**Purpose:** Reproduce and accept the **independent eleven-file C64U application package** and flat-file PHP website. This is a runbook, not a chronology. Historical managed-runtime test campaigns and one-off receipts remain in Git history. `todo.md` is the only live work queue; passing a past commit does not certify a rebuilt package.

## 1. Test environment and safety

- Read root `AGENTS.md`, current `todo.md` and [C64U-RUNTIME.md](C64U-RUNTIME.md) before changing runtime behaviour.
- Build with cc65; PHP site checks require PHP 8.1+; VICE checks need a working VICE installation, compatible local C64 ROMs, Python and (when headless) the project's documented display/Xvfb setup.
- Use a **disposable configured PHP site** accessible to the hardware, a **backed-up test SD installation**, and the same exact matching eleven-file package for any end-to-end acceptance. Avoid shared or production content.
- Back up existing `/sd/SEE64YOU/` files, legacy SD-root config/journals/recovery, the currently installed PRGs and the user's original C64U machine state; record file hashes, drives/IRQ, live turbo and clock where applicable. Never overwrite, remove or reset unrelated files or machine settings for a test.
- Hardware automation with REST/FTP may inject plain key presses and can reset the machine. **Do not call those physical-keyboard tests**; record the actual input mechanism and whether app-owned restoration or harness reset changed state.

## 2. Local build, PHP and host checks

From the repository root (use LF line endings for shell scripts under WSL):

```sh
make -C client independent-package
sh tools/validate.sh
php site/tests/run.php
php site/tests/browser.php
php site/tests/themes.php
sh site/tests/http.sh
php tools/package-site.php
```

The release candidate must meet existing launcher/Publisher/Settings/Browser memory guards, Editor overlay-slot protection, both manifests, native parser/source/config/integrity tests, site rendering and actual HTTP negotiation. A failure in an obsolete *managed* runtime target is not itself an independent-release failure; determine whether any shipping path still depends on it before discarding the failure.

Outputs: `client/build/independent-package/SEE64YOU/` for matching client files, and `dist/see64you-site/` for the upload-ready PHP site. Preserve the actual exit status, tested commit, warnings and SHA-256 of output ZIP/individual files; a developer artifact is **not** yet a public release.

## 3. Packaged VICE checks

```sh
python3 tools/test_independent_programs_vice.py --rom-dir LOCAL_ROM_DIRECTORY
python3 tools/test_basic_return_vice.py --rom-dir LOCAL_ROM_DIRECTORY
```

Use supported local C64 ROMs; old pinned Open ROM test material is not a substitute when conio startup differs. Require all four entry/exit paths and numeric **and string** BASIC commands. Check absent UCI/DOS/network paths, URL refusal, Settings wizard required fields/consent/cancel, browser stale-link guards and returned memory state. VICE does **not** prove UCI, physical disk/network, real key input or cold persistence.

## 4. Integrated first-user real-C64U acceptance — required for public beta

Use the actual instructions in [FIRST-PAGE.md](FIRST-PAGE.md) and the PHP site's six-step `start.php` walkthrough, not developer shortcuts. Start with a disposable, unconfigured site and test installation. Use the **native physical keyboard** for acceptance:

1. Enable C64U prerequisites, verify the 16 MiB REU and app folder, and copy the complete matching eleven-file package.
2. Configure the PHP site with `setup.php`, pair using HTTPS `pair.php`, import or manually enter the dedicated key using `S64CFG.PRG`, explicitly consent to plain HTTP publishing, save, restart/relaunch and confirm Settings persistence.
3. Launch `SEE64YOU.PRG`, create/save `/sd/SEE64YOU/FIRST.S64` and cleanly return to BASIC.
4. Fresh-launch `S64PUB.PRG`, load `FIRST.S64`, authenticate and publish a **public** page/post (not a draft). Verify normal HTML and exact text from another modern browser.
5. Fetch its canonical remote source, save to a **new** `.s64`, reopen/edit/save in Editor, and update that same remote document with Publisher. Verify canonical title/slug/type/body and that old/temporary sources are not silently overwritten.
6. Optionally browse this site from `S64WEB.PRG`; browser package acceptance additionally requires **two different independent sites** with different C64 colour palettes, v1 fallback, links/paging and clean colour restoration. Confirm unencrypted/HTTPS limitations are displayed rather than hidden.
7. Exit all programs into responsive BASIC; execute both a numerical and string BASIC command. Verify original drives/IRQ/clock/live turbo, existing SD files/config and valid CLEAN recovery state are preserved to the supported extent.

Record firmware/core versions, commit, package hashes, physical-keyboard observations, exact URLs (without credentials), visual issues, actual source files, restoring steps and each *passed / failed / blocked / unverified* gate. A success screen in a setup wizard alone is not a successful publish.

## 5. Browser and theme acceptance

Automated tests in section 2 include the public HTML path, exact `Accept: application/x-see64you-browser` v1, `Accept: application/x-see64you-browser; version=2` v2 with the required colour record, and malformed negotiation/input rejection. All site colours are selected from the 16 C64 values; legacy saved site settings are preserved.

For disposable configured theme UI, verify: 16 named swatches and arbitrary colour rejection; Plain/Simple and custom theme lifecycle; text/link contrast and visited/focus readability; layout/font/settings persistence; C64 built-in preset is no longer offered; previews don't alter published themes; no-JavaScript forms; readable fallback without webfont/CSS; 390px responsive controls and keyboard focus.

For a reproducible controlled browser hardware gate, prepare a **new disposable fixture** rather than pointing the runner at real user content:

```sh
php tools/create_browser_test_site.php client/build/browser-test-site
python3 tools/test_browser_c64u.py --host C64U_IP \
  --site-url http://WEB_HOST/SEE64YOU/client/build/browser-test-site/ \
  --output client/build/NEW-browser-hardware-receipt
```

The runner checks public home/page/archive/post navigation, paging, delay/cancellation, 404/stale links, HTTPS refusal, reopen, BASIC return and restoration. Review its `restored` result and exact backups. This existing runner does not by itself prove **two cross-domain palette configurations**; that remains a separate physical test.

## 6. File integrity and legacy migration (when affected)

- All new source/downloads, config, recovery, journal and preflight temp files live under `/sd/SEE64YOU/`; bare filenames must not resolve to same-named old SD-root files. Explicit `/sd/NAME.S64` retains its root origin. A failed open must not silently switch sources.
- Test failed `.s64` reads, existing download collisions, short HTTP response, config truncation/trailing bytes and denied rename/verification. Successful source/config must remain exact; a completed `.tmp` can be recoverable after failure. Delete-then-rename can leave the destination absent: never describe it as a guaranteed atomic replacement.
- Old root journals block Editor and require **explicit** Settings `M` then `Y` migration. Migration must verify ACTIVE machine restoration, durable CLEAN, exact retained `S64OLD*.DAT` backups and file retirement. Conflicts or unreadable state must block it. `SEE64YOU.REC` from SD root is not silently migrated/deleted; copy and compare recovery snapshots before manual action.
- Maintain bounded host and packaged VICE checks for those paths. For a dedicated guarded real-DOS negative check, use `python3 tools/build_independent_storage_faults.py` **only on a disposable backed-up SD**, after inspecting its fixture-name prerequisites. It isn't part of the release package.

## 7. Existing evidence and remaining gaps

- Independent Editor/Publisher/Settings package: three bounded functional edit/publish/download/update real-C64U round trips on 2026-10-10/11; exact metadata/body, truncated-response rejection, storage/config fault handling, BASIC and turbo 0/15 restoration checked. These were not three fresh-install manual sessions.
- Public browser: separate bounded real-C64U result on 2026-10-11 (`client/build/hardware-browser-final-20261011/state.json`), nine checks passed, original SD restored and IRQ/drives/turbo 0 parity recorded.
- SD-folder package install/journal migration and settings first-run/import/consent/readback were hardware tested in scoped 2026-10-11 receipts. The physical wizard key path plus complete cold first-publication walkthrough remain **unverified**.
- New v2 site palette/browser code and eleven-file candidate passed automated PHP/host/cc65 validation 2026-10-12, but full current package VICE and two-site physical C64U colour acceptance remain **unverified**.
- USB keyboard acceptance and uncontrolled SD removal/power interruption are user-deferred; do not claim them passed.

Do not ship solely from prior recorded outcomes. Gate public artifacts on section 4, relevant section 5/6 checks and the **exact** current release binary identities. Archive old detailed receipt narratives in Git history rather than duplicating them here.
