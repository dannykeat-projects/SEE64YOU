# 8TML adoption — implementation contract

**Decision:** SEE64YOU uses the external 8TML 1 draft (2026-10-20) as its only new source document format. No `.s64` migration path. Editor implementation status follows; publishing interoperability and physical hardware acceptance remain separate gates.

## Implemented Editor slice (21 October 2026)

`codex/8tml-editor` builds on maintained editor commit `6f21cfc`, incorporating this migration contract from `78c13c1` without merging the older default-branch code. No open PRs were found during the refreshed inspection. The older REU editor branch lacks the later independent package/storage fixes.

The independent Editor now edits the entire source, including its header. New documents begin with `---`, `8tml: 1`, `title: Untitled`, closing `---` and an empty body; typing begins in the body. Readers preserve unknown keys/markup as literal source, deliberately normalize CR/CRLF to LF and reject nonrepresentable bytes. Writers validate ASCII and a closed version-1 header before invoking the existing transactional storage path. Save As defaults to `.8tml`, accepts case variants and refuses other explicit suffixes. Open identifies content by the header, including recoverable `.tmp` source. The header counts toward existing capacity; no metadata side buffer, rendering engine or old-format compatibility was added.

The same shared source file still supplies historical serialization for unmigrated programs/diagnostics under their existing build modes. Independent Editor and Publisher builds exclude that serializer/parser. Browser has not been migrated; Publisher and PHP source-service details follow. The built package is a development test artifact, not a public publishing release.

**Real C64U continuation (2026-10-21):** product `1b9cb75` passed REST-assisted shipping-Editor creation/basename save, recovery reopen/edit/resave and exact FTP byte checks on firmware 1.1.0 / FPGA 122 / core 1.49 with 16 MiB REU. A separate native diagnostic using the actual codec/local-storage/UCI/REU code passed explicit file open/edit/save, unknown metadata/markup retention, CRLF normalization, empty files, maximum filename suffix, missing/invalid input, overwrite refusal and injected WRITE/RENAME transport failures with retained-temp recovery. The three actual generated files passed the external validator with zero errors and zero warnings. Every completed exit passed numeric/string BASIC, IRQ/drives/clock/live-turbo checks. All 38 originals, configuration, colours and both DOS directory contexts were restored and verified. Private receipts/backups are under `client/build/8tml-hardware-20261021-a/`; `state.json` records `restored=true`, `active=false` and `dos_context_restored=true`. `tools/test_8tml_c64u.py` reproduces the bounded test on explicitly authorized idle hardware. No shipping source change was required. Native physical-keyboard input, the modifier-key Open shortcut, cold power and uncontrolled interruption remain unverified; the user was not near the hardware.

`python3 tools/test_8tml.py --reference-tools client/build/8tml-reference/specs/tools --rom-dir client/build/private-vice-roms` reproduces host, REU, external reference and cc65/VICE codec checks. The external command is `python3 client/build/8tml-reference/specs/tools/8tml check -v client/build/8tml-empty.8tml client/build/8tml-roundtrip.8tml` against the supplied internal draft reviewed 2026-10-20. It reports zero errors; the exhaustive source-preservation fixture deliberately includes unknown constructs and an unclosed-emphasis warning. The validator is unmodified and remains ignored build material, not licensed repository content. See `docs/LOCAL_TESTING.md` for procedures and `todo.md` for physical C64U and subsequent-program gates. The inventory/checklist below records the pre-implementation audit.

## Implemented Publisher slice (22 October 2026)

The independent Publisher loads `.8tml` through the shared codec/local storage, streams full source as JSON `raw8tml`, opens raw `.8tml` from the authenticated service and saves downloads to a new file with overwrite refusal. New local uploads ask for a remote slug; optional title and unknown headers remain in source. A raw response carries publication state in `X-See64You-Status`, outside source; the client requires it and restores the selected page/post identity only after a valid open. Source-only lists exclude other records. Omitted date/order retain the current service values under the existing write lock. Editor remains independent and offline.

The production Publisher/REU code passes a real local PHP create/list/open/update/delete cycle. A 61,440-byte source produces a 123,219-byte escaped request with correct HMAC/Content-Length and identical download. Empty source bodies, unknown constructs, draft/published state, page/post identity, invalid input, failed transport, missing/malformed/duplicate status headers and metadata preservation are covered. The three actual exports pass the supplied validator with zero errors/warnings; native cc65/PETSCII serialization and BASIC return pass VICE. Shared local-storage tests pass for both programs, including collision/write/rename/recovery failures. Real-C64U Publisher acceptance remains pending; the user's machine stays powered off. Browser/full rendering work has not begun.

## Source and legal boundary

The user supplied `specs.zip` containing `specs/spec/8tml-1.md`, `specs/spec/examples/*.8tml`, optional `.8ss` files and `specs/tools/8tml` with Python parsing/rendering tooling. Use these as normative development references. Do **not** copy the reference code or extensive examples into the public repository without licence clarification. Independently written, minimal test fixtures and byte-level tests are acceptable; verify against the external validator in the local test environment.

## Format fundamentals to implement first

- Pages end in `.8tml`. The first source line is exactly `---`, followed by `8tml: 1` as the first header key; a second `---` ends the header.
- Source is 7-bit ASCII (bytes 0–127); printable content 32–126; LF line endings. CRLF is readable under the draft, but the editor should emit LF. Source lines SHOULD be <=255 characters.
- Preserve ordinary source markup literally in the editor. The file is **not** PETSCII screen memory, HTML or an S64 container. Convert input/output between machine representation and ASCII deliberately, including Commodore case/graphics limitations.
- Header keys are case-insensitive and unknown keys must remain intact on round trips. Only generate a minimal initial header (version, title and optional case); do not silently rewrite existing metadata.
- Browser interpretation is independent from editor persistence. Unknown tags and block types should degrade to readable text rather than cause parsing or execution.

