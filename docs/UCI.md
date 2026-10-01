# C64U / UCI integration

See64You treats Ultimate features as platform capabilities. Editor and publisher code consume bounded platform interfaces rather than accessing UCI registers directly.

## Hardware baseline

Real C64 Ultimate firmware 1.1 validation established:

- the documented UCI window at `$DF1C-$DF1F` works on the tested device;
- Ultimate-DOS Target `$01` is available;
- Network Target `$03` is available;
- HTTP Target `$06` is absent;
- Network `$03` resolved `example.com`, opened TCP port 80, sent raw HTTP and received `HTTP/1.1 200 OK`.

Some external projects report `$DE1C-$DE1F` in other Ultimate configurations. That remains a compatibility investigation; production should not abandon the hardware-proven mapping without evidence.

## Capability probing

`client/src/platform/uci.c` probes the command interface and supported targets with bounded commands. A target is accepted only when its identity is convincing; the generic firmware reply `NO TARGET / 00,OK` is not a valid capability.

Missing network facilities do not prevent offline editing.

## Ultimate-DOS

Ultimate-DOS provides local C64U document and settings storage.

The transactional save path is:

1. remove stale temp;
2. create temp;
3. write;
4. close;
5. remove old destination;
6. rename temp to destination;
7. for publishing settings/profiles, reopen the exact final file from `/sd/`, validate its checksum/fields and compare the complete saved record before reporting success.

Real firmware-1.1 testing exposed two details now covered by regression tests:

- FILE_STAT filenames require explicit NUL termination;
- status 82 is the firmware's normal file-not-found result for the relevant checks.

Publishing-settings load/save/profile/key-import select `/sd/` inside the settings-store boundary before file I/O. Install all application PRGs from the same current package: an older settings helper can still search the cold `/` path even when the SD file itself is valid.

Saving prepares the record/checksum without updating REU. The existing firmware-compatible temp/write/close/delete/rename sequence remains; cleanup deletes and write-side CLOSE are best effort. Final persistent readback uses the same loader as cold startup, including STAT/open/read/close, checksum and field validation. A missing, unreadable, truncated, corrupt or different final record returns a verification error; Save + Return stays in Settings. Only verified settings are handed back through REU. Profiles use the same verification path.

Host coverage verifies path selection, failed writes/readback, exact comparisons and disk reload with REU cleared. Current automatic-path loading and the production save/readback algorithm with scratch filenames and an imported direct key have passed on real firmware 1.1.0. Full power-cycle acceptance of the updated application package remains in `todo.md`.

## Network Target `$03`

`client/src/platform/network.c` provides bounded DNS/TCP open, write, read and close operations.

The active Standalone publisher uses this target directly on firmware 1.1. It frames ordinary HTTP requests itself and authenticates protected operations with the challenge/HMAC protocol in `PROTOCOL.md`.

Direct mode intentionally:

- does not send the site's normal password;
- does not use a reusable bearer token;
- requires explicit consent for plaintext `http://`;
- rejects `https://` because raw Network `$03` is not TLS;
- restores C64U turbo before network/REU I/O.

The document body remains plaintext. Authentication/integrity/replay protection do not imply confidentiality.

## HTTP Target `$06`

A bounded HTTP-target implementation remains available behind the publisher interface for compatible/newer firmware. Firmware 1.1 on the tested C64 Ultimate returns `NO TARGET` for IDENTIFY and `21,UNKNOWN COMMAND` for target commands, so `$06` is not used by the active package on that device.

Do not infer TLS from a target accepting an `https://` resource. TLS support requires an actual verified TLS path.

## Trusted-LAN relay

The relay is a compatibility path for HTTPS-required origins and WordPress. The C64U speaks authenticated HTTP to a controlled LAN service; the relay forwards only approved requests to one fixed HTTPS origin with certificate verification.

It is not a general proxy and is not the default Standalone requirement.

## Program chaining

The C64U application intentionally uses separate editor/publisher/settings PRGs to preserve memory headroom and carries state through REU.

The hardware-proven chain currently uses the C64U SD-backed software IEC drive on device 8 with `/sd/` selected and no conflicting hardware drive 8. FAT files are:

- `S64EDIT.PRG`
- `S64PUB.PRG`
- `S64CFG.PRG`

Firmware-1.1 IEC names omit the `.PRG` suffix and use unshifted PETSCII. Editor -> publisher -> settings -> publisher -> editor has completed successfully on real hardware.

A future Ultimate-DOS-aware loader can remove the software-IEC configuration dependency while retaining the same program boundaries and REU handoff. That implementation task belongs in `todo.md`.

## REU and timing

Publisher/settings companions may start with live document/history data already present in REU. Capacity probing must therefore preserve every byte it touches. The current probe is covered by a VICE capacity/preservation matrix from 128 KiB through 16 MiB and by exact real-hardware document upload.

UCI/REU/network operations are timing-sensitive. Turbo acceleration is limited to measured CPU-only regions such as SHA/HMAC unless hardware evidence supports a wider scope.

## Test infrastructure

Host mocks and VICE cover logic but cannot establish firmware behaviour. Planned reusable external lanes are tracked in `todo.md`:

- `barryw/sim6502` / `u64sim` for deterministic UCI/Ultimate-DOS state-machine tests;
- `JC-000/c64-test-harness` for common VICE/real-Ultimate smoke tests;
- a developer package deploy/run command for normal hardware cycles.

Real C64U remains the final gate for firmware, timing and modern USB keyboard claims.

## References

- https://1541u-documentation.readthedocs.io/en/latest/uci/core_uci_architecture.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/network_target.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/http_target.html
- https://github.com/bdgscotland/c64-kb
- https://github.com/barryw/sim6502
- https://github.com/JC-000/c64-test-harness
