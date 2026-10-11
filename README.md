# See64You

**Write on a Commodore 64 Ultimate. Publish to the web.**

See64You is a C64 Ultimate-native text editor and lightweight web publishing environment. It keeps the familiar 40-column Commodore interface while deliberately using the Ultimate platform underneath it: UCI, Ultimate-DOS, Network Target, a required 16 MiB REU, DMA/REU transfers, turbo control where hardware-proven, SD storage and modern keyboard support.

See64You is **not** an original-C64 compatibility project. Ordinary-C64 memory limits, IEC loading speed and 64 KiB-only operation no longer constrain product architecture.

## Current priority: usable MVP

The public C64U site browser is now included in the requested scope. Its website
format supports the established `SEE64YOU-BROWSE/1` text/link records when
requested with `Accept: application/x-see64you-browser`. A newer client
requests `Accept: application/x-see64you-browser; version=2`, receiving the
same text/links plus C64 background, text and link colour indices. Older
browser/site combinations retain the original protocol. These colour changes
are source-implemented but still await C64U package/real-hardware acceptance.
Ordinary requests still receive HTML. Launch `S64WEB.PRG` independently
to read the configured public site, or press `O` to enter another HTTP URL.
Cursor up/down selects a row; RETURN follows site-coloured link text; SPACE/`-` pages
forward/back. RUN/STOP cancels a pending read or exits to BASIC. Loading feedback
appears before reads. HTTPS is explicitly unsupported on the firmware-1.1 direct
TCP path; no credentials are sent and HTTPS is never downgraded. This is a public
See64You reader, not a general HTML browser.

Website custom themes preserve their existing font, spacing and layout
controls. Website colour dropdowns accept only the 16 standard C64 palette
colours; saved legacy values are not overwritten automatically. Changing a
website's colours takes place only on the website, not from Editor, unless a
future user request reopens that scope.

The complete current application is not yet accepted end to end on hardware.
The immediate target is **three independently launched, file-based programs**: save a
canonical `.s64` in Editor, exit and open that file in Publisher, then publish or
download a new `.s64` for later reopening in Editor. Settings reads/writes the
existing config file independently. Each program starts fresh and returns to BASIC. The user-requested first-publication wizard now guides website setup, key import,
verified Settings saving and the file-based publishing steps. Optional backends
and large-document redesign remain deferred.

A measured candidate now builds with `make -C client independent-package` into
`client/build/independent-package/SEE64YOU/`. Launch `SEE64YOU.PRG` for Editor
(`S64EDIT.PRG` and its BIN files are internal Editor assets), `S64PUB.PRG` for
Publisher, and `S64CFG.PRG` for Settings. Publisher uses `L` to load a local
`.s64`, `2` to test the direct key, `3` to publish/update, `6`/`7` to open a
remote page/post, and `D` to save it as a new `.s64`; STOP exits to BASIC.
Editor uses white text on a black background.
New document names, downloads, autosave (`SEE64YOU.REC`), session journals and
storage-check temporary files now default to `/sd/SEE64YOU/`. To open an old
root document explicitly, enter `/sd/NAME.S64`; ordinary Save retains that
absolute filename. Use Save As with a new bare name to create a folder copy.
Corrupt or unreadable folder files never trigger an automatic root fallback.

If Editor reports `JOURNAL BLOCKED - RUN S64CFG.PRG`, launch Settings, press `M`,
read the migration screen and confirm `Y`. Settings resolves and verifies any
interrupted machine state, verifies retained root-journal backups, then removes
the old journal slots. This does not migrate documents or autosaves. Legacy root
`SEE64YOU.REC` is retained and is not automatically offered by Editor. Compare
root and folder recovery files before copying; keep backups and never overwrite
unresolved or different recovery. A saved canonical document can instead be
opened explicitly and saved under a new folder name.

The folder candidate passes host tests, all four packaged VICE program checks,
existing build guards and bounded real-C64U migration/edit/save/download/update
acceptance. These automated hardware checks use REST plain keys; USB and physical
power interruption remain unverified. See LOCAL_TESTING for exact receipts.

