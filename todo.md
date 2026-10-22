# SEE64YOU work queue

**Objective:** ship a small, reliable, offline-first C64 Ultimate 8TML Editor with optional self-hosted publishing. The separate 8TML/T8WN ecosystem provides the compatible browser. See [product design](docs/PRODUCT.md) for scope and interface decisions. Check boxes describe remaining work; Git history and test receipts preserve completed investigations.

## P0 — Prove the current user workflow

- [ ] Physical-keyboard acceptance for New/Open/Save/Save As, modified-document exit, Help, selection and typing latency. Verify actual C64 modifier Open controls; REST key injection is not this gate.
- [ ] Cold-power saved-file reopen and permitted uncontrolled-interruption/recovery behavior; verify byte-exact source and machine state.
- [ ] Real-C64U Publisher acceptance: local 8TML load, page/post, draft/published, first slug, authenticated upload/update, remote identity/status, download to a new filename, failure/collision recovery and clean BASIC/machine restoration. Only host/PHP/VICE integration has been exercised for the new source path.
- [ ] Modern-site 8TML HTML output: pin supported constructs, implement or integrate a maintained complete safe 8TML renderer where justified, and validate links/styles/escaping/drafts on a disposable host. Do not claim full conformance until tested.
- [ ] Test the same source with an external compatible T8WN/8TML browser and an independently hosted source URL; record the exact ecosystem version and behavior. **Do not adapt `S64WEB.PRG` into a new browser.**

## P1 — Simplify after identifying callers

- [ ] Map actual Publisher command usage, state/remote-identity logic, direct authentication and all package/build/test dependencies. Propose and implement the [minimal guided publish/update interface](docs/PRODUCT.md) in small reversible changes; default to choosing a local file, selected destination, visibility and confirmed remote identity.
- [ ] Keep Settings for setup; assess making it a simpler one-time wizard. Retain an explicit source-download/recovery path as a secondary action if the Editor cannot otherwise reopen the only remote copy.
- [ ] Validate and promote removal of the browser executable from the active package: forced clean SD/USB builds, exact ten-file inventories, module/overlay manifests, Publisher/launcher fit, packaged VICE startup/BASIC return and bounded real-C64U storage/machine-restoration. A focused source change is prepared but is not yet hardware accepted. Retain separate diagnostic source and site wire code until caller traces justify removal.
- [ ] Trace WordPress, relay, managed runtime and unused Publisher preview/import/export/delete handlers. Retire only proven-unused active paths without weakening authentication, storage or updated-document safeguards.
- [ ] Consolidate duplicate release/install text into [one installation guide](docs/INSTALL.md); revise release packaging to embed that verified guide rather than requiring per-version narrative. Keep published tags/assets unchanged.
- [ ] Consider reducing installed program/file count only after matching module-manifest and real-hardware acceptance, not by deleting companion files from the current eleven-file package.

## P2 — Remaining compatibility and infrastructure

- [ ] Investigate USB-only reliability: actual tested mount `/USB1/`, byte-corrupting FTP transfers with unpaced uploads on tested firmware. Paced small chunks can verify readback but do not establish full-package runtime safety. Require exact transfer hashes before execution; confirm a firmware/FPGA fix before recommending updates.
- [ ] Validate portable static hosting and source-to-HTML conversion independent of a proprietary publishing account or API. Treat 8TML specification and validator redistribution/licensing as unverified until clarified.
- [ ] Run full release acceptance on the exact built package: targeted suites, host/PHP/HTTP, VICE entry/BASIC return, SD and selected USB-root manifests/fit, relevant real-hardware evidence and recoverable rollback. Mark outstanding gates explicitly.
