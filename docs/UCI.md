# C64U / UCI integration

See64You treats C64U networking as a platform capability, not an editor dependency.

## Current implementation

`client/src/platform/uci.c` performs a bounded startup probe of the published Ultimate Command Interface:

- UCI register block at `$DF1C-$DF1F`;
- identification value check on `$DF1D`;
- Ultimate-DOS Target `$01`;
- Network Target `$03`;
- HTTP Target `$06`;
- target `IDENTIFY` command `$01`.

If a networking target is absent, See64You remains an offline editor. The Network Target is especially important for stock Commodore C64 Ultimate firmware: it can open named TCP connections and exchange raw bytes without requiring the newer HTTP Target. VICE is expected to report these targets as unavailable unless a future emulator/simulation layer provides UCI.

The editor status bar shows:

- `HTTP` when the HTTP target identifies successfully;
- `OFF` otherwise.

This is capability detection only. It is not yet a claim that publishing has been validated on physical C64U hardware.

## Direct networking goal

The preferred Standalone architecture is a direct connection from the C64U to the See64You site with no PC or relay machine required.

The older Network Target `$03` is the compatibility foundation for that path. It exposes DNS-backed TCP open, read, write and close operations and is available on firmware lines that predate HTTP Target `$06`. See64You now contains a bounded `platform/network.c` transport plus a downloadable `network-probe.prg` that validates this behavior on real hardware before the publishing protocol is moved onto it.

HTTP Target `$06`, introduced in firmware 3.15, remains useful when present because it offloads HTTP framing. It is not required for the direct Standalone goal.

Neither target currently provides a verified TLS session to a C64 application. Direct internet publishing therefore must not send the site password, bearer token or unpublished source in plaintext. The next layer is a narrow authenticated and encrypted Standalone protocol carried over raw HTTP/TCP to a server endpoint we control. Cryptographic primitives will be selected only after the real-hardware `$03` probe establishes the available transport and the publisher memory/performance budget is measured.

The existing trusted-LAN relay remains a compatibility/fallback path, particularly for WordPress and arbitrary HTTPS destinations, rather than the intended default Standalone setup.

### Direct Standalone security

The live hardware/site probe established that the stock C64 Ultimate 1.1 can connect through Network Target `$03` directly to `danielkeating.org:80` and receive the Standalone API's `HTTP/1.1 200 OK` response. The normal Standalone path therefore does not need a PC relay.

Because port 80 is plaintext, See64You does not send the publishing password or bearer token over that path. Direct C64U requests instead use a dedicated 256-bit per-site key provisioned through the HTTPS pairing page. Each protected request obtains a short-lived one-time server challenge and sends an HMAC-SHA256 signature covering the protocol version, challenge, HTTP method, exact request URI, and SHA-256 of the request body. The server consumes a challenge once a valid signature is accepted, preventing captured requests from being replayed.

The document body itself remains plaintext by design. Public publishing content generally does not benefit enough from confidentiality to justify a full encryption layer on the 6510, while authentication, integrity and replay protection prevent unauthorized publishing, modification and deletion. Encryption can remain an optional later extension for private drafts if there is a real use case.

### Stock C64 Ultimate 1.1 hardware result

Real hardware validation on 2026-09-29 established that stock Commodore C64 Ultimate firmware 1.1 exposes Ultimate-DOS `$01` and Network `$03` when Command Interface is enabled. The See64You `network-probe.prg` successfully resolved `example.com`, opened TCP port 80 through `$03`, wrote a raw HTTP request and received `HTTP/1.1 200 OK`.

HTTP Target `$06` is not present on this firmware. The command-interface dispatcher answers an unregistered target's IDENTIFY command with data `NO TARGET` and status `00,OK`; the original generic probe treated any non-empty successful IDENTIFY reply as a capability and therefore produced a false positive. A follow-up hardware inspector showed `$06 IDENTIFY -> NO TARGET / 00,OK` and `$06 FREE_ALL $10 -> 21,UNKNOWN COMMAND`. The probe now rejects the generic `NO TARGET` identity.

## Planned layering

```
editor
  |
publisher
  |
See64You secure direct protocol
  |
raw HTTP framing
  |
UCI Network Target $03

fallback / newer firmware:
publisher -> UCI HTTP Target $06
optional compatibility:
publisher -> trusted-LAN relay
```

