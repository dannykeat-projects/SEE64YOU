# External engineering references — previous alpha research

> **Historical reference constraints only.** The approved 8TML direction supersedes the earlier blanket prohibition on new formats. Follow [8TML roadmap](8TML-ROADMAP.md); reuse outside code/spec text only after verifying its licence and compatibility.

External software is **reference material**, not a new SEE64YOU workstream. Do not import code, dependencies, protocols or hosting requirements before the independent MVP is usable. A replacement must demonstrably reduce total runtime complexity without breaking `.s64`, PHP hosting, published HTML, C64 browser links, recovery or hardware restoration.

## Established C64U platform references

- [Official Ultimate documentation](https://github.com/GideonZ/1541u-documentation) is the authoritative intent for Ultimate-DOS, Network/Control targets, REU, turbo and runtime capabilities. **Actual firmware-1.1 hardware tests override assumed availability**.
- Tested C64U: UCI `$DF1C..$DF1F`, Ultimate-DOS `$01`/`$02`, Network `$03`, required 16 MiB REU, HTTP `$06` absent. Use capability detection and preserve user's settings.
- [JC-000/c64-test-harness](https://github.com/JC-000/c64-test-harness) and [barryw/vice-mcp](https://github.com/barryw/vice-mcp) can help automate focused emulator/real-hardware acceptance **outside** production runtime. Adoption is optional; the current repo already has accepted harnesses.
- [barryw/sim6502](https://github.com/barryw/sim6502), [xlar54/ultimateii-dos-lib](https://github.com/xlar54/ultimateii-dos-lib), and [JC-000/c64-https](https://github.com/JC-000/c64-https) are engineering research references only. Do not add GPL library code into shipping clients without an explicit licensing decision.

- [Ultimate USB FTP corruption, issue #803](https://github.com/GideonZ/1541ultimate/issues/803), reviewed 22 October 2026: Gideon reports fixing unaligned USB DMA reads in the FPGA on 2 September. Our firmware-1.1.0/FPGA-122 machine reproduces USB-only FTP corruption while SD is exact; two 700-byte/20-ms paced disposable USB transfers pass. Likely related, not an independently proven root cause. Keep readback verification; do not treat smaller unpaced chunks as sufficient or assume a release contains the fix without checking its FPGA image. No firmware/source import or flash performed.

### USB input fixes reviewed 22 October 2026

The user's C64U still reports 1.1.0 / FPGA 122 / core 1.49. Their first mouse did not work in limited testing; they subsequently tried another and report that it works. The first model/receiver/hub remains unidentified, so no general mouse failure is established. Read-only configuration shows Joystick Swapper Normal and no newer Joystick Settings category. SEE64YOU installs cc65's C64 mouse driver; it does not enumerate USB devices. No firmware/configuration change or agent-performed physical input acceptance was performed.

| Symptom | Primary upstream evidence | Adoption boundary |
| --- | --- | --- |
| Mouse not recognized / limited compatibility | [HID report-protocol support #675](https://github.com/GideonZ/1541ultimate/pull/675), merged 12 April; [descriptor-read fix 04200ac0](https://github.com/GideonZ/1541ultimate/commit/04200ac0b4923ebc61d41814809b9c3be61739c3) and [interface limit 954104c0](https://github.com/GideonZ/1541ultimate/commit/954104c0b251153daac6b6d18c0ae62ffad057a0), 20 May | These postdate the official March-build 1.1.0. First test a plain wired mouse directly in a 1351 program on port 1; establish the exact device before attributing failure. |
| USB key stays pressed after leaving menus / runaway repeat | [Matrix-state fix #723](https://github.com/GideonZ/1541ultimate/pull/723), merged 17 July; [bounded repeat #796](https://github.com/GideonZ/1541ultimate/pull/796), merged 24 August | Distinct defects; do not assume every keyboard/receiver has them or that a community build includes both. |
| Mouse jumps while scrolling / remote menus steal clicks | [Mouse input fix #913](https://github.com/GideonZ/1541ultimate/pull/913), merged 16 September | A later-stack regression, not evidence for why this stock-1.1.0 mouse never works. |

[Commodore's download page](https://commodore.net/downloads/) still lists 1.1.0. [Community Prkl v1.1.0s2p8](https://github.com/jusii/prkl_ultimate/releases/tag/v1.1.0s2p8), released 11 June, advertises a RAM-loaded SoftPatch with broader HID support and the May descriptor/interface fixes, verified by its author on FPGA 122/core 1.49. It is a possible controlled compatibility experiment, not an official recommendation or locally accepted firmware. Its June release does not establish inclusion of later July–September fixes; it does not claim to replace the FPGA, so it cannot establish adoption of #803's FPGA fix. Preserve configuration and inspect exact binary/provenance before any separate firmware test. No external code imported.

## Alternatives evaluated for product scope

| Project | Relevant lesson | Why no wholesale replacement now |
| --- | --- | --- |
| [Compunet Reborn](https://github.com/tgreaves/compunet-reborn) | Small C64 client, server-driven page model, separate C64 and modern wire interfaces, protocol conformance testing | Compunet application model and historical C64 networking protocol differ; its Python services add hosting complexity relative to our working flat-file PHP site |
| [Commodore Markup Language (CML)](https://github.com/eightbitswide/CML) | Plain hosted HTML can contain C64-specific content | Requires different authoring/display semantics; remote machine commands are outside our read-only browser trust model; code licence needs independent verification |
| [Contiki C64](https://github.com/oliverschmidt/contiki) | Simple HTML can be read on vintage hardware | Browser/transport/setup are not a verified drop-in replacement for our published link/colour format or C64U interface |
| [HDN Shell](https://hdn.sh/) | Complex modern pages can be transcoded off-device | Requires different external browser/server design, not our independently hosted dual-presentation output |
| [drericflores/AdveditC64](https://github.com/drericflores/AdveditC64) | Compact editor implementation patterns | Our Editor and `.s64` round-trips already work; wholesale editor rewrite is unjustified |

Compunet Reborn's [MIT licence](https://github.com/tgreaves/compunet-reborn/blob/main/LICENSE) covers its modern recreation but explicitly excludes historical and third-party material. Maintain provenance and required licence/copyright notices for any later selective code import. Code from CML or other projects is **not** assumed reusable without licence review.

## Adoption gate (only after MVP validation)

1. Identify a **measured** release problem in SEE64YOU; don't replace code just because another design is interesting.
2. Trace actual call paths, build targets, tests and source/file compatibility contracts.
3. Establish licence compatibility and precise external source revision.
4. Prototype a **single bounded component** on a disposable branch with a clear rollback.
5. Compare shipping code size, dependencies, configuration work, memory fit, reliability, maintainability and user workflow against the existing implementation.
6. Pass source/host/PHP/VICE and affected real-C64U checks before removal.

If no candidate clearly improves all relevant trade-offs, retain the existing implementation. **No further external-tech evaluation is required for beta.** The release/stop decisions are in [`todo.md`](../todo.md).
