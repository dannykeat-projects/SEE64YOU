# See64You

**Write on a Commodore 64 Ultimate. Publish to the web.**

See64You is a C64 Ultimate-native text editor and lightweight web publishing environment. It keeps the familiar 40-column Commodore interface while deliberately using the Ultimate platform underneath it: UCI, Ultimate-DOS, Network Target, a required 16 MiB REU, DMA/REU transfers, turbo control where hardware-proven, SD storage and modern keyboard support.

See64You is **not** an original-C64 compatibility project. Ordinary-C64 memory limits, IEC loading speed and 64 KiB-only operation no longer constrain product architecture.

## Current priority: usable MVP

The complete current application is not yet accepted end to end on hardware.
The immediate target is **three independently launched, file-based programs**: save a
canonical `.s64` in Editor, exit and open that file in Publisher, then publish or
download a new `.s64` for later reopening in Editor. Settings reads/writes the
existing config file independently. Each program starts fresh and returns to BASIC. Existing Basic Settings and
manual one-time prerequisites are sufficient; optional backends, expanded setup,
large-document redesign and UI enhancements are deferred.

See [todo.md](todo.md) for the only live work queue, continuation instructions,
acceptance gates and cleanup order. Prove the replacement on hardware before
removing fallback machinery. Goals below are product direction, not claims that
every capability is complete.

## Product goals

- A fast, predictable editor with familiar desktop editing behaviour.
- A small Markdown-like source format that remains the canonical editable document.
- Large working documents, clipboard and history backed by the 16 MiB REU.
- Independently launchable Editor, Publisher and Settings; internal overlays may be used when useful.
- Direct local storage through Ultimate-DOS.
- Direct Standalone publishing through Ultimate Network Target $03 on the firmware-1.1 baseline.
- A first-run setup wizard that hides UCI/REU/transport terminology from normal users.
- Temporary machine changes managed centrally and restored to the user's previous settings on clean exit.
- Publish the same source to a small database-free PHP site or, later, WordPress through the See64You adapter.
- Public standalone sites that require no JavaScript and remain usable in very old browsers.
- Keep the editor useful offline and publishing backends separate from editor state.

## Platform baseline

The committed product target is the **Commodore 64 Ultimate / compatible Ultimate platform with the required capabilities**, initially validated against firmware 1.1.

Required runtime capabilities:

- Ultimate Command Interface;
- Ultimate-DOS Target $01;
- 16 MiB REU;
- SD-backed storage.

Network Target $03 is required for the current direct Standalone publishing path but is not required for offline editing.

Where possible See64You probes capabilities instead of trusting firmware version strings. If a required machine setting cannot be safely changed from software, the first-run wizard gives the user the exact one-time Ultimate-menu instruction and immediately rechecks afterward.

See docs/C64U-RUNTIME.md for the platform contract.

## Runtime architecture (2026-10-09 decision)

The intended interface between user-visible programs is **ordinary files**, not
live REU/application handoffs:

