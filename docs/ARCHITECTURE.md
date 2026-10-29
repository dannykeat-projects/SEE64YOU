# Architecture

SEE64YOU is C64 Ultimate-only and uses UCI, Ultimate-DOS and a required 16 MiB REU. There are three independently launched user programs: Editor (`SEE64YOU.PRG` via internal overlay assets), Publisher (`S64PUB.PRG`), and Settings (`S64CFG.PRG`). All return to BASIC. The active ten-file package contains no browser executable; reading uses an external compatible 8TML browser.

## Source and storage

`.8tml` is the only supported editing format. The 8TML 1 draft header and body are one literal ASCII/LF document, retained in the current 61,440-byte REU document region. The Editor preserves unknown header entries and markup; input with CR/CRLF is normalized to LF. Generated HTML is not saved back into source. No automatic disk autosave occurs.

The default build uses `/sd/SEE64YOU/`. A full rebuild with `STORAGE_ROOT` selects another mount, with no silent fallback. Editor local storage uses Ultimate-DOS `$02`; launcher/journal assets use `$01`. Existing files are protected against accidental collision, incomplete writes and failed renames as described in [runtime](C64U-RUNTIME.md).

## Publication

The independently launched Publisher loads literal source using the shared codec/storage path and sends the same ASCII/LF bytes through standard API 1 status/challenge/list/put operations over UCI Network `$03`. Streaming HMAC appends its signature after the source; local filenames and remote paths stay separate. Existing names require explicit confirmation, with no conditional-write guarantee in the test draft.

Settings owns a standard endpoint URL, key ID, dedicated secret and explicit raw-HTTP consent. A tagged standard destination uses the existing record size and readback checks. Legacy key-only destinations cannot authenticate to the new route. Editor requires neither credentials nor network access. Legacy CMS/relay/WordPress source remains pending caller tracing and replacement acceptance; it is excluded from the independent Publisher link.
## Boundaries and validation

Source format, local storage, publishing transport, authentication, HTML conversion and external browsing remain separate responsibilities. 8TML-capable browsing is provided by the wider ecosystem rather than another SEE64YOU client. Preserve bounded REU/conventional-RAM layouts, package manifests, clean BASIC return and restoration of machine settings. See [protocol](PROTOCOL.md), [validation](LOCAL_TESTING.md) and [product scope](PRODUCT.md).
