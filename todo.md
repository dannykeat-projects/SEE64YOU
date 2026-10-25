# SEE64YOU work queue

**Objective:** ship a small, reliable, offline-first C64 Ultimate 8TML Editor with optional self-hosted publishing. The separate 8TML/T8WN ecosystem provides the compatible browser. See [product design](docs/PRODUCT.md) for scope and interface decisions. Check boxes describe remaining work; Git history and test receipts preserve completed investigations.

## P0 — Publication write safety

- [x] **Create/update intent (software validated):** server-enforced create-only saves and explicit identity/existing-target updates share the write lock. Publisher confirms replacements and supports updating an explicitly entered slug after local reopen. PHP/HMAC/native tests cover collision, missing/mismatched identity, missing-target refusal and simultaneous creators. No installed files or service changed.
- [x] **Stale-update protection (software validated):** updates compare opaque revisions under the write lock; stale writers cannot overwrite source or metadata. Publisher retains open/save revisions and probes explicitly entered existing slugs before confirmation. PHP/concurrent-writer/HMAC/native tests passed. Forced SD and `/USB1/` packages preserve the 1,024-byte Publisher/launcher guards. Real-C64U publication/confirmation acceptance remains required before deployment.

## P0 — External 8TML reader interoperability

- [x] Published-only source endpoint: `source.php?type=page&slug=SLUG` / `type=post`, exact ASCII/LF, MIME/length, GET/HEAD, no redirects, no-store/nosniff. HTTP tests cover maximum size, drafts, unpublish/republish/delete, invalid identities and failed updates; no public static copies.
- [x] Bounded real-C64U VVV interoperability: VVV 1.0 at `a3c87cf` fetched a disposable SEE64YOU published-source endpoint over SwiftLink HTTP/1.0, accepted `text/8tml`, parsed the page and followed a relative query-string link to a second endpoint; user confirmed the display works. SEE64YOU `473f1b3`, browser SHA256 `33eda8e04aece441051a63925632a8354f143b8d4122c1661ce846f1831324f2`; local receipt `client/build/vvv-interop-20261023/`. No machine settings changed or SEE64YOU package files replaced during the browser test.
- [ ] Extend external-reader acceptance to independently hosted source, redirects, larger documents and complete Editor-save/Publisher-upload/read workflow. Physical browser controls, cold restart and BASIC/machine restoration remain unverified; the current VVV build has limited page memory.

## P0 — Prove the current user workflow

- [x] Automated editing robustness: 10,800 seeded byte-reference edits, 36 size/pattern boundaries through 61,440 bytes, clipboard/capacity and multi-level history checks; fault tests cover partial paste and undo/redo failures. Menu ghosts over short documents are reproduced/fixed with shared modal redraw invalidation. Partial REU failures remain recoverable dirty text, not atomic rollback.
- [x] Bounded real-C64U Editor acceptance: injected function-key selection/copy/Help/cut/paste/replacement, byte-exact source saves/recovery, no background autosave, native DOS collision/failure/recovery and BASIC return passed. All 40 original files and captured DOS/IRQ/turbo/colours restored and verified. Receipt: `client/build/editor-audit/hardware-20261024-final/`. This is not physical keyboard or cold-power acceptance.
- [ ] Reduce measured source-codec and long-line/large-document edit costs. Base-C64 VICE shows a maximum-size read plus validation takes about seven minutes and serialization another five, excluding DOS; avoid per-byte/repeated scans using a bounded, measured change. Match source correctness, fit and C64U turbo/UI evidence before promotion.
- [ ] Complete maximum-size native DOS open/save/reopen acceptance. The initial shorter hardware timeout produced no verified output; it does not prove a hang. Use explicit `--large-source` with the longer window after performance investigation.

