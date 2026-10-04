# C64 Ultimate runtime contract

This document is the authoritative hardware/resource/state-management plan for See64You.

See64You is intentionally **C64 Ultimate-only** and requires a **16 MiB REU**. The application should exploit useful Ultimate facilities aggressively while remaining disciplined about resource ownership, restoration and failure handling.

## 1. Platform contract

Initial supported baseline:

- Commodore 64 Ultimate / compatible Ultimate platform exposing the required interfaces;
- firmware 1.1 as the first real-hardware validation baseline;
- Ultimate Command Interface enabled;
- Ultimate-DOS Target $01;
- 16 MiB REU enabled;
- SD storage.

Network Target $03 is required for current direct web publishing but not for offline editing.

The shared platform preflight in client/src/platform/preflight.* now owns the required UCI, Ultimate-DOS $01, exact-16-MiB REU and SD-storage checks. Both the standalone diagnostic and SEE64YOU.PRG launcher call the same implementation. The storage probe selects /sd/, finds an unused S64CHK?.TMP candidate, writes and reopens a fixed marker, verifies exact readback, deletes it and confirms absence without overwriting a pre-existing candidate. The launcher presents exact requirement guidance and supports immediate RECHECK/EXIT. Real-hardware acceptance remains P0 work.

Optional capabilities are detected individually. Firmware version alone does not enable a code path.

## 2. One-time machine prerequisites

Some settings can only be useful to software after the user has enabled the facility in the Ultimate configuration UI.

The setup wizard may therefore require the user to enable:

- Command Interface;
- REU;
- REU size 16 MiB;
- a software-controllable turbo mode if register control is desired.

The wizard must:

1. probe first;
2. show only missing requirements;
3. give the exact menu action;
4. allow immediate RECHECK;
5. remember that setup is complete only after runtime verification.

Do not require Software IEC/device 8 for normal operation.

## 3. Capability inventory

At startup, build one capability record containing the results of bounded probes for:

- UCI register window/identity;
- DOS $01;
- DOS $02;
- Network $03;
- Control $04;
- Software IEC $05 only as diagnostic/legacy information;
- HTTP $06 if actually present;
- REU presence and usable 16 MiB extent;
- software-controllable turbo registers;
- optional runtime palette commands;
- other future features.

A generic success/status string is not enough to claim a target exists; require convincing identity/behaviour as appropriate.

## 4. System-state manager

No feature module owns global Ultimate settings directly.

A central manager exposes scoped modes and records every temporary setting See64You intends to alter.

State classes include:

- turbo CPU-speed register/control mode when readable;
- badline timing when controlled by the application;
- runtime palette if See64You ever changes it;
- C64 CPU interrupt/memory-map state used by low-level transfer code;
- open DOS/network resources;
- any future reversible Control Target state.

The manager follows:

**capture → journal → apply → use → restore → verify**

If the previous value cannot be determined and the setting is not guaranteed ephemeral, See64You should not silently change it.

The manager in client/src/platform/system_state.* is backend-driven and host-tested. It provides the four application modes, requires a successful capture before any mode write, avoids duplicate writes for the same mode, and makes clean restoration idempotent. A backend may verify restored state; verification failure remains visible and retryable. The launcher-owned coordinator in client/src/platform/session_lifecycle.* now composes system-state and journal operations so the ordering contract is executable and host-tested: capture -> durable ACTIVE journal -> apply; on shutdown restore/verify -> durable CLEAN journal. A failed ACTIVE write blocks the first managed change, while restore/verification failure deliberately leaves the durable session ACTIVE. The C64U turbo backend removes the old ad-hoc begin/restore guard and is exercised by host tests plus the direct-auth diagnostic. Linking manager/backend code into the current production publisher overflowed its conventional-RAM budget, so production publisher acceleration is intentionally disabled until launcher/session ownership or the C64U-native module layout removes that migration constraint. Final launcher/module binding and real-hardware acceptance remain P0 work.

## 5. Settings See64You should not take ownership of

Do not automatically rewrite unrelated persistent machine preferences merely because they are reachable:

- SID/audio configuration;
- HDMI/video preferences;
- drive A/B configuration unless a specific feature explicitly requires it and restoration is proven;
- cartridge selection;
- DHCP/static network configuration;
- Ultimate home directory;
- user interface colours;
- joystick/mouse preferences;
- unrelated palette configuration.

Use every useful C64U facility, not every setting.

## 6. Session journal

Before the first temporary global change, create/update a small persistent session journal.

It records:

- format/version;
- session ID;
- whether the previous run exited cleanly;
- restorable settings captured by See64You;
- current package version;
- recovery state references.

On clean exit:

1. resolve unsaved-document handling;
2. close network/DOS resources;
3. restore temporary settings;
4. verify restoration where possible;
5. mark the session clean;
6. remove/invalidate the recovery journal as appropriate.

On next launch after an unclean session, perform only restoration actions that are safe and well-defined.

The journal does not promise to restore arbitrary C64 RAM or another program that was running before See64You.

The first journal implementation is now host-tested in client/src/platform/session_journal.*. Its fixed 24-byte wire format is encoded explicitly rather than serialising compiler struct layout. It records format version, ACTIVE/CLEAN state, caller-supplied package version and session ID, a monotonically increasing sequence, recovery reference, and explicit restore-valid flags plus values for state See64You actually captured. The current restore payload can carry the captured turbo-control byte only when its valid bit is set.

Until the final /sd/SEE64YOU/ tree lands, the journal uses two small root-SD slots, S64JRN0.DAT and S64JRN1.DAT. Each update writes and reopens/verifies the alternate sequence slot; the previous valid slot is not deleted first. Startup chooses the highest valid sequence. If the newest slot is torn/corrupt, the previous valid slot remains available, which can cause a safe false-positive interrupted-session recovery rather than losing the captured state. The session lifecycle coordinator owns ACTIVE/CLEAN sequencing but is intentionally not linked into the constrained editor/publisher overlays; the final launcher/supervisor will bind its save callback to s64_session_journal_store_save().

