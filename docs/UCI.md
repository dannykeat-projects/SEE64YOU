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

Host coverage verifies path selection, failed writes/readback, exact comparisons and disk reload with REU cleared. Real firmware 1.1.0 has passed normal Save/readback and profile save/load. After a full physical power cycle, the 334-byte active CFG matched the saved record exactly; the current loader started from `/` with no valid REU settings cache, selected `/sd/` and passed record/checksum/UI validation. The persisted direct key then passed an authenticated GET (HTTP 200). Normal companion chaining still requires the separate acceptance gate in `todo.md`.

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

Production companion transitions still use the temporary software-IEC chain, but the Ultimate-DOS replacement now has two bounded layers. `client/src/platform/companion_loader.c` selects `/sd/`, opens the exact packaged `.PRG`, obtains its size, requires the `$0801` PRG load address, and rejects any image reaching `$C800`; it then consumes the two-byte header and leaves the file positioned at the executable payload. The existing 1 KiB headroom guard already keeps every application BSS below `$C800`, while the software stack begins at `$CC00`, so `$C800-$CBFF` is the explicit resident-loader reservation. `client/src/platform/dos_file.c` keeps the C64U-only `FILE_INFO`/`FILE_SEEK` helpers out of the conventional C64 editor link. The read-only `companion-stream-probe.prg` has passed all three full companion reads on real firmware 1.1. `companion-launch-probe.prg` is the next gate: an assembly loader in the reserved `$C800-$CBFF` block performs the replacement read, closes the file, enters the standard cc65 startup at `$080D`, and verifies return from a large matching diagnostic target. Production transition wiring remains outstanding in `todo.md`.

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
