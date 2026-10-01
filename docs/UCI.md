# C64U / UCI integration

See64You is a C64 Ultimate-native application. UCI is a core platform service, not an optional compatibility enhancement.

Application modules use bounded platform interfaces; feature code should not manipulate UCI registers directly unless it is the platform layer responsible for that function.

## Hardware baseline

Real C64 Ultimate firmware 1.1 validation has established:

- the UCI window at $DF1C-$DF1F works on the tested device;
- Ultimate-DOS Target $01 is available;
- Network Target $03 is available;
- HTTP Target $06 is absent on that firmware;
- Network $03 resolved DNS, opened TCP, sent raw HTTP and received a valid response;
- settings persistence, direct authentication and large direct document transfer work on hardware.

Other register-window reports remain compatibility evidence, not a reason to abandon the hardware-proven mapping.

## Product requirement

The new product baseline requires:

- Command Interface enabled;
- 16 MiB REU;
- Ultimate-DOS;
- SD storage.

Network $03 is required for the current direct Standalone publisher, not for offline editing.

Software IEC/device 8 is **not** a production requirement.

## Capability probing

Probe actual targets/features instead of enabling code solely from firmware version.

A target is accepted only when identity/behaviour is convincing. Generic NO TARGET / 00,OK-style responses must not be mistaken for capability.

The startup capability inventory is specified in C64U-RUNTIME.md.

## Ultimate-DOS $01/$02

Ultimate-DOS provides two independent instances, $01 and $02, with separate directory/file state.

See64You should use this intentionally so user-document context and application-resource/module operations do not constantly destroy each other's path/file state.

Every subsystem must still establish its required path explicitly.

### Transactional storage

Document/configuration writes follow a transactional pattern:

1. remove stale temp;
2. create/write temp;
3. close;
4. remove/replace destination;
5. reopen and validate critical configuration before success.

Real firmware-1.1 testing established relevant details including explicit filename termination and file-not-found status handling. Those behaviours remain regression-covered.

### Direct file-to-REU loading

Ultimate-DOS command DOS_CMD_LOAD_REU ($21) reads bytes from the currently opened file directly into REU at a supplied address/length.

Official Ultimate documentation states that this function assumes a **16 MB REU configuration**.

This is the preferred foundation for See64You's module cache:

- open validated module;
- load it directly into allocated high REU;
- verify transferred length/status/integrity;
- transition from cached REU image later.

The corresponding SAVE_REU operation may also be useful for carefully scoped future features, but normal canonical document persistence should remain explicit/transactional rather than treating raw REU images as the document format.

## Network Target $03

client/src/platform/network.c provides bounded DNS/TCP open, write, read and close operations.

The firmware-1.1 Standalone publisher frames ordinary HTTP itself and authenticates protected operations with the challenge/HMAC protocol in PROTOCOL.md.

Direct mode:

- never sends the normal site password;
- does not use a reusable bearer token;
- requires explicit consent for plaintext http://;
- rejects https:// unless a real TLS-capable path is being used.

Authentication/integrity/replay protection do not provide confidentiality.

## Control Target $04

Control Target provides hardware-management functions and is part of the new platform evaluation.

Use only narrow, capability-proven functions. Potential See64You uses include:

- identify/capability information;
- deliberate recovery/reboot after an unrecoverable state;
- runtime palette capture/restore if See64You intentionally changes the runtime palette.

Do not use Control Target as permission to rewrite unrelated machine preferences.

## Software IEC Target $05

Software IEC is legacy/diagnostic information for See64You.

The old application chain depended on SD-backed software IEC/device 8 and exposed a real firmware/hardware stall. That dependency is retired from the product architecture.

Do not repair or preserve software-IEC chaining unless it is needed for an isolated regression/control test.

## HTTP Target $06

HTTP Target is optional and capability-detected.

It is documented on newer Ultimate firmware but is absent on the tested firmware-1.1 C64 Ultimate. Existing code may remain as a compatibility/future transport while it is useful.

Never infer TLS merely because a URL begins with https:// or because $06 exists; TLS/security behaviour must be verified.

## REU

See64You now requires a 16 MiB REU.

The REU contains shared application state, allocator-managed editor data and validated executable module cache. It is not merely a handoff buffer.

Capacity probing must preserve occupied data. Future tests should focus on the required 16 MiB configuration instead of treating 128 KiB as the product minimum.

REU is volatile working state. Canonical settings/documents that must survive power loss are persisted to SD.

## Turbo

Ultimate turbo registers can be software-controlled only when the user's Ultimate Turbo Control configuration exposes register control.

See64You therefore:

1. detects whether software control is available;
2. captures the prior runtime setting;
3. uses central INTERACTIVE / CPU_INTENSIVE / IO_SAFE / TRANSITION modes;
4. restores the captured value afterward.

Do not permanently force maximum turbo as a setup side effect.

CPU-intensive operations can use higher speed once measured. Timing-sensitive UCI/network/REU paths stay at a hardware-proven safe setting until faster operation is validated.

## Program/module transitions

The old production path uses separate Editor/Publisher/Settings PRGs and software-IEC chaining. It is migration code only.

Hardware evidence now includes:

- full Ultimate-DOS streaming of all current matching companion PRGs;
- successful companion-launch-probe replacement of the running PRG;
- successful entry into the loaded target's cc65 startup and return to the resident probe.

That completed the proof needed from the old $C800-$CBFF diagnostic design.

The committed production direction is now:

1. validate package manifest/module;
2. cache module in 16 MiB REU using Ultimate-DOS;
3. commit shared application state;
4. transition through a tiny overwrite-safe transfer routine;
5. enter the destination module;
6. validate the shared ABI/control block.

No permanent $C800-$CBFF reservation is assumed.

## Trusted-LAN relay

The relay remains an optional compatibility path for HTTPS-required origins and deferred WordPress use. It is not required by the direct Standalone firmware-1.1 path.

## Test infrastructure

Host/VICE/simulation can verify logic but not establish firmware behaviour.

Preferred development lanes:

- JC-000/c64-test-harness for reusable VICE/real-Ultimate smoke tests;
- barryw/sim6502 / u64sim for deterministic UCI/Ultimate-DOS state-machine tests;
- automated package upload/run helpers;
- real C64U as the final timing/firmware/input authority.

## Primary references

- https://1541u-documentation.readthedocs.io/en/latest/uci/core_uci_architecture.html
- https://1541u-documentation.readthedocs.io/en/master/uci/ultimate_dos_target.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/network_target.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/control_target.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/http_target.html
- https://1541u-documentation.readthedocs.io/en/latest/config/turbo_mode.html
- https://github.com/bdgscotland/c64-kb