Startup inspection and recovery resolution are now separately host-tested in client/src/platform/session_recovery.*. The launcher-facing classifier distinguishes no journal, clean prior session, interrupted ACTIVE session, invalid journal and storage failure. Only an interrupted ACTIVE record is converted into a restore snapshot, and that snapshot contains only fields whose journal restore-valid bits were explicitly recorded; a clean, invalid or unreadable record never exposes a restoration action. The resolver restores those captured fields, verifies them when a backend provides verification, then writes a CLEAN supersession. Restore/verification failure leaves the session ACTIVE; a failed CLEAN write also leaves the inspected in-memory journal ACTIVE so recovery remains safely retryable. The launcher-owned recovery flow in client/src/launcher/recovery_flow.* now applies startup policy without coupling it to editor UI: NONE/CLEAN proceed silently; INTERRUPTED automatically runs safe restore/verify/CLEAN resolution; failures return concise RETRY or DIAGNOSTICS outcomes while leaving retryable ACTIVE state intact. The launcher runtime adapter in client/src/launcher/recovery_runtime.* now binds that policy to the real Ultimate-DOS journal store and the existing turbo backend's restore/verify callbacks. A real C64U launcher entry point now wraps that runtime in client/src/launcher/main.c and builds as SEE64YOU.PRG. It first runs shared platform preflight, then automatic startup recovery. After recovery succeeds it begins the lifecycle coordinator: capture -> durable ACTIVE -> INTERACTIVE. The launcher runtime now exposes an explicit ACTIVE-preserving transition boundary; the editor handoff stub switches to TRANSITION and, because cached-module transfer is not implemented yet, cancels back to INTERACTIVE without cleaning the session. Only the later exit path performs restore/verify -> durable CLEAN. This establishes the non-legacy handoff contract without invoking software-IEC chaining. The next P0 step is the shared control metadata/module-cache path that actually transfers execution to the editor.

## 7. Performance governor

Use central modes rather than ad-hoc turbo writes.

### INTERACTIVE

Fastest hardware-proven mode for typing, navigation and UI rendering.

### CPU_INTENSIVE

Temporary high turbo for operations such as:

- SHA/HMAC;
- large search/replace;
- source transforms;
- indexing;
- preview generation;
- package verification.

### IO_SAFE

Known-safe settings for timing-sensitive operations until faster modes are proven.

### TRANSITION

Known CPU/interrupt/memory state for executable overlay replacement.

Every scope restores the previous mode on exit.

Current conservative turbo mapping, pending real-hardware acceptance: only CPU_INTENSIVE requests the existing maximum $D031 speed nibble while preserving the captured upper bits. INTERACTIVE, IO_SAFE and TRANSITION request the captured original byte. If register control cannot be safely detected, the backend makes no turbo change. The direct-auth diagnostic currently exercises this mapping; production publisher SHA/HMAC remains unaccelerated until the manager can be linked without violating the publisher's conventional-RAM safety budget.

## 8. REU resource model

The 16 MiB REU is a shared application heap.

Reserve fixed addresses only for data that truly requires a stable ABI address:

- global control block;
- allocator metadata;
- small transition metadata if needed.

All other large data should come from a central allocator.

The allocator/control-block foundation lives in client/src/platform/reu_runtime.*. During migration, banks 0-1 remain reserved for the existing document/history/clipboard/settings/handoff layout. A fixed 256-byte shared-control region begins at bank 2, with allocator-managed space starting at linear REU address 0x020100 and extending to the 16 MiB limit. The control block has an explicit versioned/checksummed 192-byte format-2/ABI-1 wire format with 16 ownership entries, application/session metadata, remaining-space accounting and owner-tagged aligned bump allocations. It now has REU-backed store/load/readback verification using the reserved bank-2 region. After the launcher has durably entered ACTIVE state, SEE64YOU.PRG initializes this block from the journal package version/session ID and refuses to proceed to transition preparation until exact REU readback validates. There is still intentionally no free/reclamation API; reclamation policy and migration of existing fixed allocations come after the ABI and module cache are proven.

Allocator clients include:

- document pages/pieces;
- undo/redo history;
- clipboard;
- module cache;
- recovery shadows;
- search/index scratch;
- remote list/open cache;
- publisher workspace;
- preview/render scratch;
- update staging.

The allocator must expose free space and ownership for diagnostics.

## 9. Memory-pressure policy

Reclaim in roughly this order:

1. disposable generated caches;
2. remote-list/preview caches;
3. old recovery shadows that have durable replacements;
4. oldest undo history.

Never silently discard:

- canonical active document source;
- unsaved persistent configuration;
- currently required clipboard content during an operation;
- critical shared control/session metadata.

## 10. Module cache

Normal executable modules are preloaded and validated at startup.

Ultimate-DOS provides DOS_CMD_LOAD_REU (command $21) to load bytes from the currently opened file directly to a specified REU address. Official documentation notes that this function assumes a 16 MiB REU configuration.

For each module:

1. select the application directory using a dedicated DOS context;
2. open the exact file;
3. validate metadata/header/size;
4. choose/allocate the cache destination;
5. load the file/payload directly into REU;
6. verify expected transferred length/status;
7. validate integrity;
8. mark the cache entry ready.

Module descriptors contain ID, filename, ABI/application version, size, load address, startup address, cache location/capacity and integrity data. The initial manifest implementation now defines Editor (S64EDIT.PRG), Publisher (S64PUB.PRG) and Settings (S64CFG.PRG). SEE64YOU.PRG reserves a 64 KiB aligned cache region for each module through the shared allocator and persists those allocations before transition preparation. Artifact-dependent fields remain unset in the static C identities; a descriptor is not considered loadable until load/start addresses, exact byte length and integrity are populated and validated. The package build now generates S64MODULE.MNF only after S64EDIT.PRG, S64PUB.PRG and S64CFG.PRG exist. The generator derives the PRG load address from the two-byte header, parses the BASIC SYS stub for the startup address, records exact full-file byte length and CRC32, then validates the emitted manifest against the same PRGs before packaging it. This avoids guessed or duplicated artifact metadata. The C64-side parser now consumes this file after the launcher has reserved all three cache regions. It requires the exact manifest magic, exact module order/filenames, fixed-width hexadecimal load/start/integrity fields, bounded decimal length, and no trailing records. The launcher reads the manifest through Ultimate-DOS from /sd/, fills the allocator-backed descriptors and blocks transition preparation until all three descriptors pass loadability validation.

