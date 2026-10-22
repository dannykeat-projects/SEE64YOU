# Architecture

SEE64YOU is C64 Ultimate-only and uses UCI, Ultimate-DOS and a required 16 MiB REU. There are three independently launched user programs: Editor (`SEE64YOU.PRG` via internal overlay assets), Publisher (`S64PUB.PRG`), and Settings (`S64CFG.PRG`). All return to BASIC. The active package contains no browser; a separate compatible 8TML/T8WN browser is used to read published source.

## Source and storage

`.8tml` is the only supported editing format. The 8TML 1 draft header and body are one literal ASCII/LF document, retained in the current 61,440-byte REU document region. The Editor preserves unknown header entries and markup; input with CR/CRLF is normalized to LF. Generated HTML is not saved back into source. No automatic disk autosave occurs.

The default build uses `/sd/SEE64YOU/`. A full rebuild with `STORAGE_ROOT` selects another mount, with no silent fallback. Editor local storage uses Ultimate-DOS `$02`; launcher/journal assets use `$01`. Existing files are protected against accidental collision, incomplete writes and failed renames as described in [runtime](C64U-RUNTIME.md).

## Publication

The separately launched Publisher loads source using the shared codec/storage path. Publishing metadata (page/post, slug, publication state and optional ordering) is separate from the 8TML source. Authenticated requests stream JSON to the standalone PHP service over UCI Network `$03`. The server stores exact source and metadata together in a guarded JSON/PHP file and derives display content; its present HTML renderer supports a limited escaped subset of 8TML, not full conformance.

Settings holds site configuration and a per-site direct key. HTTPS pairing happens in a modern browser; C64U direct HTTP uses challenge/HMAC, protecting request integrity/authentication but **not confidentiality**. Neither editing nor source ownership requires a publishing account or remote service.

## Boundaries and validation

Source format, local storage, publishing transport, authentication, HTML conversion and external browsing remain separate responsibilities. 8TML-capable browsing is provided by the wider ecosystem rather than another SEE64YOU client. Preserve bounded REU/conventional-RAM layouts, package manifests, clean BASIC return and restoration of machine settings. See [protocol](PROTOCOL.md), [validation](LOCAL_TESTING.md) and [product scope](PRODUCT.md).
