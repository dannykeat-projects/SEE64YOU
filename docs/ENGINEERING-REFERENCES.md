# External engineering references — previous alpha research

> **Historical reference constraints only.** The approved 8TML direction supersedes the earlier blanket prohibition on new formats. Follow [8TML roadmap](8TML-ROADMAP.md); reuse outside code/spec text only after verifying its licence and compatibility.

External software is **reference material**, not a new SEE64YOU workstream. Do not import code, dependencies, protocols or hosting requirements before the independent MVP is usable. A replacement must demonstrably reduce total runtime complexity without breaking `.s64`, PHP hosting, published HTML, C64 browser links, recovery or hardware restoration.

## Established C64U platform references

- [Official Ultimate documentation](https://github.com/GideonZ/1541u-documentation) is the authoritative intent for Ultimate-DOS, Network/Control targets, REU, turbo and runtime capabilities. **Actual firmware-1.1 hardware tests override assumed availability**.
- Tested C64U: UCI `$DF1C..$DF1F`, Ultimate-DOS `$01`/`$02`, Network `$03`, required 16 MiB REU, HTTP `$06` absent. Use capability detection and preserve user's settings.
- [JC-000/c64-test-harness](https://github.com/JC-000/c64-test-harness) and [barryw/vice-mcp](https://github.com/barryw/vice-mcp) can help automate focused emulator/real-hardware acceptance **outside** production runtime. Adoption is optional; the current repo already has accepted harnesses.
- [barryw/sim6502](https://github.com/barryw/sim6502), [xlar54/ultimateii-dos-lib](https://github.com/xlar54/ultimateii-dos-lib), and [JC-000/c64-https](https://github.com/JC-000/c64-https) are engineering research references only. Do not add GPL library code into shipping clients without an explicit licensing decision.

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
