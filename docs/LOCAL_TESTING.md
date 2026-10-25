# Development and acceptance

Read [AGENTS.md](../AGENTS.md) and [todo.md](../todo.md); inspect current branch, package dependencies and any unrelated changes before editing. Use a disposable PHP site and backed-up storage. Record the exact source commit, build root, tool versions, checks run, hashes, failures and acceptance scope. An earlier passing run does not certify a new build.

## Source, Publisher and PHP

From the repository root, with suitable C compiler, cc65, Python 3, PHP 8.1+ and optional VICE/ROMs:

~~~sh
python3 tools/test_8tml.py
python3 tools/test_editor_stress.py --sanitize
php site/tests/eighttml.php
python3 site/tests/eighttml_http.py
python3 tools/test_publisher_8tml.py
python3 tools/test_storage_paths.py
make -C client independent-package
~~~

Optional `--reference-tools PATH` in the 8TML scripts invokes the externally supplied 8TML 1 draft validator; it is not bundled pending licensing. Optional `--rom-dir PATH` enables cc65/PETSCII and VICE checks where supported. The Publisher host test uses an isolated PHP service and a simulated UCI Network adapter; it does not prove real C64U networking or publisher UI. Validate strict source bytes, preserved unknown markup, JSON escaping, size bounds, authentication/replay, failed write/rename, collision refusal, draft privacy, state and identity handling.

For related regressions and packaging, when relevant:

~~~sh
php site/tests/run.php
php site/tests/themes.php
sh site/tests/http.sh
python3 tools/test_independent_programs_vice.py --rom-dir LOCAL_ROM_DIRECTORY
python3 tools/test_basic_return_vice.py --rom-dir LOCAL_ROM_DIRECTORY
sh tools/validate.sh
~~~

`tools/validate.sh` contains additional non-shipping targets. Diagnose failures and trace shipping callers; do not call the entire suite passed if one target fails. Enforce all launcher/Publisher ≥1,024-byte headroom guards, Editor region/overlay fit, manifests and complete independent-package consistency.

## Editing correctness, screen restoration and cost

`tools/test_editor_stress.py` compares production REU editing against an independent byte model over 10,800 deterministic actions and 36 size/pattern fixtures through 61,440 bytes. It checks clipboard replacement/capacity refusal, navigation, saved/dirty state and bounded multi-level undo/redo. The existing REU clipboard/history tests additionally inject read/write failures and check dirty state, cursor recalculation and invalidated history after partial operations.

~~~sh
python3 tools/test_editor_interactions_vice.py --rom-dir client/build/private-vice-roms
python3 tools/test_input_commands_vice.py --rom-dir client/build/private-vice-roms
python3 tools/test_editor_performance.py
python3 tools/test_editor_latency_vice.py --rom-dir client/build/private-vice-roms
~~~

Provide your existing compatible ROM directory. The interaction diagnostic uses production UI/menu/REU code with a small driver, not the shipping launcher/overlay lifecycle; it compares full source bytes across repeated Help/menu/prompt and clipboard transitions and checks blank tail rows. Its fixture setup is not keyboard input. The host performance report counts REU operations; host CPU time is not C64 latency. The VICE latency diagnostic measures direct Editor functions with the emulated `clock()`; zero ticks means below timer resolution. Neither performance diagnostic includes shipping overlay loading, redraw, C64U turbo or physical keyboard feel. Preserve report source/program hashes when comparing measurements.

The input-command diagnostic checks 18 native Commodore, mapped-keyboard and ordinary function-key decoding cases against production input code. It supplies KERNAL output bytes with the scanner paused; physical modifier timing and keyboard usability still require hardware acceptance.

The real-C64U harness below exercises the actual shipping function-key selection/copy/cut/paste path across Help dismissal, then compares saved bytes. Optional `--large-source` also opens, saves and reopens a 61,440-byte fixture, with a 40-minute completion window. The base-speed codec alone takes several minutes per pass; the shorter default test excludes that case. These are distinct from the diagnostic-driver and host gates. Maximum-size real-DOS acceptance remains unverified after the earlier shorter timeout; do not infer a hang or successful round trip from that timeout.

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

Use a matching `--package-dir`/`--storage-root` where needed. It stages and verifies a complete package, tests Editor create/save/recovery and a native local source/storage/fault probe, then restores original files and machine state. Check the receipt rather than inferring restoration. Remote key injection is **not** physical keyboard evidence; private receipts and storage backups must not be committed.

Before release acceptance, perform these additional checks and mark each **passed, failed, blocked or unverified**:

1. Physical C64 and intended USB keyboard: New/Open/Save/Save As, editing responsiveness, help/menu/exit confirmation, and cursor/selection.
2. Power off and cold restart; reopen saved source byte-exact, validate header/body, and distinguish existing recovery from unsaved edits.
3. Safely simulate permitted interruption/failed writes on disposable data; preserve existing files and recoverable temporaries.
4. Run Settings provisioning/relaunch and actual Publisher publish/update/download/error paths against a disposable configured PHP host. Verify original remote source and publication identity on failure.
5. Verify complete cleanup, original configuration/files, DOS contexts, live machine settings and responsive BASIC numeric and string commands.
6. Verify 8TML in an **external compatible browser**, and rendered modern HTML. Record incomplete 8TML rendering rather than claiming full interoperability.
7. Investigate USB transfer/readback integrity before any USB program execution; SD testing alone does not prove USB acceptance.

**Known unverified gates:** physical keyboard/modifier usability, cold restart, uncontrolled interruption, USB-only acceptance, new Publisher on real C64U, full HTML 8TML rendering, and T8WN/VVV interoperability. Don't merge/deploy code requiring these gates until they are appropriately validated.