Editor and Publisher show Loading.../Saving... before local file operations;
Publisher shows Connecting.../Publishing... before those network operations.
These messages indicate activity, not a completion percentage.
Publisher also shows Loading... while fetching each remote list item and opening
the selected document, including after the replacement confirmation.

**Not yet a hardware-accepted release.** The launcher expects
`/sd/SEE64YOU/`; do not overwrite an existing installation or config without
backups (a separate test SD is preferred). The Editor launcher temporarily
preloads the other two PRGs to satisfy the current module manifest, but no
document, undo history or active application session moves between programs.

Bounded hardware checks now pass Editor save â†’ fresh Publisher publish â†’ remote
download â†’ Editor reopen/edit/save â†’ fresh Publisher update on a matching local
test site. Physical Ctrl+O and Settings persistence after a user-confirmed cold
power cycle also pass. Three functional round trips now pass (two guided native
keyboard cycles and a third using remote keys after physical Ctrl+O). Working
messages, partial-response rejection, bounded storage faults and live turbo
restoration at 0/15 pass. USB testing is user-deferred; physical storage/power
interruption and broader UI coverage remain unverified. See LOCAL_TESTING.

See [todo.md](todo.md) for the only live work queue and acceptance gates.
Prove the replacement on hardware before removing fallback machinery. Goals below are product direction, not claims that
every capability is complete.

## Start here: first page or post

Open your website's `start.php` for the six-step browser walkthrough, or follow
[the printable first-publication checklist](docs/FIRST-PAGE.md). Website setup
and HTTPS pairing link into the guide. On C64U, launch `S64CFG.PRG`: its three-step
wizard appears for missing site/key settings; W from Basic Settings starts it again.
The two guides refer to each other. Settings saves/verifies the configuration;
Editor writes a local `.s64`; fresh Publisher connects and publishes it. Check
page/post type and published state before publishing. No live state crosses programs.

The new wizard passes host/HTTP, package/fit and VICE navigation/consent/failure/exit
checks. A bounded real-C64U run using remote plain keys also passes first entry,
cancel, key import, explicit consent, verified save/relaunch, Publisher authentication
and Editor startup/exit. Original SD files and live turbo 15 are restored. Physical
wizard key input and a new cold power cycle remain unverified; USB stays deferred.

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
Editor (existing launcher) -> /sd/SEE64YOU/*.s64 -> Publisher -> remote site
                                              |
                                              +-> new /sd/SEE64YOU/*.s64 -> Editor
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

- todo.md â€” authoritative live work queue.
- docs/ARCHITECTURE.md â€” system component boundaries and data flow.
- docs/C64U-RUNTIME.md â€” C64U-only hardware/resource/state-management contract.
- docs/SETUP-UX.md â€” first-run wizard and Basic/Advanced settings UX.
- docs/UCI.md â€” UCI targets and hardware-proven behaviour.
- docs/EDITOR.md and docs/INPUT.md â€” editor and keyboard behaviour.
- docs/PROTOCOL.md â€” publishing/wire/authentication contracts.
- docs/LOCAL_TESTING.md â€” current reproducible validation procedures.
- docs/ENGINEERING-REFERENCES.md â€” external evidence, licensing and adoption boundaries.

Git history is the archive for superseded conventional-C64 plans and completed troubleshooting.

## Building the independent MVP

Requires cc65.

~~~sh
make -C client independent-package
~~~

Copy all eleven files from `client/build/independent-package/SEE64YOU/` into
`/sd/SEE64YOU/`. Launch `SEE64YOU.PRG` for Editor, `S64PUB.PRG` for Publisher,
and `S64CFG.PRG` for Settings; `S64WEB.PRG` reads published sites independently.
Do not cold-launch the internal `S64EDIT.PRG`.
Back up configuration, source and recovery files before replacing an installation.
The 14-file `c64u-package` and managed/persistent-runtime targets are retained
fallback diagnostics; their validation does not certify this independent package.

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