Only the platform layer may touch UCI registers. Publisher/editor code should consume normal operations such as login, list, get, save, delete and settings.

## Text and binary command parts

C64 editor/UI strings are native PETSCII, while the Ultimate HTTP and DOS targets consume ordinary byte-oriented text. The UCI command-part API therefore marks text parts explicitly with `S64_UCI_PART_NATIVE_TEXT`. Those parts are preflighted and converted byte-by-byte to 7-bit ASCII while they are written to the hardware queue; binary command fields and payloads are transferred unchanged. Textual UCI status and HTTP string-query responses are converted back to the platform-native representation before higher layers inspect them.

This boundary is intentionally streaming. It avoids an additional 896-byte command staging buffer on the C64 and prevents PETSCII from leaking into URLs, HTTP headers, form values or Ultimate DOS filenames.

## Validation gates

Before calling C64U publishing complete:

1. Compile the UCI path with cc65.
2. Run under VICE and confirm missing UCI fails cleanly.
3. Test capability detection on target C64U firmware.
4. Run `network-probe.prg` and verify DNS + TCP + raw HTTP through Network Target `$03`.
5. Measure the secure direct-protocol implementation on the C64U before making it the Standalone default.
6. Exercise authenticated direct Standalone access without transmitting reusable credentials in plaintext.
7. Save and retrieve a draft.
8. Publish/update/delete a test document.
9. Keep the relay and HTTP Target paths as compatibility fallbacks.
10. Repeat WordPress validation later.

## External implementation evidence

In addition to the official Ultimate documentation, the following projects are useful for validating assumptions:

- `barryw/sim6502`: its `u64sim` backend models Ultimate-DOS target state, file open/read/write/close, directory state, stat/delete/rename and common status codes. It is suitable as an external CI tool, not as shipping code.
- `JC-000/c64-test-harness`: provides a common VICE/real-Ultimate test transport and explicitly supports C64 Ultimate firmware 1.x.
- `xlar54/ultimateii-dos-lib`: mature cc65 UCI/DOS/network implementation. GPL-3.0, so use as behavioral/reference evidence unless licensing intentionally changes.
- `c64uploader/c64uploader`: hardware-tested UCI implementation reports that some configurations expose the active UCI register window at $DE1C-$DE1F rather than $DF1C-$DF1F. Treat this as a compatibility case to investigate, not proof that the current known-working mapping is wrong.
- `sandlbn/ultimate64-manager`: MIT host-side Ultimate REST/FTP/memory implementation with useful real-firmware behavior notes.

The official Ultimate-DOS documentation says each DOS target owns independent directory/file state. See64You should therefore never assume a directory selected by another target or previous program is still active.

### Current settings-persistence incident

On stock C64 Ultimate firmware 1.1, direct-key import eventually worked after explicitly entering the SD filesystem path, but saving `SEE64YOU.CFG` still reports `SAVE FAILED - SETTINGS NOT WRITTEN`.

The next step is diagnostic rather than another guessed workaround. The settings-I/O probe must capture current path, open/create, write, close, stat, delete, rename and final read-back separately. Once the failing stage is known, the production settings store should make its path and file-state preconditions explicit and test that sequence under `u64sim` plus real hardware.

The current transactional temp-file design remains desirable because it avoids destroying the previous settings until replacement data has been written. It should only be simplified if target-hardware evidence shows that the firmware cannot reliably support the required delete/rename sequence.

### C64U program chaining

The existing short PRG aliases are valid CBM-DOS names, but the current KERNAL/IEC chain path cannot load programs that exist only on the Ultimate SD filesystem. The preferred future C64U path is an Ultimate-DOS-aware loader/trampoline, preserving the existing REU handoff and separate program memory budgets. This requires an explicit safe RAM reservation and real-hardware multi-transition testing.

For the broader source/licensing/adoption record, see `docs/ENGINEERING-REFERENCES.md`.


## References

- https://1541u-documentation.readthedocs.io/en/latest/uci/core_uci_architecture.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/network_target.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/http_target.html
- https://1541u-documentation.readthedocs.io/en/latest/howto/release_3.15.html
