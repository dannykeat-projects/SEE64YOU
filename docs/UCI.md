# C64 Ultimate Command Interface (UCI)

SEE64YOU uses Ultimate-native facilities rather than software-IEC drive chaining. Capability-probe the target and verify responses; firmware labels alone are insufficient evidence.

On the tested firmware 1.1 device, UCI at `$DF1C..$DF1F`, Ultimate-DOS `$01`/`$02` and Network `$03` worked. HTTP Target `$06` was absent. Offline editing needs UCI/DOS/REU but not network availability.

## Disk and REU

Ultimate-DOS `$01` and `$02` have independent directory/file contexts. Document operations and bootstrap/journals own separate contexts; every subsystem must explicitly establish its path and close files. Check statuses and readbacks, including failed creates/writes, file-not-found and rename failure. Files, configuration and snapshots are stored on the selected root (default SD; USB is an explicit whole-package build option).

A 16 MiB REU is required and holds current source/history and runtime working buffers. Do not assume that the full REU is available to a single document; the current source cap remains 61,440 bytes. Preserve buffer boundaries and DMA bank isolation.

## Network and security

Network `$03` supports direct DNS/TCP/raw HTTP in the verified configuration, not general TLS. It is optional for Editor and required for current Publisher direct transport. Require a site-specific key and challenge/HMAC; verify the full method/URI/body and consume challenges. This authenticates/integrity-checks but does not encrypt content. Reject unsupported URL schemes, unsafe ports and oversized input before opening connections. Never silently treat `https://` as working HTTPS.

## Machine control

Capture and restore any temporary live turbo, interrupt, screen-colour or other changed state. Avoid altering user Ultimate preferences such as SID/audio, HDMI, home directory, drive setup, or DHCP. Networking and file I/O at high turbo require real-hardware testing. Existing JavaScript-less website and physical hardware configuration remain independent.

Tests using host mocks, VICE or REST-assisted C64U operation cannot replace a physical-keyboard and cold-start pass. See [runtime](C64U-RUNTIME.md) and [test procedures](LOCAL_TESTING.md).

Primary platform references: [UCI architecture](https://1541u-documentation.readthedocs.io/en/latest/uci/core_uci_architecture.html), [Ultimate-DOS](https://1541u-documentation.readthedocs.io/en/master/uci/ultimate_dos_target.html), [Network](https://1541u-documentation.readthedocs.io/en/latest/uci/network_target.html).
