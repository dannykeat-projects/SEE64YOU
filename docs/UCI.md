# C64U / UCI integration

See64You treats C64U networking as a platform capability, not an editor dependency.

## Current implementation

`client/src/platform/uci.c` performs a bounded startup probe of the published Ultimate Command Interface:

- UCI register block at `$DF1C-$DF1F`;
- identification value check on `$DF1D`;
- HTTP Target `$06`;
- target `IDENTIFY` command `$01`.

If the interface or HTTP target is absent, See64You remains an offline editor. VICE is expected to report the target as unavailable unless a future emulator/simulation layer provides UCI.

The editor status bar shows:

- `HTTP` when the HTTP target identifies successfully;
- `OFF` otherwise.

This is capability detection only. It is not yet a claim that publishing has been validated on physical C64U hardware.

## Why HTTP Target is preferred

Firmware 3.15 introduced the HTTP Target, allowing the Ultimate to own HTTP request/response framing rather than forcing the 6502 to implement a complete HTTP stack. It supports reusable request headers and body objects, URL-encoded/JSON/binary bodies, and raw/object exchanges.

See64You should use this target when present because the application needs ordinary web publishing rather than game-specific persistent sockets.

## Planned layering

```
editor
  |
publisher
  |
See64You protocol
  |
HTTP transport
  |
UCI HTTP Target
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
4. Exercise a harmless HTTP status request.
5. Exercise authenticated login.
6. Save and retrieve a draft.
7. Publish/update/delete a test document.
8. Repeat against WordPress.

## References

- https://1541u-documentation.readthedocs.io/en/latest/uci/core_uci_architecture.html
- https://1541u-documentation.readthedocs.io/en/latest/uci/http_target.html
- https://1541u-documentation.readthedocs.io/en/latest/howto/release_3.15.html
