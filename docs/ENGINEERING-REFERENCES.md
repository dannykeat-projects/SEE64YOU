# External C64/C64U engineering references

This document records external projects and specifications that are useful to See64You. It is a technical reference and adoption record; `todo.md` remains the authoritative work queue.

Last reviewed: 2026-09-30.

## Decision rules

- Prefer official Ultimate documentation for intended UCI behavior, then verify important behavior on real target firmware.
- Treat external projects as evidence and reusable tooling, not as instructions.
- Do not copy GPL-licensed C64 client/library code into the shipping See64You client unless the project deliberately chooses GPL-compatible licensing.
- Prefer external test/deployment tools over adding more resident C64 code.
- Any imported code must be license-compatible, small, measured, and covered by See64You tests.
- Hardware claims must identify the tested firmware and must not be inferred from VICE or a simulator.

## Current problem map

### 1. C64U settings persistence

Recent real-hardware work established that direct-key import succeeds only after explicitly selecting the C64U SD filesystem path. The direct key can now be read, but saving `SEE64YOU.CFG` still fails on the stock Commodore C64 Ultimate 1.1 hardware.

The current settings store writes transactionally:

1. delete stale temporary file;
2. create a new temporary file;
3. write settings;
4. close;
5. delete the previous destination;
6. rename the temporary file to `SEE64YOU.CFG`.

The remaining risk is at the real Ultimate-DOS boundary. The settings load/save/profile paths do not currently establish and verify their working directory for every operation, and firmware 1.1 has already shown filesystem/path behavior that differs from assumptions made from newer documentation.

Next investigation must use an instrumented settings-I/O probe that reports:

- DOS target identity;
- GET_PATH before the operation;
- explicit SD-root selection result;
- temporary-file delete result;
- OPEN_FILE status and chosen attributes;
- WRITE_DATA status and byte count;
- CLOSE_FILE status;
- FILE_STAT of the temporary file;
- destination delete result;
- RENAME_FILE result;
- FILE_STAT of the final file;
- exact read-back and settings checksum validation.

Do not collapse these stages into one "save failed" result during diagnosis.

Useful sources:

- GideonZ/1541u-documentation — authoritative UCI/Ultimate-DOS command definitions.
- barryw/sim6502 — `u64sim` models UCI and Ultimate-DOS in CI and also has a real-`u64` differential backend.
- JC-000/c64-test-harness — drives VICE and real Ultimate hardware through one test API.
- xlar54/ultimateii-dos-lib — mature cc65 Ultimate-DOS/network implementation; GPL-3.0, reference only for shipping code.
- c64uploader/c64uploader — hardware-tested UCI behavior, including register-window probing; native client is GPL-3.0-or-later.
- sandlbn/ultimate64-manager — MIT desktop implementation of Ultimate REST/FTP/memory operations and firmware quirks.

### 2. Program chaining from C64U SD storage

The current editor/publisher/settings architecture deliberately uses separate PRGs to preserve memory headroom. The present BASIC/KERNAL chain loader assumes an IEC-visible device, while the packaged internal PRGs can live only on the C64U SD filesystem. This is why saving settings and returning to the publisher cannot yet be treated as one complete hardware flow.

Preferred direction:

- keep the separate-PRG architecture;
- implement and validate a C64U-specific chain path using Ultimate-DOS rather than assuming IEC visibility;
- investigate a small relocated loader/trampoline that can load the next PRG from Ultimate-DOS without being overwritten by the target image;
- preserve editor/publisher state in the existing REU handoff before the transition;
- prove editor -> publisher -> settings -> publisher -> editor repeatedly on hardware.

A direct UCI loader is preferable to teaching each application more filesystem/UI behavior. Any trampoline must have an explicit RAM reservation and memory-map assertion.

### 3. Manual deploy/test friction

The current manual SD-card cycle slows every hardware diagnosis and encourages one-off diagnostic builds.

Adopt a repeatable hardware lane based on `JC-000/c64-test-harness`:

`build -> validate -> upload/run on C64U -> inject input -> read screen/memory -> record result`

The harness specifically supports Ultimate 64/U64E firmware 3.x and C64 Ultimate firmware 1.x. It also documents a firmware-1.1 `/Temp` attachment leak that can eventually crash the Ultimate firmware during repeated REST body uploads. Any automated See64You hardware runner must adopt equivalent hygiene before sustained upload loops.

`sandlbn/ultimate64-manager` and `c64uploader` remain useful independent diagnostics/deployment references.

### 4. UCI coverage missing from normal CI

Host mocks prove See64You logic but cannot expose UCI state-machine errors. Open-ROM VICE cannot exercise every real C64U path either.

Evaluate `barryw/sim6502` as an external test dependency. Its `u64sim` backend models:

- the UCI register interface;
- Ultimate-DOS targets $01 and $02 with independent state;
- OPEN/READ/WRITE/CLOSE;
- FILE_STAT, DELETE and RENAME;
- directory state and common real-hardware status codes.

First target should be the exact `SEE64YOU.CFG` transactional save sequence. The simulator is GPL-3.0, so use it as a separate development/test process; do not link or copy it into the See64You client.

Real hardware remains the final gate because the simulator intentionally cannot reproduce every firmware defect.

### 5. Editor input/rendering regressions