## 11. Executable transition

The launcher now preloads the three manifest-described files using Ultimate-DOS
`$21` in IO_SAFE mode. Its application-resource context is explicitly DOS `$01`
at `/sd/`; no user-document handle shares this context during startup. Each file
is opened, its exact size checked with FILE_INFO, transferred and closed. The
focused DOS API accepts an already-open file and rejects zero/unrepresentable
lengths, non-24-bit addresses and extents outside 16 MiB before issuing a command.
It requires successful status and the exact reported transfer count/destination.
The official protocol uses two little-endian 32-bit fields; the reply contains
six-character hexadecimal fields that firmware may pad with spaces.
DOS reply parsing compares wire ASCII explicitly. Manifest reads convert ASCII
to native text before comparing the parser's PETSCII identities/delimiters.

Caches contain **full PRG files**, including the two-byte load-address header.
Manifest lengths and CRC32 therefore describe those exact cached bytes. A future
execution transfer must skip those two header bytes when copying into C64 RAM.
Cache states are EMPTY, LOADED_UNVERIFIED and READY. After preload, the launcher
verifies all three cached full-file CRC32 values against the manifest before
marking the whole set READY. Executable handoff is enabled only in the managed diagnostic package.
Any failed transfer/size/close leaves that entry EMPTY. A retry resets all states
before loading the full set again. CRC32 uses the standard reflected polynomial 0xEDB88320, initial/final XOR
0xFFFFFFFF, matching the build generator's zlib CRC32. A 64-byte nibble table
and 256-byte static DMA scratch keep conventional RAM bounded. Reads split at
bank boundaries and cover only each exact file extent. Reverification revokes
prior READY state; any descriptor, DMA or checksum failure leaves no module
READY. Retry reloads the files before hashing again. The verifier requires an
active IO_SAFE system-state scope. Each DMA runs in IO_SAFE; only the CRC loop
uses CPU_INTENSIVE, returning to IO_SAFE after each chunk. Mode failures also
reject readiness and attempt IO_SAFE before the session owner restores/verifies.
The launcher rechecks IO_SAFE before every preload retry. On firmware 1.1,
complete startup through verified CLEAN took 12.4 seconds with captured turbo
byte 0, compared with over 200 seconds for the prior IO_SAFE-only verifier.
This measures the complete launcher, including preflight, preload and journals;
it is not an isolated CRC benchmark. Turbo readback returned to byte 0.
After cache planning, the allocator reserves a separate 128-byte, 256-aligned
RUNTIME allocation tagged with module-session flag 1. With today's three cache
slots this is 0x050100..0x05017F; code discovers it from allocator ownership,
not that address. The control wire format/ABI and module manifest stay unchanged.
Before every preload the launcher writes/readbacks a zero record, revoking shared
readiness before cached bytes change. After all CRCs pass it publishes and
readbacks the record, then decodes it through the destination-facing load API.

The version-1 record uses fixed little-endian encoding: bytes 0..7 contain ASCII
S64M, record version, runtime ABI, module count and a zero reserved byte; bytes
8..31 contain the exact 24-byte durable ACTIVE journal; bytes 32..109 contain
three 26-byte descriptors (ID, READY state, ABI/app version, load/start address,
full PRG length, CRC32, cache address/capacity); bytes 110..125 are reserved zero;
bytes 126..127 contain a nonzero rotate/XOR checksum. Canonical filenames are
reconstructed from IDs. Struct memory is never serialized. Static bounded
scratch makes the codec non-reentrant.

A consumer reads the shared allocator control and supplies the current journal
from its trusted active lifecycle/verified handoff capsule. Cold inspection must
read the newest durable DOS journal; normal handoffs need no fresh SD read.
Loading requires ACTIVE with a nonzero sequence, exact journal-byte
agreement, matching control session/package, supported record/ABI, checksum and
reserved bytes, all three READY descriptors and unique exact MODULE allocation
ownership for their extents. CLEAN, stale, damaged or mismatched metadata fails
without modifying the caller's output. The record supplies restore state via
the bound journal; it does not replace durable recovery or independently prove
that cache bytes have remained unchanged since verification. The launcher clears/readbacks shared readiness before restoration and durable
CLEAN, including failed publication, so an old ACTIVE capsule cannot authorize
a closed session. Failed revocation blocks CLEAN and preserves recovery state.
A fresh destination runtime can now adopt the verified caller's ACTIVE journal:
load allocator/control and READY descriptors from REU, check the compiled package
and destination ID, validate the original turbo byte read-only at TRANSITION,
then adopt the original snapshot and journal into the central manager/lifecycle.
It does not recapture, probe/write turbo, read DOS or save another ACTIVE record.
An unavailable turbo setting remains unowned. Subsequent managed acceleration
and exit still restore the original caller snapshot before CLEAN. Adoption
rejects owned destinations, invalid/CLEAN/zero-sequence transfers, mismatched
package/cache metadata and unexpected turbo state without claiming ownership.

The existing hardware gate now simulates source/destination replacement with
independent runtime instances, checks zero destination DOS calls during adoption,
accelerates in the adopted owner and proves restore/verify -> CLEAN with captured
byte 2. No executable code is overwritten in this gate.

The verified transfer capsule is now implemented as a separate allocator-owned,
256-byte-aligned 64-byte RUNTIME allocation discovered by owner/flag rather than
a hard-coded REU address. The launcher issues a capsule only while ACTIVE and in
TRANSITION, only after the published shared-session record decodes for the same
journal, package and destination descriptor. The capsule contains a fixed
version/ABI header, destination module ID, the exact encoded ACTIVE journal and
the exact READY destination descriptor, followed by reserved zero bytes and a
nonzero checksum. Store is immediately read back and decoded.

Capsule validation requires the destination descriptor to belong to exactly one
MODULE allocation, retain the canonical module identity, fit its PRG payload
(load header excluded) wholly in C64 RAM, and place the startup address inside
that payload. CLEAN/stale sessions, invalid destinations, unverified modules,
retargeted cache addresses, corrupt/reserved bytes and unsafe execution extents
are rejected without modifying caller output. Session cleanup clears/readbacks
the capsule before revoking the shared module-session record and before
restore/verify -> CLEAN.

