# Architecture

See64You is a **C64 Ultimate-native** editor and publishing environment. It retains a Commodore-style 40-column interface while using the Ultimate platform as the actual runtime platform.

Ordinary-C64 compatibility is retired as a product requirement.

## Architectural principles

- Canonical source text belongs to the editor, not a backend.
- The editor remains useful without network access.
- The C64 Ultimate's 16 MiB REU is shared application memory, not an optional enhancement.
- UCI/Ultimate-DOS/Network/DMA/turbo facilities are platform services.
- Editor, Publisher and Settings remain separate executable modules where overlays keep the design simpler.
- To the user those modules must behave like one application.
- Temporary hardware changes are centrally owned and restored.
- More available hardware does not remove the requirement to stream, bound and validate data carefully.

## Runtime topology

~~~text
                    SEE64YOU launcher/supervisor
                              |
                  C64U system-state manager
                              |
       +----------------------+----------------------+
       |                                             |
 conventional C64 RAM                              16 MiB REU
 executing overlay + UI                shared control/state + allocator
 small hot buffers                     document/history/clipboard
 transition stub                       module cache/recovery/scratch
       |                                             |
       +----------------------+----------------------+
                              |
                         Ultimate UCI
                 +------------+------------+
                 |            |            |
             DOS $01/$02   Network $03  Control $04
                 |            |            |
            SD/files       TCP/IP       hardware/state
~~~

Optional/future UCI targets are capability-probed. HTTP Target $06 is not available on the tested firmware-1.1 C64 Ultimate and is not a baseline dependency.

## Launcher/supervisor

The final user-facing entry point is SEE64YOU.PRG.

Its responsibilities are:

1. detect required C64U capabilities;
2. recover/close an abandoned previous session where practical;
3. validate the 16 MiB REU requirement;
4. capture restorable machine state;
5. establish the shared REU control block and allocator;
6. validate/create application storage directories;
7. load persistent configuration;
8. validate the installed module manifest;
9. preload normal modules into REU;
10. offer document recovery if required;
11. launch the Editor.

After first-run setup this should normally require no user interaction.

## System-state manager

A single runtime service owns temporary machine changes.

Other modules request operating modes instead of directly changing global C64U state. Examples include:

- NORMAL/INTERACTIVE;
- CPU-INTENSIVE;
- I/O-SAFE;
- TRANSITION.

The manager captures the user's prior settings when readable, applies temporary changes only when needed and restores the captured values on clean exit.

A small persistent session journal records enough information to identify an interrupted See64You session and perform safe recovery/restoration on the next launch where possible.

See C64U-RUNTIME.md for the detailed contract.

## REU as shared application memory

A 16 MiB REU is required.

REU is divided into:

- a small stable control/ABI area;
- module-cache metadata;
- allocator metadata;
- dynamically allocated application memory.

The dynamic pool serves:

- active document;
- undo/redo history;
- clipboard;
- search/index scratch;
- publisher workspace;
- generated preview/cache;
- recovery shadows;
- remote-list/cache data;
- future features.

Avoid hard-coding most of the 16 MiB into permanent fixed banks. Stable addresses are reserved only where cross-module ABI or transfer mechanics genuinely require them.

## Shared control block

Every executable module exchanges state through a versioned REU control structure containing, at minimum:

- magic/version;
- application/package version;
- session state;
- active/requested module;
- allocator state/handles;
- document descriptor;
- cursor/selection/view state;
- history descriptor;
- clipboard descriptor;
- publisher binding/profile state;
- settings descriptor;
- recovery/session-journal state;
- module-cache descriptors;
- integrity/version fields.

A module must reject an incompatible or corrupt control block rather than guessing.

## Executable overlay model

Editor, Publisher and Settings remain separate PRGs/overlays because each can use the conventional address space cleanly while sharing large state through REU.

The production transition model is:

1. package manifest validates module filename, load address, length, version and integrity;
2. launcher opens the module with Ultimate-DOS;
3. DOS_CMD_LOAD_REU loads the full PRG, including its two-byte load header, into its assigned high-REU cache region;
4. later transitions select the already validated cached image;
5. a tiny overwrite-safe routine DMA/copies the cached PRG image into its C64 load range;
6. CPU/memory state is established;
7. the transfer stub writes a one-shot conventional-RAM arrival proof and execution enters the module startup;
8. the new module consumes that proof, validates the transfer capsule/shared control block, adopts the existing ACTIVE lifecycle without recapturing global state, and continues.

No normal module transition should depend on IEC device 8, KERNAL LOAD or the current software-IEC chain.

