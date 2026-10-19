# SEE64YOU v0.1.0-alpha.1 — C64 Ultimate installation

**Early alpha: this may not work on your setup.** Bugs and incomplete hardware
testing may cause failures or data loss. Back up your SD files, configuration
and documents, and use a disposable test installation. Native Editor
reopen/edit/update, physical-keyboard usability and cold-power observations
remain incomplete. This is experimental software, not a dependable public beta.

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
PHP website is a separate installation ZIP requiring PHP 8.1+ and HTTPS for
pairing. Follow the included first-page checklist or the
[versioned first-page guide](https://github.com/dannykeat-projects/SEE64YOU/blob/v0.1.0-alpha.1/docs/FIRST-PAGE.md).
The public C64 browser is for compatible SEE64YOU sites, not arbitrary HTML.
Direct HTTP publishing authenticates requests with HMAC but does not encrypt
document text. Never use sensitive test content over this connection.

Local package/VICE and bounded real-C64U save/publish/download checks passed;
complete first-user acceptance remains open. The early alpha release is
explicitly authorised despite those gaps. Current evidence is in the
[versioned work queue](https://github.com/dannykeat-projects/SEE64YOU/blob/v0.1.0-alpha.1/todo.md).