The execution-transfer implementation is now staged but not yet enabled in the
production launcher. A host-tested transfer plan derives the exact REU payload
source (cached PRG address + 2), RAM destination, payload length and startup
address from the verified capsule. It rejects REU wrap, RAM images below $0801,
images extending into $D000+, and startup addresses outside the copied payload.

The tiny 6502 transfer stub is linked into launcher builds as ordinary data and
is copied transiently into application-owned screen RAM at $0700 immediately
before execution. Its parameter block is at $07D0. This avoids reviving the old
permanent $C800-$CBFF loader reservation and keeps the stub below the current
$0801+ module image. The stub programs the REU registers, uses the same delayed
$FF00 trigger sequence as the previously hardware-proven REU DMA path, restores
the CPU memory-configuration byte and flags, then jumps indirectly to the
validated startup address. The assembly is size-guarded to remain below the
parameter block.

Production SEE64YOU.PRG still stops after TRANSFER CAPSULE READY / TRANSFER PLAN
READY / EDITOR HANDOFF PENDING; it does not call the new execution stub yet.
That final call remains gated on build/CI confirmation and real-C64U validation.

Because REU can survive reset, a valid capsule alone is not sufficient proof of
an immediate handoff. The transient transfer stub now writes a one-shot
conventional-RAM arrival marker at $02A7-$02A9 immediately after DMA and before
jumping to the destination. The destination consumes and clears this marker
before managed-session adoption. A cold/direct module launch therefore cannot
adopt persistent REU state merely because a previous ACTIVE capsule still
exists.

A lightweight destination runtime owner is now host-testable independently of
the launcher. Given trusted arrival, it loads the runtime control and verified
capsule, adopts the original turbo snapshot without a fresh capture/write,
adopts the ACTIVE lifecycle and can resume INTERACTIVE. Untrusted/cold arrival
is rejected without claiming ownership. A dedicated managed-editor fit target
links this ownership stack beside the current C64U editor and requires at least
1 KiB conventional-RAM headroom before production S64EDIT.PRG is switched to it.

Destination builds retain the existing control/session/capsule decoders and
validation, but omit startup-only construction and manifest parsing. These
routines now live in reu_runtime_build.c, module_manifest_build.c,
module_manifest_parse.c, module_session_build.c and transfer_capsule_build.c.
The launcher and host tests link both sets; the destination fit target links
only the consumer set. Wire formats, allocation ownership and checksums are
unchanged. This separates startup responsibilities without duplicating codecs
or weakening the destination checks.

First-session creation now lives in session_lifecycle_start.c,
session_journal_start.c and system_state_start.c. The turbo probe/capture
backend lives in turbo_capture.c. Destinations bind the adoption-only turbo
backend, retaining apply/restore/verify callbacks with no capture callback.
They therefore cannot recapture the user's original snapshot before or after
adoption. Startup rollback and adopted-session exit share the same CLEAN save
helper. Launcher and capture-owning diagnostics still link full startup paths.

With Ubuntu cc65 2.19 and the repository -Os flags, the 2026-10-03 gate passes
18 affected host suites and the launcher build (8,887 bytes before the stack).
Separating startup-only routines reduces managed-editor MAIN overflow from
18,780 to 13,374 bytes. The current ordinary C64U editor still exceeds MAIN
by 1,979 bytes. These are linker failures, so production adoption/execution
remains disabled until the executing layout fits and clears its headroom gate.

The separate `editor-overlay-fit` diagnostic now links a resident image and
three internal code assets against the same final layout. MODEL holds editor
and formatting code; FILE holds document, local storage, screen, menu and
editor publisher-handoff code;
RUNTIME holds destination session ownership. Their CODE/RODATA share the
`$8C00-$C7FF` execution slot. DATA/BSS, low-level DMA, buffer/history, input,
UCI/DOS and the cache selector stay resident. The slot ends 1 KiB below the
software stack at `$CC00`; it is a RAM execution reservation, not a fixed REU
bank. Each cache must be an exact MODULE allocation from the shared allocator.

Resident veneers preserve A/X and both `sreg` bytes before loading a group and
tail-calling its implementation. Same-group calls stay direct. FILE screen
code's three MODEL selection queries use explicit resident return bridges:
call MODEL, preserve its result, reload FILE, restore the result, return.
The build rejects other cross-group imports. FILE's temporary string pointers
are consumed immediately by resident status rendering; DOS document callbacks
and session/backend callbacks run within the already loaded group. Overlay
code is not called from interrupts.

With the same cc65 toolchain, the diagnostic fit passes: MODEL 14,828 bytes,
FILE 14,773 bytes, RUNTIME 14,928 bytes including asset/session/return consumers,
each below the 15,360-byte slot limit; resident BSS ends at `$86E1`.
cc65 crt0 starts its software stack at MAIN start + size, so MAIN extends to
`$D000` while the separate resident-fit check keeps BSS below `$8C00`.
The linker asserts that actual crt0 stack origin equals HIMEM; the 1 KiB
stack occupies `$CC00-$CFFF`, above the overlay slot and its 1 KiB gap.
Host selector tests and a VICE 3.9 fixture passed.
The fixture copies tiny linked code assets from resident arrays and exercises
30 swap cycles, 32-bit arguments/results, caller restoration, persistent data
and cache hits. It does not prove real REU DMA timing or production editor use.

The current c64u-package uses this managed editor root. Managed entry consumes
the copier marker and configures the selector from exact linked lengths only
on trusted arrival. It then adopts the original ACTIVE session, validates the
asset record in IO_SAFE, and resumes INTERACTIVE before calling editor code.
Cold or failed entry stops in resident code and leaves ACTIVE recoverable.
The diagnostic asset launcher now integrity-checks these exact assets and
preloads/publishes allocator-owned caches. `managed-package` stops before the
copier; `managed-handoff-package` explicitly enables the diagnostic executable
transfer and requires the same exact manifests and launcher headroom gate.
The c64u-package target assembles the executable variant into a separate product
directory and revalidates both manifests against the copied files. Standalone
launcher and managed-package targets remain gated preload diagnostics.
Managed publisher transitions are blocked until
their ownership path is ready. Clean editor exit revokes assets before the
capsule/shared-session clear and restore/verify/CLEAN path; a CLEAN retry skips
already completed revocation. Legacy publisher-return records are not imported
into a managed initial entry.

