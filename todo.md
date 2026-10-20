# SEE64YOU work queue

## Direction (20 October 2026)

8TML 1 is the sole format for **new** source documents (`.8tml`). The old `.s64` document model is retired: no importer, exporter, compatibility mode or new `.s64` documents. An `.8ss` stylesheet is optional. Preserve useful existing editor/UI/storage code, but do not preserve obsolete format machinery for its own sake.

The supplied `specs.zip` includes the **8TML 1 draft (20 October 2026)**, examples, and Python reference validator. It is an external specification, not copied into this repository while its licence is unsettled. The format remains a draft: track changes before declaring compatibility complete.

## P0 — Inspect and adapt, in small verified steps

- [x] Establish 8TML migration branch and authoritative migration queue.
- [ ] Identify the actual maintained Editor/Publisher/Browser builds and all save/open code across `main` and outstanding feature branches; check PRs and tests before reusing implementations. **Warning:** `main/client/src/main.c` currently has a save placeholder, not working persistence.
- [ ] Create a minimal compliant `.8tml` document on a C64U-accessible device. First line `---`, first key `8tml: 1`, closing `---`, LF, ASCII, title, body. Treat PETSCII/ASCII conversion at the file boundary; do not export screen character codes raw.
- [ ] Open and edit the saved `.8tml` source without losing the header, source text or formatting.
- [ ] Introduce a regression fixture/check that validates produced files with the supplied reference validator (keep the validator external pending licence clearance); add independent byte-level tests for ASCII, line endings, and header.
- [ ] Compile/test selected editor target with host/VICE tooling, then verify save/reopen and document bytes on a physical C64U.
- [ ] Adapt Publisher to transfer unchanged 8TML source through a narrow authenticated publishing API. Confirm networking and HTTPS termination separately from document handling.
- [ ] Adapt Browser to render 8TML, preserve numbered links and C64 palette mapping, and reject unsafe or excessive input.
- [ ] Test full cycle: C64U edit/save -> publish -> modern web view -> C64U fetch/display -> edit source again.

## P1 — Reduce and release

- [ ] Remove obsolete `.s64` document code and old proprietary rendering only after the relevant new path works; do not delete current functioning code blindly.
- [ ] Review WordPress integration against the minimal independent MVP; defer or remove it unless justified.
- [ ] Update README, architecture, release instructions and assets to reflect *actual* validated capabilities, not intended ones.
- [ ] Keep documentation consolidated and retire superseded planning records after implementation and test evidence are recorded.

## Boundaries

Keep the three-program model (Editor, Publisher, Settings) unless a tested simplification improves it. The Browser remains a separate reader. No mandatory JavaScript or database for the standalone site. No platform-specific extensions to 8TML for MVP; don't fork the format. No hardware or production validation is claimed until it is run.
