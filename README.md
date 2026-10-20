# SEE64YOU

**A C64 Ultimate text editor for portable 8TML publishing.**

**Direction adopted 20 October 2026:** 8TML is SEE64YOU's long-term content standard, regardless of whether T8WN or the VVV browser continue development. The editor will be useful offline; source files can be independently hosted without a T8WN account. Publishing and static HTML conversion will not depend on one provider.

**IMPORTANT: the current [v0.1.0-alpha.1](https://github.com/dannykeat-projects/SEE64YOU/releases/tag/v0.1.0-alpha.1) does *not* support 8TML.** It is an experimental, C64U-only **legacy `.s64` release**. Do not use it for important or private documents without backups. The complete, current [alpha installation and usage guide](docs/releases/v0.1.0-alpha.1.md) is the starting point for testers.

The [8TML roadmap](docs/8TML-ROADMAP.md) sets the architecture, independently hosted format and publishing requirements, compatibility rules, milestones, security and retirement gates. [todo.md](todo.md) is the sole active execution queue. The pre-pivot queue is [archived](docs/archive/TODO-before-8TML-pivot-2026-10-20.md).

## Current alpha: programs and prerequisites

The released C64U ZIP contains **eleven files**. Copy them all together to `/sd/SEE64YOU/`. Enable **Command Interface (UCI)**, **Ultimate-DOS** and a **16 MiB REU** in the C64 Ultimate settings. Firmware 1.1.0 is the tested baseline. Do not launch private `S64EDIT.PRG` on its own.

| Launch | Purpose in the current alpha |
| --- | --- |
| **`SEE64YOU.PRG`** | **Editor**: create, open and save `.s64` locally and offline (loads private editor assets) |
| `S64PUB.PRG` | Publisher: authenticated `.s64` publish/update, remote-source download to a new local file |
| `S64CFG.PRG` | Settings: legacy PHP website URL, dedicated key and persistence |
| `S64WEB.PRG` | Optional reader for legacy SEE64YOU websites; **not** a general HTML or 8TML reader |

Programs launch independently and return to BASIC. See the [step-by-step alpha guide](docs/releases/v0.1.0-alpha.1.md) for Editor keys, PHP host setup, pairing, publisher operation and known limits.

The alpha's separate PHP 8.1+ website package is **only** for legacy SEE64YOU publishing. Pair its dedicated key over HTTPS using a modern browser. C64U firmware-1.1 direct publishing requires explicit HTTP consent: challenge/HMAC provides authentication and integrity, **not confidentiality**. Never publish sensitive content by that path. A web host that redirects all HTTP to HTTPS won't support that direct transport.

## New direction (not implemented yet)

1. Pin a versioned authoritative 8TML profile and conformance fixtures; verify charset/escaping/stylesheets rather than guessing draft syntax.
2. Make the **offline Editor** create/open/save `.8tml` source, while preserving and safely importing existing `.s64`.
3. Demonstrate ordinary static hosting of source and faithful safe conversion to HTML for modern browsers; external VVV support is optional.
4. Add optional authenticated remote publishing through a provider-neutral, independently self-hostable API or an interoperable third-party endpoint.
5. Retire redundant legacy browser/site/markup code only when a tested migration and rollback exist.

The editor should remain usable even if upstream APIs, browsers and hosting disappear.

## Repository and development

```text
client/        cc65 C64U programs and host tests (legacy alpha today)
site/          current PHP 8.1+ flat-file legacy site/API (frozen features)
wordpress/     deferred legacy WordPress adapter
relay/         optional legacy transport relay (deferred)
docs/          current hardware contracts, 8TML direction and release guidance
tools/         build, test and package helpers
packages/c64u/ pinned matching alpha binaries and integrity records
```

The released baseline and detailed build instructions are in [packages/c64u/README.md](packages/c64u/README.md). For source builds with cc65, use `make -C client independent-package`; for the legacy PHP package, use `php tools/package-site.php`. Do not mix matching binary versions. [docs/LOCAL_TESTING.md](docs/LOCAL_TESTING.md) owns test/hardware evidence; passing host/VICE checks is not real C64U acceptance.

The current `main` history diverges substantially from the alpha development branch. Check refs, open PRs and source provenance before integrating; never assume `main` is the latest shipped code.

### Historical contracts (still active for alpha only)

- [C64U runtime and restoration](docs/C64U-RUNTIME.md)
- [Current alpha architecture](docs/ARCHITECTURE.md)
- [SEE64YOU/1 protocol](docs/PROTOCOL.md)
- [See64You Markup v1](docs/MARKUP.md)
- [Legacy PHP site](docs/SITE.md)
- [Hardware validation](docs/LOCAL_TESTING.md)

These remain documented to protect existing users, not because they are the future format.