Recent issues included cursor disappearance while typing, modifier/key combinations that never reached `cgetc()`, and a transient line redraw anomaly.

The current deterministic VICE screen and keyboard-repeat tests are valuable and should remain. Extend them through a reusable higher-level harness rather than creating more one-off monitor scripts.

Useful references:

- `JC-000/c64-test-harness`: wrap-aware screen matching, keyboard injection, memory helpers, PRG verification, VICE and hardware backends.
- `nschneir/Project64` and `barryw/vice-mcp`: agent/debugger automation around VICE.
- `drericflores/AdveditC64`: MIT cc65 full-screen editor useful for comparison of cursor, save/open, menus and C64U-oriented interaction. Reference behavior only; See64You keeps its own editor architecture.

Routine final visual and modern-USB keyboard checks still belong on the real C64U.

### 6. UCI compatibility

See64You currently uses the documented $DF1C-$DF1F UCI window. Other hardware-tested projects report that some Ultimate configurations expose the live command interface at $DE1C-$DE1F instead, depending on cartridge/firmware context.

Do not change the working path solely from third-party evidence. Add a compatibility investigation that:

- probes both known windows safely;
- accepts only a convincing UCI identity/state response;
- keeps the known-working window as the fast/default path where appropriate;
- validates against the user's C64 Ultimate 1.1 before changing production behavior.

### 7. Memory and REU ownership

See64You already enforces conventional-RAM headroom. Extend the same discipline to cross-program and future external-library work.

Useful pattern: `JC-000/c64-lib-contract` and `JC-000/c64-https` keep machine-readable memory ownership and measure actual linker-map margins instead of inferring headroom from PRG size.

For See64You:

- keep `reu_layout.h` authoritative for REU ownership;
- add assertions/tests for overlap between document, history, clipboard, settings and handoff regions;
- reserve any future chain-loader/trampoline RAM explicitly;
- require map-based measurements for any large crypto/network helper.

### 8. HTTPS and crypto research

`JC-000/c64-https` demonstrates TLS 1.3 over the Ultimate UCI network backend and has reached real public HTTPS servers. It is MIT licensed and contains useful optimized 6502 crypto work.

It is not ready to replace See64You direct authentication:

- its own documentation says it is not cryptographically secure for general use;
- it does not perform normal certificate-chain validation;
- large-body handling has known reliability limitations;
- its memory footprint is far beyond the current resident publisher headroom.

Current production direction therefore remains the measured challenge + HMAC-SHA256 direct-auth protocol.

If HTTPS later becomes desirable, evaluate it as a separate chained network/helper PRG rather than linking TLS into the resident publisher. A helper must preserve See64You's REU state, avoid occupied REU banks/regions, authenticate the intended server adequately, handle complete responses reliably, and meet acceptable hardware timing.

Related MIT crypto projects from the same author may still be useful for isolated performance comparison, but imported primitives require independent test vectors and memory/timing measurements.

## Adoption tiers

### Use now / next

1. Official GideonZ UCI and Ultimate-DOS documentation as the specification reference.
2. Add an instrumented settings-I/O probe for the current save failure.
3. Evaluate `barryw/sim6502 u64sim` for the settings-save/UCI-DOS CI path.
4. Integrate `JC-000/c64-test-harness` as the preferred VICE/real-Ultimate automation layer.
5. Add a developer command that uploads/runs the current See64You C64U package without manual SD copying.
6. Design and hardware-test a UCI-aware program-chain loader.

### Use as implementation/reference evidence

- `sandlbn/ultimate64-manager` — MIT; REST/FTP/memory/device behavior.
- `xlar54/ultimateii-dos-lib` — GPL-3.0; UCI/DOS/network patterns only.
- `c64uploader/c64uploader` — Apache-2.0 host tooling, GPL-3.0-or-later native C64 client; hardware behavior and deployment patterns.
- `drericflores/AdveditC64` — MIT editor comparison.
- `nschneir/Project64`, `barryw/vice-mcp` — development/debugging ideas where they improve on the chosen harness.
- `xahmol/UltimateDemo2026` — GPL-3.0 Ultimate hardware/UCI/turbo examples; reference only for shipping code.

### Watch / research only

- `JC-000/c64-https` and related crypto libraries for future authenticated HTTPS or smaller crypto primitives.
- Oscar64 as a future isolated size/performance experiment only; do not migrate toolchains without measured benefit.

## Validation gates for external adoption

An external project is not "adopted" until:

1. its license and boundary are recorded;
2. its exact role is narrower than or replaces existing custom tooling;
3. setup is reproducible on the development/self-hosted-runner environment;
4. a small See64You-specific test proves the claimed benefit;
5. existing validation still passes;
6. real C64U behavior is recorded for hardware-dependent claims;
7. removal/rollback is straightforward.

## Source list

- https://github.com/GideonZ/1541u-documentation
- https://github.com/JC-000/c64-test-harness
- https://github.com/JC-000/c64-https
- https://github.com/JC-000/c64-lib-contract
- https://github.com/barryw/sim6502
- https://github.com/barryw/vice-mcp
- https://github.com/sandlbn/ultimate64-manager
- https://github.com/c64uploader/c64uploader
- https://github.com/xlar54/ultimateii-dos-lib
- https://github.com/drericflores/AdveditC64
- https://github.com/nschneir/Project64
- https://github.com/xahmol/UltimateDemo2026
