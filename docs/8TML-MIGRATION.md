# 8TML adoption — implementation contract

**Decision:** SEE64YOU uses the external 8TML 1 draft (2026-10-20) as its only new source document format. No `.s64` migration path. This is a bounded development plan, not evidence of implemented compatibility.

## Source and legal boundary

The user supplied `specs.zip` containing `spec/8tml-1.md`, `spec/examples/*.8tml`, optional `.8ss` files and `tools/8tml` with Python parsing/rendering tooling. Use these as normative development references. Do **not** copy the reference code or extensive examples into the public repository without licence clarification. Independently written, minimal test fixtures and byte-level tests are acceptable; verify against the external validator in the local test environment.

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

## Current risks and constraints

- The draft can change. Record the spec date in tests and review changes before final release.
- The supplied reference implementation's licence is unresolved.
- `main/client/src/main.c` currently displays `Save: storage layer next` on save. Confirmed `codex/browser-palette-fix` and `feature/c64u-reu-editor` instead call `save_document()` and `save_as_document()`; **inspect their storage implementations, tests and relative commit histories before selecting a source branch**. Do not copy entire branches indiscriminately.
- No tested C64U build, hardware I/O or end-to-end publishing is established by this migration planning commit.