The code-asset foundation now supplies that package and session contract.
`editor-overlay-fit` emits/revalidates `S64OVL.MNF` beside the exact resident
PRG and canonical `S64MODEL.BIN`, `S64FILE.BIN`, `S64RUN.BIN` copies. The
52-byte little-endian manifest contains `S64O`, format/count bytes 1/3,
16-bit ABI/application version, slot address/capacity and zero reserved word,
32-bit full-editor length/CRC32, three ordered length/CRC32 pairs, then CRC32
of the preceding 48 bytes. Its root payload must end before `$8C00`, and its
SYS address must lie inside that payload. The C parser checks the exact same
root/version/layout binding and leaves output unchanged on failure.

`code_assets.c` atomically plans three exact, 256-byte-aligned MODULE allocations
with flags `$21/$22/$23` and a 96-byte RUNTIME record with flag 3. It first
clears/readback-verifies prior asset readiness, then preloads the fixed raw
filenames through native Ultimate-DOS, checks exact lengths and close results,
and verifies every full cache CRC32 before marking any group READY. The module
and asset paths share a bounded CRC32 updater and bank-safe REU chunk verifier;
DMA uses IO_SAFE and hashing uses the existing temporary CPU_INTENSIVE scope.
The owner still must restore/verify state after failures.

`code_assets_session.c` publishes/readbacks the checksummed record only for a
verified editor and completely READY asset set. Its `S64C` v1 record binds the
exact 24-byte ACTIVE journal, root length/CRC32, three ordered READY group
IDs/flags/cache addresses/lengths/CRCs and the execution slot. Load rejects
CLEAN, stale/mismatched sessions, checksum/reserved-field corruption and
non-exact cache ownership without changing output. Binding requires trusted
arrival, never reads SD or recaptures original state, and invalidates any old
selector configuration on failure. Failed publication blocks handoff until
readiness can be cleared and verified; best-effort revocation alone is not
proof of successful cleanup.

The foundation passes 20 affected host suites and the same VICE ABI fixture.
`code-assets-fit` links the complete producer beside the launcher with 2,134
bytes before the stack; the ordinary launcher retains 8,695 bytes after the
CRC extraction. These are layout gates. Production still does not read or
preload the asset manifest. The resident entry must obtain launcher-validated
cache handles before its first overlay call; the full record consumer is in
RUNTIME and cannot bootstrap itself from an empty slot. Only after this entry
boundary and launcher lifecycle/revocation integration are wired can the
matching package run the real C64U handoff/exit acceptance gates.

All launcher exits after durable ACTIVE, including partially failed session
start, now converge on session end: restore, verify, then durable CLEAN. If
cleanup fails, the launcher offers retry/exit and leaves ACTIVE available for
interrupted-session recovery. An error exit stays an error even after successful
cleanup. Real firmware-1.1 `$21` preload and full-file CRC verification have
passed with the matching package, followed by verified restoration and a durable
CLEAN journal from captured turbo byte 0. Same-length PRG-header corruption
has also been rejected on hardware, followed by X exit and a newer durable CLEAN.
Journal slots can be reused with WRITE | CREATE_ALWAYS, excluding CREATE_NEW.
The reproducible module-cache gate also passed with captured turbo byte 2,
verified restoration to 2 and a newer durable CLEAN. An outer diagnostic-only
manager restored its initial byte 0. Seventeen 16-byte sentinel blocks in the
migration document/history/clipboard/settings/handoff regions, the unused
control tail, immediately after the cache/session allocation range and distant banks
through bank 255 survived the real launcher run. The probe restored and checked
the original sentinel bytes. This is sampled preservation evidence, not an
exhaustive check of every unrelated REU byte. The
session-lived launcher runtime uses static storage to keep its large control
structure off the cc65 software stack.

The transition path should be as small as practical.

Normal flow:

1. current module commits shared state to REU;
2. current module requests destination module;
3. UI immediately displays the transition state;
4. transition code verifies the cached module descriptor;
5. a tiny overwrite-safe routine copies/DMA-transfers the cached image into C64 RAM;
6. required CPU/memory state is established;
7. execution enters the destination startup;
8. destination validates the shared ABI/control block.

The transfer routine should be placed only after measuring safe memory ownership on the C64U. Preserve any bytes in a shared low-memory scratch region if the location is not exclusively owned by See64You.

No normal transition performs a fresh IEC/KERNAL LOAD.

## 12. Dual Ultimate-DOS contexts

Targets $01 and $02 have independent state.

Preferred division, subject to measurement:

- one target retains user document/storage context;
- the other handles application resources, configuration/recovery, module loading and temporary work.

Do not hard-code this division if a particular operation benefits from switching roles; do keep each subsystem responsible for establishing its own path rather than relying on previous UI activity.

## 13. Filesystem plan

Target layout:

~~~text
/sd/SEE64YOU/
  APP/
  CONFIG/
  DOCS/
  KEYS/
  RECOVERY/
  CACHE/
  TEMP/
  LOGS/
  UPDATE/
~~~

Names may be adjusted for filesystem constraints, but roles should remain clear.

The existing /sd/SEE64YOU.CFG persistence path has already passed real power-cycle testing. Migration to the new tree must be transactional:

1. load/validate old configuration;
2. write the new location;
3. reopen/validate exact values;
4. retain rollback until success is established.

## 14. Recovery model

Use REU for frequent cheap state and SD for durability.

- REU recovery can capture recent editor/session state frequently.
- SD recovery survives reset/power loss.
- A recovered document remains dirty until explicitly saved.
- Successful save/discard cleans stale recovery state.

Publisher/network failures never invalidate local recovery.

## 15. Network model

Network Target $03 is the firmware-1.1 baseline for direct Standalone publishing.

The application owns sockets narrowly and closes them on all normal/error paths.

