# External C64U engineering references

This document records external projects/specifications, licence boundaries and the evidence they provide.

See64You is now C64 Ultimate-only with a required 16 MiB REU. References are evaluated against that product direction rather than original-C64 compatibility.

Unresolved implementation belongs in todo.md.

## Decision rules

- Prefer official Ultimate documentation for intended UCI/Ultimate behaviour.
- Verify important firmware/timing/input behaviour on real C64U hardware.
- Treat third-party projects as evidence/tooling unless deliberately adopted.
- Do not copy GPL client/library code into the shipping client unless the project deliberately changes licensing.
- Prefer external host/test tooling over adding unnecessary resident client complexity.
- Any imported code must be licence-compatible, bounded, measured and tested.
- Capability claims should describe the tested hardware/firmware when behaviour may vary.

## Primary platform references

Authoritative platform sources:

- GideonZ/1541u-documentation
- Ultimate-DOS UCI target documentation
- Network Target documentation
- Control Target documentation
- Turbo mode/register documentation
- HTTP Target documentation
- bdgscotland/c64-kb for C64/6510/VIC/CIA/KERNAL details

Key architectural facts now used by See64You:

- Ultimate-DOS provides independent targets $01 and $02.
- DOS_CMD_LOAD_REU ($21) can load an opened file directly into REU and explicitly assumes a 16 MiB REU configuration.
- the U64 can expose software-controlled turbo registers when configured for register control;
- Control Target $04 exposes hardware-management/runtime functions;
- Network Target $03 provides the firmware-1.1 direct TCP path;
- HTTP Target $06 is not available on the tested firmware-1.1 machine and is therefore optional capability-detected functionality.

## Established real-hardware evidence

On the tested C64 Ultimate firmware 1.1:

- UCI $DF1C-$DF1F works;
- DOS $01 works;
- Network $03 works for DNS/TCP/raw HTTP;
- HTTP $06 is absent;
- transactional settings save/readback works;
- configuration survives a full physical power cycle;
- persisted direct authentication succeeds;
- full current companion PRGs can be streamed through Ultimate-DOS;
- the resident replacement-loader diagnostic successfully overwrote the running program, entered cc65 startup, ran the target and returned.

The last two points are evidence for executable overlay work, not a mandate to preserve the old $C800-$CBFF production design.

## REU/module-cache architecture

The new product requires 16 MiB and should use native DOS→REU loading for module preload where practical.

External references should now help answer:

- allocator/ownership design;
- fast verified REU transfers;
- tiny overwrite-safe executable replacement;
- state restoration;
- package/manifest integrity;
- hardware automation.

JC-000/c64-lib-contract is useful as a pattern for machine-readable ownership and map-based contracts.

## System-state/turbo references

Use official turbo/control documentation first.

See64You should capture and restore temporary runtime settings rather than permanently forcing performance settings.

Third-party demos such as xahmol/UltimateDemo2026 may provide implementation evidence, but GPL code remains reference-only unless licensing changes deliberately.

## Ultimate-DOS/UCI testing

Useful projects:

- barryw/sim6502 / u64sim — deterministic UCI/Ultimate-DOS external CI, GPL-3.0 process/tool boundary;
- xlar54/ultimateii-dos-lib — mature cc65 patterns, GPL-3.0 reference only;
- sandlbn/ultimate64-manager — MIT host-side REST/FTP/memory reference.

Real hardware remains the final gate.

## Hardware automation

Preferred reusable lane:

build → validate → upload/run → inject bounded input → inspect screen/memory/diagnostics → record result

JC-000/c64-test-harness remains the preferred evaluation because it spans VICE and real Ultimate hardware and documents firmware-1.1 REST /Temp hygiene.

Manual SD copying should become the exception during development.

## Editor/input references

Keep deterministic host/VICE tests, but C64U USB/native keyboard behaviour must be established on hardware.

Useful references:

- JC-000/c64-test-harness;
- drericflores/AdveditC64 (MIT);
- nschneir/Project64;
- barryw/vice-mcp.

Editor references may teach compact algorithms without imposing original-C64 memory ceilings on the product.

## UCI register-window compatibility

The tested firmware-1.1 C64U works at $DF1C-$DF1F.

Other projects report $DE1C-$DE1F in some Ultimate configurations. Treat that as a compatibility investigation requiring safe probing/identity validation.

Do not change the known-working path solely because third-party code uses another window.

## HTTPS/crypto research

JC-000/c64-https demonstrates TLS work over Ultimate networking and includes MIT-licensed optimized 6502 material. It remains research rather than the active production TLS path until its security, integration, resource and maintenance properties are deliberately accepted.

Current direct Standalone publishing remains challenge + HMAC-SHA256 over Network $03 with explicit plaintext-HTTP consent.

The trusted-LAN relay remains the practical HTTPS compatibility path.

## Adoption priorities

Evaluate next:

1. JC-000/c64-test-harness for repeatable real-hardware cycles.
2. barryw/sim6502 / u64sim for external UCI/DOS CI.
3. Native DOS_CMD_LOAD_REU module-cache implementation.
4. A minimal overwrite-safe cached-module transition.
5. C64U system-state/turbo capture and restoration.
6. Safe alternate UCI-window probing.

Reference-only unless deliberately adopted:

- xlar54/ultimateii-dos-lib — GPL-3.0.
- c64uploader/c64uploader — mixed host/native licensing.
- drericflores/AdveditC64 — MIT.
- sandlbn/ultimate64-manager — MIT.
- nschneir/Project64 / barryw/vice-mcp — tooling patterns.
- xahmol/UltimateDemo2026 — GPL-3.0 implementation evidence.

Research:

- JC-000/c64-https and related crypto work.
- Oscar64 only as a measured future compiler experiment, not an assumed migration.

## External-adoption gate

An external project is not adopted until:

1. licence/boundary is recorded;
2. its role is narrow and explicit;
3. setup is reproducible;
4. a See64You-specific test proves value;
5. existing validation still passes;
6. hardware-dependent behaviour is verified appropriately;
7. rollback/removal is straightforward.

## Source list

- https://github.com/GideonZ/1541u-documentation
- https://1541u-documentation.readthedocs.io/en/master/uci/ultimate_dos_target.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/network_target.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/control_target.html
- https://1541u-documentation.readthedocs.io/en/latest/config/turbo_mode.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/http_target.html
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
