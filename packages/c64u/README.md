# Compiled C64 Ultimate candidate

These eleven files are the independent client built from source commit
`c67f19f8504f959ee398b8479ac328900caf987d` using cc65/cl65
(V2.18, Ubuntu package 2.19). Build command: `make -C client independent-package`.
`BUILD.json` records provenance and per-file hashes; `SHA256SUMS` verifies the
files from this directory with `sha256sum -c SHA256SUMS`.

Copy **all eleven files** from `SEE64YOU/` together into `/sd/SEE64YOU/` on a
C64 Ultimate with a 16 MiB REU. Back up existing files first. Keep the private
assets beside the entry programs; do not launch `S64EDIT.PRG` directly.

- `SEE64YOU.PRG`: Editor.
- `S64PUB.PRG`: Publisher.
- `S64CFG.PRG`: Settings.
- `S64WEB.PRG`: optional read-only public Browser.

Each entry program launches independently and returns to BASIC. The standalone
PHP website is separate. Follow [First page](../../docs/FIRST-PAGE.md) and
[hardware acceptance](../../docs/CODEX-MVP-HARDWARE-ACCEPTANCE.md) from the
repository root.

This is a **provisional candidate**, not an approved public beta or GitHub
Release. Local package/VICE checks passed; first-user acceptance for this exact
build remains incomplete. Hardware evidence and remaining gates are recorded in
[the authoritative queue](../../todo.md).
