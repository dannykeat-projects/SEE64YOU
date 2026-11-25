# C64 Ultimate release handover

Use a clean checkout of the candidate commit containing this guide on `codex/local-model-experiment`. Run hardware checks from a local machine that can reach the C64 Ultimate. Keep the original data, private receipts and publishing credentials on the local machine. Record each gate as passed, failed, blocked or unverified, with the reason and package hashes.

## Build and capture the baseline

1. Record `git rev-parse HEAD`, cc65 and VICE versions, firmware, PAL/NTSC, input devices and mouse settings, turbo settings, REU capacity and actual volume names.
2. Run `sh tools/validate.sh --host-only`. Force separate `make -B -C client STORAGE_ROOT=/sd/ independent-package` and `make -B -C client STORAGE_ROOT=/USB1/ independent-package` builds. Copy each complete `client/build/independent-package/SEE64YOU/` tree to its own local directory before rebuilding. Record SHA-256 for all ten files of each build, manifest checks and reported memory margins. Use the real USB mount name if it differs.
3. Run the packaged VICE smoke test against each retained tree, placing that tree at the normal build path for the test. See [development and acceptance](LOCAL_TESTING.md).
4. Start with idle BASIC. Back up the original app folder and confirm the fixture names are absent. Capture DOS contexts, drives, configuration, IRQ, turbo and colours. Verify the device reads back the exact candidate hashes before execution. Stop a USB run if any byte differs.

## Automated SD and publishing acceptance

Set `C64U_IP` to the device's LAN address locally. Use the complete SD package retained above and a new output directory. Start with the ordinary shipping Editor and native storage gate.

~~~sh
python3 tools/test_8tml_c64u.py --host "$C64U_IP" \
  --package-dir "$SD_PACKAGE" --storage-root /sd/ \
  --output client/build/candidate-sd --multiformat --source-benchmark
~~~

Run Settings import separately with `--settings-only` and a fresh output path. For publishing, run the full harness with `--publisher-acceptance` and a fresh output path using the existing dedicated T8wn test profile and matching `8TML.KEY`. The fixture covers disposable 8TML, 8SS and TXT creates, replacement, cancellation, exact public readback and positive refusal above the server limit. If a write has an uncertain outcome, inspect remote state before another attempt. Record the failing operation, HTTP code, server error code and message, normalized source hash and outcome. Keep private identifiers, signatures and keys out of shared logs. Repeat the reported 8TML failure with a sanitized copy if available. Use the known-good prior package and the same fixture to distinguish a regression from an endpoint failure.

Exercise Network target `$03` failed connect and successful reconnect, invalid test credentials, delayed/fragmented replies, cancellation and uncertain outcome using the [network acceptance procedures](LOCAL_TESTING.md). Verify socket cleanup and record the next successful status operation. Open a disposable published page in an external compatible reader and follow a relative link.

## Physical checks

| Gate | Procedure and expected result |
| --- | --- |
| Startup and BASIC return | Check prerequisite guidance on the installed firmware and prompt-free boot with prerequisites enabled. Exit normally, then run BASIC numeric and string commands. Check settings, IRQ, colours and DOS contexts against capture. |
| Recovery | For each snapshot format use a disposable valid snapshot. RUN/STOP keeps it through New, Open, Save and exit. Relaunch and recover with Y. Save explicitly and compare bytes. On a separate copy, D discards it. Failed reads and failed deletions retain the recovery copy. |
| Save and cold restart | Use all three New choices and an explicit `.md` suffix. Exercise exact-name Open, overwrite refusal and confirmation. Once back at idle BASIC, power off and restart, reopen and compare source bytes. For interrupted-save checks use disposable storage. Check original, `.tmp` and `.bak` bytes and report which copies survived. |
| Keyboard, mouse and latency | Compare candidate and baseline with identical 1,024/16,384/60,000-byte short-line and single-long-line sources. Match cursor and text on a fresh open and after insert/delete. Capture first-key and sustained input-to-screen latency, wrap on/off, fast/reverse drag, wheel, backward click and scroll. Record median, worst case and sample count. Check Link, first-body-line Delete, exact saved source and modal dismissal. |
| Publisher workflow | Check local paging and ignored keys, RUN/STOP during enumeration and a successful browse immediately afterwards. Confirm full source/target names, pending status, result acknowledgement and remote browsing after an error. |
| USB-only | With the exact build for the mounted USB root, repeatedly compare readback before execution, including after recopy and reboot. Any stale first sector blocks execution. Once readback passes, repeat startup, open/save/reopen and restoration checks on USB without SD fallback. |

## Return release evidence

Keep the harness `state.json`, exact package inventories and validation summary together. Record restoration of original files, device state and responsive BASIC. Return the candidate commit and sanitized results, including failed or unverified checks. The release packager requires a matching full SD hardware receipt and a validation summary with hashes for both roots. See the [packager](../tools/package_8tml_release.py) for exact fields.