Raw $03 networking is not TLS. Challenge/HMAC authenticates and protects integrity/replay semantics but does not encrypt document content.

Future HTTP/TLS paths are capability-detected and hardware-validated before becoming preferred.

## 16. Control Target

Control Target $04 can provide hardware-management functions and should be evaluated narrowly.

Potential legitimate uses include:

- identify/capability queries;
- safe recovery/reboot routes after unrecoverable state;
- runtime palette capture/restore if a See64You visual mode ever uses it.

Do not use reboot as a substitute for cleanup and do not alter unrelated persistent settings.

## 17. Turbo

Official U64 turbo registers permit software speed selection only when the user's Turbo Control setting exposes register control.

Therefore:

- detect whether software control is available;
- capture the prior runtime value;
- use measured modes;
- restore the captured value;
- do not tell users to permanently choose maximum speed just because See64You can benefit from acceleration.

The setup wizard may recommend/require a register-controllable turbo mode if automatic performance management proves worthwhile, while preserving the user's selected underlying speed policy.

## 18. UI transition behaviour

Every overlay transition immediately replaces stale UI with a deliberate state such as:

- OPENING PUBLISHER...
- OPENING SETTINGS...
- RETURNING TO EDITOR...

The message is a failure-clarity feature even if a cached REU transition becomes nearly instantaneous.

## 19. Diagnostics

An integrated diagnostics screen should report:

- See64You/package version;
- detected hardware/firmware identity;
- UCI window;
- REU size/health;
- DOS targets;
- Network/Control/HTTP targets;
- current/previous performance mode;
- allocator usage;
- module-cache status;
- storage path;
- recovery/session-journal state;
- current site connection status.

Provide an SD-exportable diagnostic report without secrets.

## 20. Exit contract

Exit See64You performs:

1. dirty-document decision;
2. durable recovery/save updates;
3. socket/handle cleanup;
4. system-state restoration;
5. restoration verification where possible;
6. clean-session journal update;
7. predictable return to the C64 environment.

A failure to restore a setting should be reported rather than silently ignored when the application can still display a message.

## 21. Current real-hardware evidence

Firmware 1.1 has already demonstrated:

- UCI at $DF1C-$DF1F on the tested C64 Ultimate;
- DOS $01;
- Network $03;
- absent HTTP $06;
- persistent settings through physical power cycle;
- authenticated direct request;
- full DOS streaming of all matching application companions;
- overwrite replacement loader entering cc65 startup and returning successfully.

The launcher now additionally demonstrates direct REU preload, whole-package
CRC verification, corruption rejection, sampled unrelated-region preservation,
and verified turbo restoration with durable CLEAN from captured bytes 0 and 2.
The shared session record also round trips through the consumer decoder on
hardware, and the diagnostic rejects it against the newest durable CLEAN and
a simulated leftover ACTIVE capsule after exit revocation.
On 2026-10-04, `managed-handoff-package` ran the real cached Launcher -> Editor
transfer, trusted arrival, original ACTIVE snapshot adoption, asset binding and
INTERACTIVE resume. The editor declined a preserved recovery snapshot, displayed
injected `TEST` text, then accepted RUN/STOP and discard confirmation. Clean exit
returned to BASIC; independent journal checksum readback confirmed newest CLEAN
sequence 52/package 513. This exercises the software manager's restore/verify;
REST RAM DMA reads of `$D031` do not independently prove live I/O-register state.
Original SD companions, manifest and recovery file were restored with exact
SHA256 readback parity; only newly created diagnostic files were removed. Drive
configuration remained unchanged, including enabled Drive A. Physical keyboard,
long editing sessions and managed Publisher/Settings transitions remain untested.
The diagnostic now captures the actual original SYS context before stock launcher
crt0 runs. An ordered STARTUP object, guarded by a link assertion, records
original $01, incoming CPU SP/flags, the 26 cc65 zero-page bytes and all 256 CPU
stack bytes. Stock crt0 then runs unchanged. The existing overlay/stack gap holds
642 temporary bytes at $C800-$CA81; producer link guards exclude resident BSS and
software-stack overlap. This is scratch, not a resident-loader reservation.

The shared REU allocator reserves a 321-byte RUNTIME record (flag 4), containing
explicit S64B bytes, format/length, the exact ACTIVE journal, the 285-byte raw
context and CRC32. Publication requires verified control persistence and full
record readback. The managed editor loads and validates ownership, journal and
every integrity byte while ACTIVE in IO_SAFE mode. It readback-revokes the record
using separate scratch, preserving the validated RAM snapshot through asset,
capsule and shared-session teardown and restore/verify -> durable CLEAN. Failed
revocation or CLEAN blocks return. A missing/corrupt context still permits clean
shutdown, then retains the resident Session clean. Reset to BASIC. park.

After CLEAN, the transient $0700 return stub runs stock cc65 destructors (including
IRQ-vector restoration), restores CPU stack/zero page/$01/SP/flags, clears BASIC
status and RTS through the original SYS frame. No C runtime is used after its
zero page is restored. The VICE synthetic-frame probe verifies these restoration
mechanics. Host suites cover exact session/allocator binding, corruption of every
record byte, publication/readback/revocation failures and cleanup ordering.

On 2026-10-04, firmware 1.1.0 hardware ran this integrated diagnostic from the
real pre-crt0 capture through managed editor adoption, an injected TEST edit,
RUN/STOP/discard and restored BASIC. The original IRQ vector $EA31 matched;
the KERNAL clock advanced; BASIC executed PRINT 6*7, displayed 42 and returned
READY. Both journal checksums passed; newest CLEAN was sequence 56/package 513.
Original SD modules/manifest/recovery were restored with exact SHA256 readback,
temporary diagnostic files were removed, and drive configuration was unchanged.
This proves the bounded diagnostic exit, not physical keyboard, long-edit or
Publisher/Settings ownership cycles. The current c64u-package enables the proven
Editor entry/exit; other module ownership cycles remain gated.

To retain fit while adding this exit path, module/asset session producer and
consumer code now link separately. Wire contracts and shared non-reentrant scratch
are unchanged. The generated overlay entry veneers include only imported entry
names and explicit return bridges; unused internal entry stubs are omitted.