- [x] Physical C64 keyboard checks: typing, cursor repeat/shifted directions, Help/menu dismissal, clipboard copy/cut/paste, undo/redo, modified-exit cancellation, menu Save As and full-filename Open passed. Exit reached responsive BASIC; original installed files and captured DOS/IRQ/turbo/colours/configuration restored and verified. Preserve `keytest25.8tml`. Receipt: `client/build/editor-audit/manual-20261025-r2/`.
- [ ] Recheck native Commodore shortcuts on hardware: Commodore+M and Commodore+F7 failed the manual run. Decoder now accepts native graphics M and shifted function codes; 18 VICE decoding cases pass. New/Open/Save/Save As with Commodore need physical acceptance of the rebuilt package; do not infer modifier timing from injected bytes.
- [ ] Cold-power saved-file reopen and permitted uncontrolled-interruption/recovery behavior; verify byte-exact source and machine state.
- [ ] Real-C64U Publisher acceptance: local 8TML load, page/post, draft/published, first slug, authenticated upload/update, remote identity/status, download to a new filename, failure/collision recovery and clean BASIC/machine restoration. Only host/PHP/VICE integration has been exercised for the new source path.
- [ ] Modern-site 8TML HTML output: pin supported constructs, implement or integrate a maintained complete safe 8TML renderer where justified, and validate links/styles/escaping/drafts on a disposable host. Do not claim full conformance until tested.
- [ ] Test the same source with an external compatible T8WN/8TML browser and an independently hosted source URL; record the exact ecosystem version and behavior. **Do not adapt `S64WEB.PRG` into a new browser.**

## P1 — Simplify after identifying callers

- [x] Reorder the primary Publisher screen around choose local file, page/post + visibility, Publish/Update and result. Connection testing happens automatically before first publication; Settings remains the configuration owner. Existing remote download/maintenance keys remain secondary; no handlers or legacy runtime paths were deleted.
- [ ] Complete real-C64U acceptance of the new publish/update confirmation and reordered screen.
- [ ] Keep Settings for setup; assess making it a simpler one-time wizard. Retain an explicit source-download/recovery path as a secondary action if the Editor cannot otherwise reopen the only remote copy.
- [ ] Validate and promote removal of the browser executable from the active package: forced clean SD/USB builds, exact ten-file inventories, module/overlay manifests, Publisher/launcher fit, packaged VICE startup/BASIC return and bounded real-C64U storage/machine-restoration. Forced SD and `/USB1/` builds, exact ten-file inventories, both manifests, memory guards and packaged VICE startup/BASIC return passed on 22 October. Matching ten-file hardware acceptance is still pending; the release packager now rejects eleven-file hardware receipts. Retain separate diagnostic source and site wire code until caller traces justify removal.
- [ ] Trace WordPress, relay, managed runtime and unused Publisher preview/import/export/delete handlers. Retire only proven-unused active paths without weakening authentication, storage or updated-document safeguards.
- [x] Consolidate duplicate release/install text into [one installation guide](docs/INSTALL.md); revise release packaging to embed that verified guide rather than requiring per-version narrative. Keep published tags/assets unchanged.
- [ ] Consider reducing installed program/file count only after matching module-manifest and real-hardware acceptance, not by deleting companion files from the current eleven-file package.

## P2 — Remaining compatibility and infrastructure

- [ ] Investigate USB-only reliability: actual tested mount `/USB1/`, byte-corrupting FTP transfers with unpaced uploads on tested firmware. Paced small chunks can verify readback but do not establish full-package runtime safety. Require exact transfer hashes before execution; confirm a firmware/FPGA fix before recommending updates.
- [ ] Validate portable static hosting and source-to-HTML conversion independent of a proprietary publishing account or API. Treat 8TML specification and validator redistribution/licensing as unverified until clarified.
- [ ] Run full release acceptance on the exact built package: targeted suites, host/PHP/HTTP, VICE entry/BASIC return, SD and selected USB-root manifests/fit, relevant real-hardware evidence and recoverable rollback. Mark outstanding gates explicitly.
