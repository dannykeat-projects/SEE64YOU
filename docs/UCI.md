# C64 Ultimate Command Interface (UCI)

SEE64YOU uses Ultimate-native facilities rather than software-IEC drive chaining. Capability-probe the target and verify responses; firmware labels alone are insufficient evidence.

On firmware 1.1.0, UCI at `$DF1C..$DF1F`, Ultimate-DOS `$01`/`$02` and Network `$03` were verified; HTTP Target `$06` was absent. Firmware 1.2.0 introduced `$06`, retained in 1.2.1. SEE64YOU's current standard Publisher still uses Network `$03`; 1.2.1 REST-assisted Editor benchmarks do not establish `$06` publishing acceptance. Offline editing needs UCI/DOS/REU but no network.

## Disk and REU

Ultimate-DOS `$01` and `$02` have independent directory/file contexts. Document operations and bootstrap/journals own separate contexts; every subsystem must explicitly establish its path and close files. Check statuses and readbacks, including failed creates/writes, file-not-found and rename failure. Files, configuration and snapshots are stored on the selected root (default SD; USB is an explicit whole-package build option).

A 16 MiB REU is required and holds current source/history and runtime working buffers. Do not assume that the full REU is available to a single document; the current source cap remains 61,440 bytes. Preserve buffer boundaries and DMA bank isolation.

## Network and security

Network `$03` supports direct DNS/TCP/raw HTTP in the verified configuration, not general TLS. It is optional for Editor and required for current Publisher direct transport. Require a site-specific key and challenge/HMAC; verify the full method/URI/body and consume challenges. This authenticates/integrity-checks but does not encrypt content. Reject unsupported URL schemes, unsafe ports and oversized input before opening connections. Never silently treat `https://` as working HTTPS.

## Firmware 1.2.1 assessment

The [official firmware changelog](https://downloads.commodore-international.com/documentation/changelog-1.2.1.txt) documents network mouse control, USB input fixes, REU transfer corrections and improvements to the UCI HTTP client. Existing [HTTP Target `$06` documentation](https://1541u-documentation.readthedocs.io/en/latest/uci/http_target.html) describes request/response offload, **not HTTPS/TLS**.

- **No production transport migration:** the standard Publisher already streams source and signs requests over `$03`. `client/src/platform/http.c` supports `$06` for retained/diagnostic paths; switching the standard Publisher would require revalidating request signing, bounded source streaming, server response parsing, cleanup, 1.1.0 compatibility, code/overlay fit and real C64U behaviour. Do so only if a targeted comparison demonstrates a worthwhile gain. Do not treat `$06` as a TLS workaround.
- **No editor feature change:** existing mouse and keyboard paths remain appropriate. On 1.2.1, test physical mouse mode/sensitivity, drag in both directions, middle edits and viewport scroll before attributing latency to the Editor; network-injected mouse events can supplement but never replace physical acceptance.
- **No storage redesign:** direct Ultimate-DOS `$01`/`$02` is not the software-IEC drive. The new SoftIEC `UCI Only`/`Disabled` distinction is not an Editor dependency; do not alter users' drive settings or drop application-level save/recovery checks. Keep the existing 1.2.1 hardware and release gates in [todo](../todo.md) and [testing](LOCAL_TESTING.md), rather than opening another firmware workstream.

## Machine control

Capture and restore any temporary live turbo, interrupt, screen-colour or other changed state. Avoid altering user Ultimate preferences such as SID/audio, HDMI, home directory, drive setup, or DHCP. Networking and file I/O at high turbo require real-hardware testing. Existing JavaScript-less website and physical hardware configuration remain independent.

Tests using host mocks, VICE or REST-assisted C64U operation cannot replace a physical-keyboard and cold-start pass. See [runtime](C64U-RUNTIME.md) and [test procedures](LOCAL_TESTING.md).

Primary platform references: [UCI architecture](https://1541u-documentation.readthedocs.io/en/latest/uci/core_uci_architecture.html), [Ultimate-DOS](https://1541u-documentation.readthedocs.io/en/master/uci/ultimate_dos_target.html), [Network](https://1541u-documentation.readthedocs.io/en/latest/uci/network_target.html).