## Managed destination transition preparation

The platform module_transition API prepares a new destination from an already
adopted ACTIVE owner. It rejects a cold/restored/CLEAN owner, invalid destination,
same-module target or non-INTERACTIVE start. It zeroes the caller's output plan
before work, enters IO_SAFE, readback-revokes the previous transfer capsule,
publishes/readbacks and reloads the destination capsule, checks the exact ACTIVE
journal and derives a bounded payload plan. Only then does it enter TRANSITION
and expose the plan. It performs no SD access, allocation, state capture or
journal save; the original SYS return record and restore snapshot stay owned.

Cancellation clears the output plan, readback-revokes the candidate in IO_SAFE
and resumes INTERACTIVE. A failed preparation reports FAILED only after that
cancellation succeeds. Failed I/O mode, revocation or resume reports
RECOVERY_REQUIRED, leaves the session ACTIVE, and must not execute a transfer.
The caller owns document/history/clipboard persistence and input/resource
shutdown before successful overwrite; this planner does not execute DMA.

Host tests cover every operation/failure boundary and stale-plan rejection.
The real REU record/adoption host fixture repeats Editor -> Publisher -> Settings
-> Publisher -> Editor three times without changing ACTIVE, allocating again or
recapturing settings. These are ownership tests, not real UI/hardware cycles.

The managed Publisher fit diagnostic now links its actual trusted-arrival bootstrap,
ACTIVE-session adoption, specialized asset consumer and application-state decoder.
It consumes the one-shot Publisher arrival proof before resident control loading
and cache bootstrap, adopts through s64_module_runtime_adopt without recapture,
SD reads or another ACTIVE save, binds S64Q assets and validates the application
record before INTERACTIVE. Cold/direct entry parks with a restart-launcher message.

publisher-overlay-fit measures CONTENT 13,697, NETWORK 11,765 and RUNTIME 14,135
bytes in the 15,360-byte $8C00-$C7FF slot. Resident BSS ends at $866D with 1,426
bytes of headroom (minimum 1,024). These sizes include transition planning,
transfer/copier and original SYS-return support as well as the actual consumer.

Control decoding/validation, streaming document/DOS/REU operations and bootstrap
remain resident: selecting an overlay must not be a prerequisite for initial
control loading. RUNTIME owns adoption and asset binding; its initialization and
adoption execute in the same group because initialization temporarily clears the
control referenced by the selector. CONTENT owns application-state decoding,
transition planning, publishing state/preview and SYS-context validation. NETWORK
owns direct authentication/publishing. Explicit return bridges use the established
Editor veneer mechanism and restore the calling overlay, including nested calls.
The allowlist is limited to CONTENT's journal encode, system mode, capsule
clear/store/load and runtime prepare/resume calls, plus RUNTIME's application-wire
load. All other cross-group imports remain rejected.

The managed Publisher uses no legacy handoff or Settings helper exchange.
Editor return and Settings UI remain gated. Persistent settings format and
transactional /sd/SEE64YOU.CFG save/readback are unchanged; shared validated
session settings and managed Settings ownership remain future integration work.

Publisher asset foundation uses the same codecs compiled with
S64_CODE_ASSET_PUBLISHER=1. Its exact-root/CRC manifest is S64POVL.MNF (S64P,
format 1/count 3), with S64PCNT.BIN, S64PNET.BIN and S64PRUN.BIN in
CONTENT/NETWORK/RUNTIME order. Cache flags are $31-$33; its allocator-owned
96-byte session uses flag 5 and S64Q magic, distinct from Editor $21-$23/flag 3
and S64C. The selected module ID is enforced, so even identical root metadata
cannot authorize the other module's manifest/session. Both variants retain
atomic allocation, exact ACTIVE-journal binding, readback publication/revocation,
trusted-arrival-only binding and no-SD consumer behavior.

Host suites run both specializations, including opposite-module allocation
sentinels through preload/store/load/clear and wrong-root/magic rejection.
The Publisher fit builder generates/revalidates its exact manifest and links the
specialized consumer codecs into the measured destination. Producer/preload codecs
remain covered by both host namespace suites. The launcher still preloads only
Editor assets; no Publisher files are added to the current nine-file package.

### Shared application-state record (prepared, transitions gated)

One 512-byte, 256-byte-aligned RUNTIME allocation uses flag 6. Its fixed little-endian
S64A/version-1 encoding binds the complete 24-byte exact ACTIVE journal, including
sequence, session/package and restore snapshot. CRC32 covers bytes 0-507; bytes
508-511 carry the CRC. Reserved bytes, enums, booleans, string termination, buffer,
history and UI ranges are validated before modifying decoded output. Failed decode
or load leaves output unchanged; publication verifies all bytes in 32-byte readback
chunks. No raw C structs or document/history/clipboard payloads are serialized.

| Wire bytes | Meaning |
| --- | --- |
| 0-7 | ASCII S64A, format 1, runtime ABI, size 512 |
| 8-31 | Exact encoded ACTIVE journal |
| 32-39 | Nonzero generation, last writer/module, document-replaced flag, reserved |
| 40-45 | Gap start/end and logical cursor |
| 46-51 | All six REU history control fields; pending must be zero |
| 52-65 | View start/column, line/column, position cache, selection anchor, preferred column |
| 66-73 | Screen cursor, selection/preferred validity, mark, dirty, insert, wrap |
| 74-81 | Clipboard length, document type/status and signed order |
| 82-268 | Slug (65), title (81), date (41) |
| 269-336 | Local filename (34), repeat search (33), search validity |
| 337-507 | Reserved zero |
| 508-511 | CRC32 |

Editor capture calls s64_history_prepare_handoff before committing control state.
A pending operation, metadata read or dirty-cache flush failure blocks capture
without resetting history. Ordinary Publisher round trips preserve all Editor
fields, history, filename, clipboard and repeat search. Deliberate remote-open/import
uses the replacement helper: new metadata/body are authoritative, generation
advances, writer becomes Publisher and replacement is explicit; local filename and
undo/redo are cleared, cursor/view/selection reset to the beginning, dirty set.
Insert/wrap preferences, clipboard payload/length and repeat search remain intact.

