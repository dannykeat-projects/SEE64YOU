# Development and acceptance

Read [AGENTS.md](../AGENTS.md) and [todo.md](../todo.md); inspect current branch, package dependencies and any unrelated changes before editing. Use a disposable PHP site and backed-up storage. Record the exact source commit, build root, tool versions, checks run, hashes, failures and acceptance scope. An earlier passing run does not certify a new build.

## Checks without publishing credentials

Run `sh tools/validate.sh --host-only` for all host checks, including the retained PHP/legacy regressions and disposable loopback protocol tests. This mode does not run native builds, VICE or hardware; its success message states that scope explicitly. `sh tools/validate.sh` retains the full historical matrix and its failure gates.

Shell scripts are checked out with LF endings so Windows/WSL can use these commands directly. Native package/VICE commands below also need no account key. Real standard-server publishing and external-reader acceptance require the dedicated account credential and remain separate gates.

## Source and standard Publisher

From the repository root, with suitable C compiler, cc65, Python 3, PHP 8.1+ and optional VICE/ROMs:

~~~sh
python3 tools/test_8tml.py
python3 tools/test_source_loading.py --output client/build/source-loading.json
python3 tools/test_standard_protocol_vice.py --rom-dir LOCAL_ROM_DIRECTORY
php site/tests/eighttml.php
python3 site/tests/eighttml_http.py
python3 tools/test_publisher_8tml.py
python3 tools/test_storage_paths.py
make -C client independent-package
~~~

`test_source_loading.py` checks 1/16/60 KiB exact normalized source, REU transfer bounds and EOF/capacity/failure handling. Its host counters exclude DOS, overlays, UI and C64 latency. Measure real-C64U end-to-end load time separately before claiming a speed improvement.

Optional `--reference-tools PATH` in the 8TML scripts invokes the externally supplied 8TML 1 draft validator; it is not bundled pending licensing. Optional `--rom-dir PATH` enables cc65/PETSCII and VICE checks where supported. The legacy Publisher host test uses isolated PHP and remains a regression check for retained code. Standard transport tests and native protocol VICE tests exercise the replacement separately; neither proves real C64U networking or publisher UI. Validate strict source bytes, preserved unknown markup, JSON escaping, size bounds, authentication/replay, failed write/rename, collision refusal, draft privacy, state and identity handling.

For related regressions and packaging, when relevant:

~~~sh
php site/tests/run.php
php site/tests/themes.php
sh site/tests/http.sh
python3 tools/test_independent_programs_vice.py --rom-dir LOCAL_ROM_DIRECTORY
python3 tools/test_basic_return_vice.py --rom-dir LOCAL_ROM_DIRECTORY
sh tools/validate.sh
~~~

`tools/validate.sh` contains additional non-shipping targets. Diagnose failures and trace shipping callers; do not call the entire suite passed if one target fails. Enforce all launcher/Publisher ≥1,024-byte headroom guards, Editor region/overlay fit, manifests and complete ten-file package consistency.

To test an alternate root, force a full build, then check manifests and embedded paths:

~~~sh
make -B -C client STORAGE_ROOT=/Usb0/ independent-package
~~~

Use the actual mount name and trailing slash. USB tests do not establish USB-only operation simply because the package builds.

## Real C64 Ultimate acceptance (authorization required)

Hardware checks must be bounded and reversible. Verify clean idle BASIC, Command Interface, 16 MiB REU, expected storage root and absence of test fixture names. Capture original app-folder bytes/hashes, configuration, drives, DOS paths, IRQ/live turbo/colours and BASIC context. Do not execute a package if uploaded bytes differ from the build.

The disposable automated Editor/storage gate is:

~~~sh
python3 tools/test_8tml_c64u.py --host C64U_IP --output client/build/NEW-RECEIPT --reference-tools PATH
~~~

For focused native storage checks, add `--storage-only`; this retains package readback, app-folder backup and DOS/machine restoration but skips shipping Editor UI acceptance. Add `--source-benchmark` to check exact 1/16/60 KiB open/save/reopen sources and record native `clock()` ticks plus `CLOCKS_PER_SEC`. Those timings include source codec and DOS work, exclude shipping UI/startup, and do not establish a speedup without a comparable baseline. The extra timing scratch is captured and restored. Transient screen-monitor GET failures retry only within the selected wait deadline; writes are never replayed. Native-only receipts cannot substitute for shipping Editor UI acceptance in the release packager.