The earlier $C800-$CBFF resident-loader experiment is retired as the production design. Its real-hardware pass proves useful lower-level behaviour but does not impose a permanent 1 KiB reservation on the new architecture.

## Package manifest and integrity

The package manifest should record for each executable module:

- module identifier;
- filename;
- application/ABI version;
- PRG load address;
- executable extent;
- byte length;
- integrity checksum/hash;
- startup address.

The launcher validates modules before caching them. A later Editor → Publisher transition therefore does not depend on a fresh SD read.

Mismatched module versions must fail clearly before state is overwritten.

## Canonical document

The editable source remains metadata plus body text:

- type: page or post;
- slug;
- title;
- status: draft or published;
- date;
- order;
- body source.

Generated HTML is disposable.

SEE64YOU/1 remains the current canonical wire representation. Backends retain enough canonical source to reopen and edit exactly.

## Editor/storage evolution

The current REU-backed gap-buffer implementation is migration scaffolding, not the final size ceiling.

The C64U-only design will move toward a paged/piece-based document service backed by the REU allocator. Logical document positions may exceed 16-bit values while hot per-page operations remain small and efficient.

Undo/redo and clipboard storage become allocator-backed and memory-pressure-aware rather than fixed primarily by old C64 limits.

Memory reclamation order should prefer disposable caches and oldest history before any canonical unsaved source.

## Local storage

Local documents, settings, recovery and application resources use Ultimate-DOS.

Use DOS targets $01 and $02 as independent contexts where useful—for example one can retain the user's document/path while the other handles application resources, module cache loads or recovery work.

Saves remain transactional:

1. remove stale temporary output;
2. create/write/close a new temporary file;
3. validate where required;
4. replace the destination;
5. reopen/verify critical configuration data before reporting success.

A failed write must not intentionally destroy the last known-good document/configuration.

## Filesystem layout

The intended application tree is rooted at:

~~~text
/sd/SEE64YOU/
~~~

with logical areas for application modules, configuration, keys, documents, recovery, cache, temporary files, logs and updates.

The currently proven /sd/SEE64YOU.CFG path must not be moved until a tested migration writes the new location, reopens/validates it and preserves rollback.

## Publishing transports

### Direct Standalone transport

The firmware-1.1 baseline uses Network Target $03 for DNS/TCP and raw HTTP. Protected requests use a dedicated 256-bit direct key, one-time challenge and HMAC-SHA256 over the method, exact URI and body hash.

This provides authentication, integrity and replay protection. It does not provide confidentiality; direct HTTP requires explicit consent.

### HTTP Target compatibility/future path

HTTP Target $06 remains optional capability-detected functionality. It is absent on the tested firmware-1.1 machine and is not assumed from a URL string or firmware marketing version.

### Trusted-LAN relay

The relay remains an optional compatibility path for HTTPS-required origins and deferred WordPress use.

## Performance governor

Performance policy is centralized.

- Interactive mode: fastest hardware-proven setting that preserves correct input/display.
- CPU-intensive mode: temporary high turbo for search, hashing, transforms and similar work.
- I/O-safe mode: conservative settings for operations not yet proven reliable at higher speed.
- Transition mode: controlled state for module replacement.

Every temporary turbo/badline change restores the user's prior value.

## Recovery

Recovery has two levels:

- REU-resident cheap session/history snapshots;
- SD-backed durable recovery surviving reset/power loss.

Network or publisher failures must never make local source unavailable.

## Backends

The standalone site remains a flat-file PHP backend with no database requirement and no JavaScript requirement for public pages.

The WordPress adapter remains isolated from client state, uses WordPress-native permissions/authentication and preserves canonical See64You source in post metadata.

## Security boundaries

- The server validates authentication, type, slug and operation independently of the client.
- Browser/bearer credentials require HTTPS outside controlled testing.
- Direct firmware-1.1 publishing uses challenge/HMAC and explicit plaintext-HTTP consent.
- The direct transport does not claim source confidentiality.
- The relay is trusted-LAN-only and is never a general proxy.
- Hardware capabilities are probed and verified rather than inferred.

## Migration rule

During the C64U-only migration, old code may coexist with the new architecture only long enough to provide a safe replacement path.

Do not preserve:

- conventional-C64 release compatibility;
- 128 KiB minimum-REU assumptions;
- software-IEC chaining;
- $C800-$CBFF as a permanent production reservation;
- old RAM-headroom gates whose only purpose is original-C64 compatibility.

Current unresolved engineering and validation work belongs in todo.md.
