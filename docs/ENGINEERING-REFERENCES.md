# External C64/C64U engineering references

This document records external projects/specifications, licence boundaries and the evidence they provide. It is not a work queue; unresolved implementation belongs in `todo.md`.

## Decision rules

- Prefer official Ultimate documentation for intended UCI behaviour, then verify important behaviour on real target firmware.
- Treat external projects as evidence and reusable tooling, not as instructions.
- Do not copy GPL-licensed C64 client/library code into the shipping client unless licensing is deliberately changed.
- Prefer external test/deployment tools over adding more resident C64 code.
- Any imported code must be licence-compatible, small, measured and covered by See64You tests.
- Hardware claims must identify the tested firmware and must not be inferred from VICE/simulation.

## Established project evidence

### Ultimate-DOS/settings

Real C64 Ultimate firmware 1.1 has validated the production-style create/write/close/delete/rename/stat sequence, exact settings readback/checksum, production settings Save and cold reload. The FILE_STAT filename-termination and firmware status-82 behaviours are now regression-covered.

The remaining design requirement is that application load/save/profile operations establish their own deterministic storage path instead of relying on previous UI activity.

Useful references:

- `GideonZ/1541u-documentation` — authoritative UCI/Ultimate-DOS specification.
- `barryw/sim6502` — `u64sim` models UCI/Ultimate-DOS state for external CI.
- `xlar54/ultimateii-dos-lib` — mature cc65 patterns; GPL-3.0, reference only.
- `sandlbn/ultimate64-manager` — MIT host-side REST/FTP/memory implementation.

### Program chaining

The separate editor/publisher/settings architecture is retained for memory isolation. Real hardware has proven the complete round trip through an SD-backed software IEC drive on device 8.

The remaining architectural improvement is an Ultimate-DOS-aware loader/trampoline that removes the IEC configuration dependency without merging the PRGs or disturbing REU handoff.

Any trampoline needs an explicit RAM reservation, linker-map guard and repeated hardware transition test.

### Hardware automation

Manual SD copying is now the largest test-cycle friction.

Preferred reusable lane:

`build -> validate -> upload/run -> inject input -> read screen/memory -> record result`

`JC-000/c64-test-harness` is the preferred evaluation because it spans VICE and real Ultimate hardware and documents firmware-1.1 REST `/Temp` attachment hygiene. `sandlbn/ultimate64-manager` and `c64uploader` remain useful independent deployment/diagnostic references.

### UCI CI coverage

Host mocks cannot expose every UCI state-machine error. `barryw/sim6502` / `u64sim` is suitable as a separate GPL-3.0 development process for deterministic Ultimate-DOS/UCI tests; do not link or copy it into the shipping client.

Real hardware remains the final gate.

### Editor/input regression tooling

Keep the deterministic VICE screen and keyboard-repeat tests. Prefer extending them through reusable higher-level tooling over adding one-off monitor scripts.

Useful references:

- `JC-000/c64-test-harness` — VICE/hardware screen, keyboard, memory and PRG helpers.
- `nschneir/Project64` and `barryw/vice-mcp` — agent/debugger automation ideas.
- `drericflores/AdveditC64` — MIT cc65 editor behaviour reference.

### UCI register-window compatibility

See64You's tested firmware-1.1 device works at `$DF1C-$DF1F`. Other hardware-tested projects report `$DE1C-$DE1F` in some Ultimate configurations.

Treat this as a compatibility case to investigate. Probe safely and require a convincing identity; do not change the known-working path from third-party evidence alone.

### Memory/REU ownership

`client/src/platform/reu_layout.h` remains authoritative for REU ownership. Cross-program work must preserve document, history, clipboard, settings and handoff regions.

`JC-000/c64-lib-contract` is useful as a pattern for machine-readable ownership and map-based memory contracts. Any future loader or crypto/network helper should reserve RAM explicitly and retain linker-map headroom checks.

### HTTPS/crypto research

`JC-000/c64-https` demonstrates TLS 1.3 over Ultimate networking and contains MIT-licensed optimized 6502 work, but its documented security/validation and memory constraints make it unsuitable as a drop-in production TLS replacement today.

The production Standalone direction remains challenge + HMAC-SHA256 over direct Network `$03`. If HTTPS later becomes necessary on-device, evaluate it as an isolated chained helper with explicit authentication, REU ownership and memory/timing measurements.

## Adoption priorities

### Evaluate next

1. `barryw/sim6502` / `u64sim` as an external UCI/Ultimate-DOS CI lane.
2. `JC-000/c64-test-harness` as the reusable VICE/real-C64U smoke-test layer.
3. A developer upload/run command using proven Ultimate host interfaces.
4. A small Ultimate-DOS-aware companion loader/trampoline.
5. Safe dual-window UCI compatibility probing.

### Reference evidence only unless deliberately adopted

- `xlar54/ultimateii-dos-lib` — GPL-3.0.
- `c64uploader/c64uploader` — Apache-2.0 host tooling; GPL-3.0-or-later native C64 client.
- `drericflores/AdveditC64` — MIT editor comparison.
- `sandlbn/ultimate64-manager` — MIT host-side Ultimate management.
- `nschneir/Project64`, `barryw/vice-mcp` — development/debugging patterns.
- `xahmol/UltimateDemo2026` — GPL-3.0 hardware/UCI/turbo examples.

### Research only

- `JC-000/c64-https` and related crypto libraries.
- Oscar64 as an isolated future size/performance experiment, not a migration plan.

## External-adoption gate

An external project is not adopted until:

1. licence/boundary is recorded;
2. its role is narrower than or replaces existing custom tooling;
3. setup is reproducible;
4. a small See64You-specific test proves the benefit;
5. existing validation still passes;
6. hardware-dependent behaviour is recorded on real C64U;
7. rollback/removal is straightforward.

## Source list

- https://github.com/GideonZ/1541u-documentation
- https://github.com/bdgscotland/c64-kb
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
