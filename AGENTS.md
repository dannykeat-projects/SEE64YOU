# SEE64YOU development instructions

## Product contract

SEE64YOU is a C64 Ultimate-only, **8TML source editor first**. The canonical document is a portable `.8tml` text file, editable offline, independent of the PHP service and any external browser. Optional Publisher and Settings launch as separate programs. The 8TML/T8WN ecosystem supplies the compatible reader; do not develop another SEE64YOU 8TML browser. Do not add alternate document formats, importers or compatibility modes.

Read [product scope](docs/PRODUCT.md), [architecture](docs/ARCHITECTURE.md), [runtime](docs/C64U-RUNTIME.md) and [todo](todo.md) before substantial code work. `todo.md` is the only live work queue. Documentation should explain what exists and how to use/test it, **not narrate implementation history**.

## Working rules

- Inspect current source, Git branch/head, open PRs, tests, build manifests and changed files before modifying code. Preserve unrelated work.
- Verify failures before fixing; make the smallest maintainable, tested changes. Reuse current source codec, storage, validation and authentication boundaries. Don't introduce new runtime handoff systems or gratuitous abstractions.
- Editor must remain useful offline with explicit Save/Save As and existing recovery; **no timed background autosave**.
- Current source contract: 8TML 1 draft; ASCII/LF, complete editable header/body, unknown-source preservation, 61,440-byte current limit; no generated HTML as source. Pin external validator/spec versions when available and do not claim full conformance without fixtures.
- Source, publishing service metadata, HTTP transport, key provisioning, HTML rendering and external browsing are separate responsibilities.
- Publisher simplification is a staged task. Do not remove commands or modules until live callers, update identity, manifests, package builds, affected tests and hardware behavior have been checked. Preserve safe remote confirmation and failure behavior.
- Keep code straightforward and concise; preserve user data and avoid broad rewrites, dormant integrations and unnecessary dependencies.

## Hardware and disk safety

- The product requires Command Interface, Ultimate-DOS and 16 MiB REU. Probe capabilities (UCI `$DF1C..$DF1F` on tested firmware, DOS `$01`/`$02`, Network `$03`) rather than assuming version implies availability. Direct HTTP is optional for offline Editor; there is no general TLS support.
- Storage is an explicitly selected build-time root, default `/sd/`. Force a complete rebuild for `STORAGE_ROOT` changes; install a fully matching package. Never silently fall back to another device or migrate user data.
- Preserve existing temporary-file, collision, configuration readback, recovery-snapshot and ACTIVE/CLEAN journal protections. Preserve user files; leave recoverable temporaries intact on failure.
- Every changed machine setting follows capture → durable ACTIVE → temporary apply → restore/verify → durable CLEAN. Do not change unrelated Ultimate settings. Restore responsive BASIC and original live turbo/IRQ/DOS contexts/colours on exit.
- Keep at least 1,024-byte measured Publisher/launcher headroom and editor resident/overlay fit; validate package manifests and exact program/assets dependencies. Do not alter launcher preload/manifest behavior without tracing callers and real-hardware acceptance.
- Use the smallest proven performance mode for I/O; accelerate compute selectively, not by forcing permanent maximum turbo.
- Preserve keyboard-first operation, 40x25 UI baseline and current REU-based buffers/history; feature additions require measured benefit and bounded resource use.

## Security

Direct raw HTTP publishing requires explicit user consent, a dedicated site key and challenge/HMAC over the method, URI and body; it is **not confidential**. Pair over HTTPS, never send ordinary website passwords in direct HTTP, never silently downgrade HTTPS. Bound host/port/path, request/response sizes, authentication lifetime/replay and file/slug operations. Keep public HTML safely escaped, drafts protected and writes recoverable.

## Validation and delivery

Run narrow reproduction/host tests, affected regressions, complete shipping SD/USB-root builds/fit/manifests/VICE where relevant, and bounded real-C64U tests for actual disk/network/input/runtime changes. Record each gate **passed / failed / blocked / unverified**, with commit, build inputs, hashes and exact restoration evidence. Host/VICE/REST injection does not prove physical keyboard, cold-power or C64U network behavior.

Do not force-push, rewrite history, reset/clean user changes, delete application data, change released binaries or deploy without explicit authorization. Commit only task-related files. For user-facing changes update the canonical [installation](docs/INSTALL.md), [Editor](docs/EDITOR.md), [protocol](docs/PROTOCOL.md) or [runtime](docs/C64U-RUNTIME.md) docs in the same change. Keep remaining work in [todo](todo.md); avoid duplicate roadmaps and retrospective logs.
