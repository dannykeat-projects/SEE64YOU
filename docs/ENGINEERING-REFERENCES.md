# External engineering references

External code and services are reference material only. Do not import dependencies, protocols or implementation techniques without checking licenses, integration cost and measured relevance. Official 8TML/T8WN documents and a pinned validator/revision are the authority for format interoperability when available; keep SEE64YOU source useful without online access.

Useful references include:
- [8TML / T8WN](https://t8wn.com/8tml/) — external source standard and compatible reader ecosystem; verify current specification independently.
- [8SS stylesheets](https://t8wn.com/8tml/reference/stylesheets/), [keys](https://t8wn.com/8tml/reference/stylesheets/keys/), [style words](https://t8wn.com/8tml/reference/stylesheets/style-words/) and [cascade](https://t8wn.com/8tml/reference/stylesheets/cascade/) — checked against the 8TML 1 draft reference on 18 November 2026; no immutable 8SS revision is published. The format is `key: value` per line without an 8TML header, with `#` comments and unknown keys ignored. It is not CSS. A page's `style:` header names a relative stylesheet; client defaults, host `/site.8ss`, page stylesheet and page header form the cascade. Local editing preserves literal source without claiming grammar validation.
- [Publishing operations](https://t8wn.com/8tml/reference/publishing-api/operations/) and [filename rules](https://t8wn.com/8tml/reference/publishing-api/file-names/) — the current reference includes an `.8ss` listing example, but endpoints may reject file types with `bad-type`. This does not establish authenticated server acceptance or reader interoperability. Current external documentation includes operations beyond the locally pinned 26 October 2026 draft; do not silently expand the client protocol.
- [Commodore Ultimate UCI architecture](https://1541u-documentation.readthedocs.io/en/latest/uci/core_uci_architecture.html), [Ultimate-DOS](https://1541u-documentation.readthedocs.io/en/master/uci/ultimate_dos_target.html), and [Network target](https://1541u-documentation.readthedocs.io/en/latest/uci/network_target.html) — platform capabilities.
- [cc65](https://cc65.github.io/) — C64 cross-development toolchain.
- [VICE](https://vice-emu.sourceforge.io/) — emulator validation; physical Ultimate hardware is still required for final integration and timing checks.
- [1541 Ultimate issue tracker](https://github.com/GideonZ/1541ultimate/issues) — reported firmware/USB/UCI behavior; never equate an issue report to proof of the installed firmware.

Licensing and third-party source use must be reviewed before copying code or bundling reference validators, ROMs or font assets. Use [runtime](C64U-RUNTIME.md) and [testing](LOCAL_TESTING.md) for the project's own evidence and acceptance gates.
