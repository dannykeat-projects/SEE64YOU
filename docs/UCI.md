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

## References

- https://1541u-documentation.readthedocs.io/en/latest/uci/core_uci_architecture.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/network_target.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/http_target.html
- https://1541u-documentation.readthedocs.io/en/latest/howto/release_3.15.html