Use a matching `--package-dir`/`--storage-root` where needed. It stages and verifies a complete package, tests Editor create/save/recovery and a native local source/storage/fault probe, then restores original files and machine state. Check the receipt rather than inferring restoration. Remote key injection is **not** physical keyboard evidence; private receipts and storage backups must not be committed.

Before release acceptance, perform these additional checks and mark each **passed, failed, blocked or unverified**:

1. Physical C64 and intended USB keyboard: New/Open/Save/Save As, editing responsiveness, help/menu/exit confirmation, and cursor/selection.
2. Power off and cold restart; reopen saved source byte-exact, validate header/body, and distinguish existing recovery from unsaved edits.
3. Safely simulate permitted interruption/failed writes on disposable data; preserve existing files and recoverable temporaries.
4. Import a standard key in Settings, save/relaunch, then test Publisher status/list/create/explicit-replace/error/cancel against a disposable standard endpoint. Compare the received source and signature; never retry an uncertain write. Independent-server acceptance is a separate gate.
5. Verify complete cleanup, original configuration/files, DOS contexts, live machine settings and responsive BASIC numeric and string commands.
6. Upload through a real independent implementation and verify the reported URL and relative links in an **external compatible browser**. Mark this blocked if no independent endpoint is available.
7. Investigate USB transfer/readback integrity before any USB program execution; SD testing alone does not prove USB acceptance.

**Known unverified gates:** physical keyboard/modifier usability, cold restart, uncontrolled interruption, USB-only acceptance, new Publisher on real C64U, independent standard-server and T8WN/VVV interoperability. Don't merge/deploy code requiring these gates until they are appropriately validated.

The hardware harness accepts `--startup-timeout SECONDS` (15..1800; default70) for module preload/verification. With Turbo Control Off, verification runs at base speed and can exceed the default. Temporary enabling of C64U Turbo Registers must capture the original setting durably before applying it, then restore/verify it and mark the capture CLEAN after the program has returned safely to BASIC. Do not reset an active verification as a test shortcut.

Use `--settings-only` for bounded standard key import/persistence acceptance with the public vector credential. It checks secret redaction, explicit consent, save/relaunch, invalid-input preservation and cancelled replacement, then restores the backed-up files and machine baseline. It does not authenticate to a real account or prove publishing/network interoperability.

For the retained legacy Publisher layout, `make -C client publisher-overlay-fit` checks its three overlay slots, exact assets and resident headroom. `python3 tools/test_code_overlay_vice.py --rom-dir LOCAL_ROM_DIRECTORY` exercises generated entries/return bridges. These diagnostic checks do not certify legacy runtime transitions on C64U, and success does not imply the later managed Editor build fits.

Before a LAN fixture test, inspect inbound rules for the exact server executable as well as the listener address/port. An explicit Windows Block rule takes precedence over an Allow rule; a local loopback success does not establish C64U reachability. Do not disable existing firewall or VPN protections as a test shortcut. Arrange a temporary administrator-managed exception scoped to the C64U address, fixture executable and test port, then restore the original policy after testing. A TCP-open timeout before HTTP is a reachability failure, not evidence of a signing or reply-parser defect.

For a read-only network gate, compile `client/diagnostics/standard_network_probe.c` with `S64_STANDARD_STATUS_ONLY=1` and `S64_STANDARD_TEST_URL` set to the standard endpoint, then use `tools/test_standard_network_c64u.py --status-only` with that PRG. This mode performs status only and requires no real key. The harness reapplies captured BASIC colours after REST loading (which resets the machine), and snapshots `configs/*` values rather than just the configuration-category list.

A 16,385-byte Publisher source load measured 91.87 seconds with Turbo Control Off. For boundary-size hardware cases allow a bounded 300-second load wait; a 70-second wait can expire while valid I/O is still active. Preserve the failed receipt, wait for safe return and restore the captured baseline; do not reset or change acceleration while a program is active.
