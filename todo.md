# SEE64YOU work queue

## Direction (20 October 2026)

8TML 1 is the sole format for **new** source documents (`.8tml`). The old `.s64` document model is retired: no importer, exporter, compatibility mode or new `.s64` documents. An `.8ss` stylesheet is optional. Preserve useful existing editor/UI/storage code, but do not preserve obsolete format machinery for its own sake.

The supplied `specs.zip` includes the **8TML 1 draft (20 October 2026)**, examples, and Python reference validator. It is an external specification, not copied into this repository while its licence is unsettled. The format remains a draft: track changes before declaring compatibility complete.

## P0 — Inspect and adapt, in small verified steps

- [x] Establish 8TML migration branch and authoritative migration queue.
- [x] Identify the maintained builds and working persistence. Task branch `codex/8tml-editor` starts at `origin/codex/browser-palette-fix` (`6f21cfc`), preserving its later independent-program/storage/restoration fixes. The migration branch `78c13c1` contains documentation only; its contract/README/queue are incorporated here. `feature/c64u-reu-editor` predates the independent package; no bulk branch merge. Open PR search returned none.
- [x] Implement minimal compliant new `.8tml` source in the independent Editor: editable version/title header, cursor in the body, ASCII/LF boundary conversion, `.8tml` default and explicit-extension enforcement. Real C64U disk creation remains below.
- [x] Open, edit and resave complete source without reconstructing header keys or interpreting markup. Unknown metadata/markup, whitespace, case and tabs survive exactly; CR/CRLF normalize to LF. Preserve existing disk transaction, collision refusal, recovery snapshot and retained temporary-file recovery; no old-format parser/fallback in the independent Editor.
- [x] Add `tools/test_8tml.py`: strict host codec/storage tests, actual REU buffer at 61,440-byte capacity, every host chunk split, invalid bytes/headers, empty body, filename boundaries, failed writes/rename and recovery. External draft validator (2026-10-20) reports zero errors; deliberate unknown-markup fixture has one unclosed-emphasis warning. Complete eleven-file package, unchanged fit guards and manifests pass. FILE overlay 13,258/15,360; resident BSS `$89BD` below `$8C00`. Native cc65/PETSCII codec and BASIC return pass VICE; physical C64U format testing remains pending.
- [x] **2026-10-21 REST-assisted real-C64U acceptance of product `1b9cb75`:** shipping Editor creates/saves basename `.8tml`, recovery-reopens, edits and resaves exact source; native real-UCI/DOS/REU probe explicitly opens/saves unknown metadata/markup, normalizes CRLF, handles empty source, 33-character final filenames, missing/invalid files, collision refusal and injected WRITE/RENAME failures with retained-temp recovery. Actual generated files pass the supplied validator with **zero errors, zero warnings**. Numeric/string BASIC, IRQ/drives/clock/live turbo pass after each program; all 38 original app-folder files, full configuration, colours and both DOS directory contexts are restored/readback verified. Private receipt: `client/build/8tml-hardware-20261021-a/state.json` (`restored=true`, `active=false`, `dos_context_restored=true`, no failure). Reusable `tools/test_8tml_c64u.py` builds a disposable probe; shipping package stays eleven files. No production-code change was needed.
- [ ] Physical-keyboard usability and the Editor's modifier-key Open shortcut, cold power restart and uncontrolled-interruption acceptance remain unverified. The user was remote; REST injection/recovery reopening and native storage-service open are not physical-keyboard evidence. No SD removal or reset was performed. Host/REU/native codec, package/manifests and packaged VICE gates remain passing.
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