~~~text
Editor (existing launcher) -> /sd/*.s64 -> Publisher -> remote standalone site
                                              |
                                              +-> new /sd/*.s64 -> Editor
Settings -> /sd/SEE64YOU/SEE64YOU.CFG -> Publisher
~~~

Each separately launched program exits to BASIC. The proven Editor bootstrap and
restore path is retained. The current packaged whole-PRG handoff system remains
an **unaccepted fallback** while this candidate is built and software/hardware
validated. No independent complete package has been certified yet.

## Current migration status

The repository still contains conventional-C64 code paths, software-IEC chaining and the earlier resident-loader diagnostics while the C64U-only architecture is being migrated. They are **transitional implementation**, not supported-product requirements.

Do not preserve an old path merely to keep ordinary-C64 compatibility. Remove it when the new C64U-native replacement is hardware-proven and the associated tests/docs have migrated.

The existing real-hardware baseline includes:

- UCI at $DF1C-$DF1F on the tested firmware-1.1 machine;
- Ultimate-DOS $01;
- Network $03;
- no HTTP Target $06 on that firmware;
- settings save/readback and cold power-cycle reload;
- authenticated direct GET;
- exact direct document upload/reopen;
- full Ultimate-DOS streaming of all current companion PRGs;
- successful overwrite/launch/return through the resident replacement-loader probe.

Outstanding implementation is tracked only in todo.md.

## Repository layout

~~~text
client/       C64 Ultimate client and diagnostics (cc65)
site/         Standalone flat-file PHP site and publishing API
wordpress/    WordPress publishing adapter
relay/        Optional trusted-LAN HTTPS relay
docs/         Architecture, runtime, protocol, editor and validation documentation
tools/        Validation/development helpers
~~~

## Documentation map

- todo.md — authoritative live work queue.
- docs/ARCHITECTURE.md — system component boundaries and data flow.
- docs/C64U-RUNTIME.md — C64U-only hardware/resource/state-management contract.
- docs/SETUP-UX.md — first-run wizard and Basic/Advanced settings UX.
- docs/UCI.md — UCI targets and hardware-proven behaviour.
- docs/EDITOR.md and docs/INPUT.md — editor and keyboard behaviour.
- docs/PROTOCOL.md — publishing/wire/authentication contracts.
- docs/LOCAL_TESTING.md — current reproducible validation procedures.
- docs/ENGINEERING-REFERENCES.md — external evidence, licensing and adoption boundaries.

Git history is the archive for superseded conventional-C64 plans and completed troubleshooting.

## Building during migration

Requires cc65.

The current C64U package is built with:

~~~sh
make -C client c64u-package
~~~

Copy the complete 14-file contents of client/build/c64u-package/ into
/sd/SEE64YOU/ and run SEE64YOU.PRG from that folder. The launcher validates and
caches matching modules/code assets, enters the managed editor and restores
machine state before returning to BASIC on exit. A 16 MiB REU and
UCI/Ultimate-DOS are required. Publishing config is authoritative at
/sd/SEE64YOU/SEE64YOU.CFG; the crash-recovery journals deliberately remain at
the SD root until their separate two-slot migration is hardware-proven.

Editor, Publisher and Settings now use the authenticated managed cached-transfer
cycle in software/CI, including return-state and ACTIVE-bound settings authority.
Real C64U acceptance of the complete Editor -> Publisher -> Settings -> Publisher
-> Editor cycle, repeated cycles, physical USB/native keyboard behavior and full
file/menu behavior remains pending. Do not cold-launch S64EDIT.PRG, S64PUB.PRG or
S64CFG.PRG: managed modules require trusted launcher/module entry.

Conventional-C64 and old standalone editor targets remain optional migration
diagnostics. They are excluded from current product artifacts and CI fit gates.

## Running the standalone site locally

Requires PHP 8.1+.

~~~sh
php -S 127.0.0.1:8080 -t site
~~~

Create an empty site/ENABLE_SETUP marker, open http://127.0.0.1:8080/setup.php, and complete the one-time server setup. Successful setup removes the marker and creates local configuration.

Build a clean upload-ready package with:

~~~sh
php tools/package-site.php
~~~

The default output is dist/see64you-site/.

## Remote publishing security

The firmware-1.1 direct publisher uses raw TCP/HTTP through Network Target $03. It never sends the normal site password or a reusable bearer token over that connection. Instead it signs protected requests with a dedicated 256-bit direct key and a one-time challenge/HMAC exchange.

This provides authentication, integrity and replay protection, not confidentiality. Direct HTTP therefore requires explicit user consent. HTTPS-required destinations can use the trusted-LAN relay until a hardware/firmware path with verified TLS is adopted.

## Philosophy

See64You should feel like a polished Commodore application, not a hardware demo. The 40-column interface stays deliberate and understandable, but implementation may use every useful C64 Ultimate facility behind it.

The rule is no longer **make it fit an original C64**.

The rule is **use C64U resources efficiently, safely and invisibly to the user**.