Banks 0-1 retain their migration-era document/history/clipboard/settings layout.
Three module caches, module session, capsule, SYS return, both three-asset/session
namespaces and this single record total 15/16 allocator entries. No ABI expansion
or additional state allocations are introduced. RUNTIME authenticates the wire;
CONTENT decodes it using the existing Publisher workspace as scratch. One-shot
in-process authority connects the stages and is consumed on success or failure.
The workspace must not be reused between adoption and document decoding.

The Launcher reserves this record during cache planning. Managed Editor entry
now checks `S64_REU_RUNTIME_STATE_APPLICATION_VALID` after trusted arrival,
exact ACTIVE adoption and asset binding. Clear means fresh entry and runs the
existing Editor/document/clipboard/local initialization. Set requires the complete
record to authenticate against runtime ownership and the exact ACTIVE journal;
missing, corrupt or invalid payload state halts entry with ACTIVE recoverable.
It never falls back to a blank document. Successful resumption binds the decoded
Editor, document metadata, local filename, clipboard length, search/validity and
generation/source/replacement controls, leaving all REU payload bytes in place.
It defers recovery autosave instead of offering recovery over the resumed source.

Entry uses the existing asserted `$C800` gap synchronously: wire at `$C800-$C9FF`,
decoded controls at `$CA00` (currently 305 bytes), bounded below `$CC00`.
No permanent 512-byte wire or decoded-state BSS object is added. This temporarily
shares the BASIC-return workspace; entry never holds the return snapshot there,
and exit reloads it from its allocator-owned REU record before cleanup. The
1 KiB workspace/stack assertion remains unchanged. Wire authentication executes
in RUNTIME; payload decoding uses FILE and MODEL with explicit return bridges,
including nested MODEL -> FILE -> MODEL calls. Scalar readers/range tables and
startup binding remain resident so every overlay can safely reach them.
The managed Editor build excludes its unused legacy software-IEC handoff objects.
All three slots fit; resident BSS ends at `$8BFD`, leaving only two bytes before
`$8C00`. Any further resident change must pass the memory gates again.

Editor capture/publication and transactional validity-bit store/revocation remain
outstanding. Executable Publisher/Settings transitions remain blocked; this
consumer integration introduces no Publisher execution. Its changed package
passed the bounded first-entry/edit/CLEAN/BASIC-return hardware gate on
2026-10-05 (CLEAN 60/package 514); actual resumed-entry hardware acceptance
remains outstanding. See docs/LOCAL_TESTING.md for exact artifact evidence.

The allocator capacity change bumps package identity to 0x0202. Format-1
control blocks are rejected rather than adopted; interrupted-state recovery
continues to use the unchanged durable journal. The 192-byte control DMA remains
inside the same 256-byte reservation and heap addresses are unchanged. Host
tests cover all 16 entries, atomic exhaustion, round-trip/readback, old-format
rejection and a sentinel after the record. The rebuilt package-514 bytes passed the short real-C64U entry/edit/CLEAN/BASIC
return gate on 2026-10-04: newest CLEAN 58, IRQ parity, advancing clock and
PRINT 6*7 -> 42 -> READY. SD backups were restored with exact SHA256 parity
and drive settings were unchanged. Physical input/full file/menu acceptance
and Publisher cycles remain unverified.

## 22. Retired production constraints

The following are no longer design requirements:

- original C64 compatibility;
- a conventional-C64 release PRG;
- 128 KiB minimum REU;
- software IEC/device 8 for app transitions;
- fitting future convenience features solely to preserve original-C64 BSS headroom;
- reserving $C800-$CBFF for the final loader architecture.

Git history retains the earlier work.

The transient copier's one-shot arrival marker is consumed by resident `module_arrival.c`, independent of the launcher transfer planner. Every attempt clears the marker, including invalid and wrong-destination attempts. A valid marker authorizes only the entry boundary; full ACTIVE session and verified asset validation are still required before managed editor operation.

Resident `code_overlay_bootstrap.c` validates the application version and three exact linked asset lengths against unique, aligned MODULE caches with flags ` $21-$23` before selecting RUNTIME. It requires a caller-consumed trusted copier arrival from the launcher, whose preload must already have verified and published READY assets. It does not independently authorize persistent REU state or adopt a session. The entry caller must subsequently validate the complete asset record and adopt the ACTIVE session before editor operation. Every bootstrap failure invalidates the selector. This helper is host-tested and wired into the diagnostic managed entry.

The diagnostic final link exports `s64_code_overlay_linked_lengths` as three resident 16-bit values in MODEL/FILE/RUNTIME order, derived from linker CODE/RODATA sizes. Bootstrap entry can therefore use its own linked lengths without reading an SD manifest or duplicating size constants.

The `managed-package` diagnostic aligns launcher, managed editor and generated asset metadata through `platform/package_version.h` (currently `0x0202`). Binary manifest magic and the ca65 copier arrival marker use explicit ASCII byte values, avoiding cc65's PETSCII literal mapping. The asset coordinator reads exactly 52 binary bytes, checks close, parses/allocates, readback-persists the updated control, preloads/verifies all groups and finally publishes session-bound READY. Failure requires cleanup rather than a second allocation attempt. Cleanup clears asset readiness before capsule/module-session revocation and restore/verify/CLEAN; additional overlay allocations no longer bypass the launcher's shared-state cleanup.

On 2026-10-04, the managed preload diagnostic ran on C64 Ultimate firmware 1.1.0, FPGA 122, core 1.49 at 192.168.1.112 with Drive A enabled. It passed native module and code-asset preload/CRC/readiness, destination capsule/plan preparation, readiness revocation and restoration, and stopped at EDITOR HANDOFF PENDING before returning to BASIC. The common system-state manager restored/verified captured turbo 0; independently checked journal slot 0 was CLEAN, sequence 34, package 513, with a valid checksum. REST RAM-shadow readback was not independent live-register verification. Previous SD modules/manifest were restored with exact SHA256 parity, only newly uploaded asset files and the temporary PRG were removed, and drive settings remained unchanged. The subsequent executable entry/short edit/exit gate is recorded in section 21.
