# SEE64YOU — MVP release queue

**Only active work queue.** The product is frozen to a small public beta; shelve non-MVP features until real user feedback. Historical investigations live in Git history, not in this queue.

## Scope (four independently launched programs)

- **Editor** (`SEE64YOU.PRG` with private assets): edit and save/reopen canonical `.s64` source files in `/sd/SEE64YOU/`, useful offline; return cleanly to BASIC.
- **Publisher** (`S64PUB.PRG`): load a saved `.s64`, authenticate, publish/update to the standalone site, download remote source to a *new* `.s64`, return to BASIC.
- **Settings** (`S64CFG.PRG`): guided first-run site/key setup, verified disk persistence and optional existing-file migration, return to BASIC.
- **Browser** (`S64WEB.PRG`): optional, read-only, follows numbered/selected links on public SEE64YOU sites; supports C64-compatible site colours. It is **not** a general HTML or HTTPS browser.
- **Website**: independent flat-file PHP 8.1+ hosting; public HTML for normal browsers and a compact public text/link representation for C64U; user-adjustable C64-palette colours and saved themes.

Programs share only ordinary on-disk source/configuration, **not** live application state or REU handoffs. Preserve the existing document, browser and authentication protocol versions. C64 Ultimate with UCI, DOS, required 16 MiB REU and Network Target $03 is the tested platform. The browser and Publisher use direct HTTP; HTTPS is never silently downgraded. Direct challenge/HMAC protects authenticated writes but does **not** encrypt content.

## P0 — decisions and acceptance before release

- [ ] **Reproduce current candidate:** pin one development commit, run complete eleven-file `make -C client independent-package`, manifests/memory guards, host/PHP tests and the applicable packaged VICE gates. Record exact commit, package SHA-256 and test outcomes. Do not confuse earlier hardware receipts with validation of a rebuilt package.
- [ ] **First-user walkthrough on backed-up real C64U:** from an unconfigured disposable PHP website and test SD installation, follow `site/start.php` and `docs/FIRST-PAGE.md` with the physical C64 keyboard: configure prerequisites, HTTPS-pair key, save/reopen Settings after cold start, create/save `FIRST.S64`, publish, verify ordinary-browser HTML, download/reopen/edit/update, then return to responsive BASIC. Record friction, screen messages, source integrity, SD contents and original machine settings. Fix only concrete blocking defects.
- [ ] **Browser/theme acceptance:** run current PHP/theme/browser regression suite and built-package VICE checks, then visit two independent public SEE64YOU sites with different C64 colour settings on real C64U; verify correct colours, URL/link navigation, legacy v1 fallback, restoration of screen colours and clean BASIC return.
- [ ] **Release safety:** verify no data loss under existing bounded negative cases (existing destination refusal, partial response, config errors), exact documents and configuration, machine restoration and stable startup/exit. Document remaining hardware gaps explicitly; USB keyboard and uncontrolled power/SD removal are deferred, not passed.
- [ ] **Go/no-go:** if ordinary first publication still needs developer intervention or a major architecture rewrite, stop and evaluate shelving. No further broad redesign to save the MVP.

## P1 — bounded cleanup and beta publication

- [ ] **Remove provably dead build/runtime paths** only after P0: trace references and release dependencies first. In particular, assess unused Publisher/Settings preloads in the Editor launcher and old cross-program transition targets. Never remove the working Launcher→Editor bootstrap, journal/recovery, UCI transport, protocol, integrity gates or protected hardware restoration merely to shrink the tree.
- [ ] **Retire stale generated diagnostics and consolidate documentation.** Keep one live runbook, minimal README, current architecture and only meaningful required tests. Preserve unique verified recovery procedures in their owning technical docs; use Git history for superseded experiments.
- [ ] **Reconcile development with `main`** without rewriting history or dropping the main-only README change; review full diff and validation on the intended release commit.
- [ ] **Publish versioned artifacts only after acceptance:** eleven-file C64U package and standalone PHP website ZIP as GitHub Release assets; include immutable commit, SHA-256 hashes, basic install/first-page instructions, exact known limitations. No deployment or merge solely because CI is green.

## Established evidence (not new validation)

- Three functional real-C64U edit/publish/download/update round trips and bounded storage/network failures were recorded 2026-10-10/11; the tested native keyboard workflow was not a complete fresh-user setup.
- Independent public browser navigation/paging/cancellation/404/HTTPS-refusal and clean BASIC return were recorded on hardware 2026-10-11.
- SD-folder migration and Settings persistence, including cold power-cycle checks, were recorded under bounded real-hardware procedures.
- C64 site colour protocol v2, legacy v1 compatibility, PHP/browser regressions and eleven-file packaging passed focused automated checks on 2026-10-12; **the full two-site real-C64U colour acceptance remains outstanding**.
- Refer to `docs/LOCAL_TESTING.md` for runnable checks, prior package identities and receipt interpretation. Hardware pass claims apply only to exact tested builds.

## Explicitly outside MVP

WordPress publishing, ordinary-C64 compatibility, arbitrary HTML/CSS/JS browsing, HTTPS/TLS client redesign, mouse/joystick, USB keyboard acceptance, expanded themes/editor colour controls, new document models, full editor rewrite, large-document allocator redesign, spellcheck, accounts/chat, and further Compunet/CML experiments. Reconsider solely in response to actual user feedback or a demonstrated release blocker.

**Stop rule:** two bounded attempts at the same blocker without improvement trigger a review; never start another managed-runtime/persistent cross-program architecture. Preserve unrelated changes and don't clean SD or rewrite file formats without explicit approval.
