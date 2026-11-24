# Architecture

SEE64YOU is C64 Ultimate-only and uses UCI, Ultimate-DOS and a required 16 MiB REU. There are three independently launched user programs: Editor (`SEE64YOU.PRG` via internal overlay assets), Publisher (`S64PUB.PRG`), and Settings (`S64CFG.PRG`). All return to BASIC. The active ten-file package contains no browser executable; reading uses an external compatible 8TML browser.

## Source and storage

The Editor supports 8TML 1 draft source, literal 8SS source and general ASCII text using the same 61,440-byte REU document region and streaming codec. 8TML requires its version/header structure; 8SS and general text do not require headers and are not parsed as 8TML. Explicit `.md`, `.ini` and other safe text suffixes remain plain text; input with CR/CRLF is normalized to LF. Generated HTML is not saved back into source. No automatic disk autosave occurs. Source reads normalize and convert through the existing 64-byte codec workspace, appending validated chunks to an EOF gap. A failed append does not advance logical length; failed input is cleared when the reader finishes.

The default build uses `/sd/SEE64YOU/`. Four PRG files remain directly in that directory; its `SYSTEM/` subfolder contains the four verified Editor `.BIN` overlays and the two `.MNF` manifests. The launcher reads manifests/overlays from `SYSTEM/` but loads PRGs from the parent directory. The Editor places 8TML, 8SS and general text files in `docs/`. Format-specific `.REC` snapshots, configuration and journals stay at the package root. A full rebuild with `STORAGE_ROOT` selects another mount, with no silent fallback. Editor local storage uses Ultimate-DOS `$02`; launcher/journal assets use `$01`. Existing files are protected against accidental collision, incomplete writes and failed renames as described in [runtime](C64U-RUNTIME.md).

The independent Editor keeps typing, input, REU buffers/history and descriptor validation resident. Infrequent manifest/session preparation uses the existing PREP overlay and checked return bridges; shared state remains resident. Descriptor validation must remain callable while cache rebinding clears the overlay registry. The shipping build enforces at least 1,024 bytes of resident reserve below the overlay slot and excludes the unused outbound Publisher-transfer stub. No additional overlay or program handoff is introduced.

Public storage entry points use generated FILE overlay wrappers so callers can safely arrive from another overlay. Entry discovery scans raw source, which must keep shared braces outside preprocessor alternatives; an unbalanced scan fails the build. The overlay regression checks the storage header's complete public API against discovered definitions.

## Publication

The independently launched Publisher loads `.8tml`, `.8ss` and `.txt` source using its compact read-only codec/storage path and sends ASCII/LF bytes through standard API 1 status/challenge/list/put operations over UCI Network `$03`. The filename selects the shared codec's document kind: 8TML header validation remains required, while 8SS and text use literal headerless source. Streaming HMAC appends its signature after the source; local filenames and remote paths stay separate. Existing names require explicit confirmation, with no conditional-write guarantee in the test draft. Other Editor text suffixes remain local.

Settings owns a standard endpoint URL, key ID, dedicated secret and explicit raw-HTTP consent. A tagged standard destination uses the existing record size and readback checks. The Editor requires neither credentials nor network access. Publisher links only the standard publishing client, not a bundled website or browser.

## Boundaries and validation

Source format, local storage, publishing transport, authentication, HTML conversion and external browsing remain separate responsibilities. 8TML-capable browsing is provided by the wider ecosystem rather than another SEE64YOU client. Preserve bounded REU/conventional-RAM layouts, package manifests, clean BASIC return and restoration of machine settings. See [protocol](PROTOCOL.md), [validation](LOCAL_TESTING.md) and [product scope](PRODUCT.md).