Example of minimal bytes (line endings LF):

```text
---
8tml: 1
title: Untitled
---
Hello, world.
```

## First vertical slice — acceptance gates

1. Select the maintained C64U editor target from real source, build scripts and tests; do not assume default `main` reflects later feature branches.
2. A newly created document saves to an explicitly chosen `.8tml` filename and includes a compliant header and ASCII/LF output.
3. Reopening, editing and resaving preserves the source, header keys, and unsupported markup byte-for-byte except explicit edits.
4. Automated tests cover empty body, header, newline conversion, printable ASCII, filename extension, failed I/O and near-capacity documents.
5. A produced file passes the provided Python `8tml` validator with zero errors; record validator version and exact invocation.
6. Host/VICE build and tests pass; C64U hardware round trip is a **separate** physical test gate.

## Subsequent vertical slice

Publish the exact file bytes via an authenticated SEE64YOU service to our self-hosted PHP site; render 8TML for modern browsers without modifying the source. Retrieve the original file with the C64U Browser and show supported 8TML text/links. Keep protocol/auth separate from 8TML parsing. Do not claim compatibility with T8WN services without interoperability testing.

The PHP service and native Publisher source exchange are implemented as described above: authenticated JSON `raw8tml` with separate service metadata, atomic snapshots and exact raw export. Existing HMAC verification signs the complete original request body; checked 32-bit request counts handle JSON expansion without increasing source capacity or weakening memory guards. No old-source conversion is added. Native Publisher hardware acceptance, full 8TML HTML rendering and Browser integration remain unfinished. See `docs/PROTOCOL.md` and `todo.md` rather than introducing another plan.

## Current risks and constraints

- The draft can change. Record the spec date in tests and review changes before final release.
- The supplied reference implementation's licence is unresolved.
- `main/client/src/main.c` currently displays `Save: storage layer next` on save. Confirmed `codex/browser-palette-fix` and `feature/c64u-reu-editor` instead call `save_document()` and `save_as_document()`; **inspect their storage implementations, tests and relative commit histories before selecting a source branch**. Do not copy entire branches indiscriminately.
- No tested C64U build, hardware I/O or end-to-end publishing is established by this migration planning commit.

## Verified working-branch code inventory (20 October 2026)

This section records **repository inspection**, not a completed conversion. On `codex/browser-palette-fix`:

| Function / file | Existing behaviour | Required change |
|---|---|---|
| `client/src/main.c` | `open_document()`, `save_document()` and `save_as_document()` call local storage | Keep command flow and prompts; route new documents through 8TML serialization |
| `client/src/storage/local.c` | Adds `.s64` where a filename has no extension; derives metadata from `.s64` | Default to `.8tml`, explicitly reject unintended extensions as appropriate, preserve overwrite/error behaviour |
| `client/src/document.c` | `s64_document_write_wire()` emits `SEE64YOU/1`, `Type`, `Slug`, `Title`, `Status`, `Date`, `Order` then body | Replace wire format with `---\n8tml: 1\n...
---\n` plus body; do not place site-specific publishing fields into 8TML without format support |
| `client/src/document.c` | Converts native strings/body into ASCII through `s64_charset_native_to_ascii()` | Reuse and test conversion; ensure LF output, reject unsupported characters safely |
| `client/Makefile` | Working branch builds independent Editor/Publisher/Settings and references `src/storage/local.c`, `src/document.c` | Reuse existing build/tests after branch ancestry and size constraints are checked |

**Do not patch the default branch's placeholder `main.c`**: it is not the intended starting point. Compare branch ancestry and inspect `document.h`, `storage/local.h`, any storage tests and the actual independent editor build before choosing which branch to migrate. No PRs were open during the preceding inspection, but refresh immediately before editing.

### Specific traps to avoid

- The 8TML header is recognized only when `---` is the first line and `8tml` is the first header key. The original `SEE64YOU/1` header is incompatible.
- Preserve unknown header keys and unsupported tags literally. A parser restricted to today's own generated fields is **not** a correct round-trip editor.
- A fresh `.8tml` file needs valid header/body boundaries even if the body is empty.
- The editor must reject or explicitly handle invalid ASCII before altering a destination file; preserve the existing safe-save/overwrite path.
- Do not silently port former post/page, status, slug or ordering metadata into invented 8TML header fields; maintain these at the publishing-service level if needed.
- Verify actual C64 disk filename/device behaviour with the longer `.8tml` suffix; do not assume PC filesystem semantics.
- The supplied spec identifies itself as an **internal draft**; interoperability is provisional until confirmed by the format maintainer.

### Minimal next execution checklist

1. Diff the maintained editor branch against `main` and the migration branch. Identify the smallest safe integration approach; preserve unrelated branch work.
2. Reproduce baseline save/open tests before edits.
3. Update the serializer and reader together, then extension handling, using a small self-contained set of C tests; include unknown-header round-trip and bad-file failures.
4. Run the independent Editor build and relevant host/VICE tests. Invoke the supplied external validator on emitted files.
5. Inspect diff, record byte-level outputs and failures, commit to a task branch only after tests; defer the hardware gate.
