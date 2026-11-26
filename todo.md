# SEE64YOU work queue

This is the live queue for the local-model experiment. [Product scope](docs/PRODUCT.md) defines the supported programs. [Development and acceptance](docs/LOCAL_TESTING.md) describes the commands and hardware procedures.

## Baseline and evidence

Alpha.8 is the released ten-file package. Ordinary sources live in `SEE64YOU/docs/`. The experiment adds an aligned REU read window and an 80-character Publisher status display. See the [release](https://github.com/dannykeat-projects/SEE64YOU/releases/tag/v0.2.0-alpha.8) and the [pre-audit evidence ledger at 53d006e](https://github.com/dannykeat-projects/SEE64YOU/blob/53d006e26e734e1ad7b2e2b8beec680b344429af/todo.md) for historical commands, hashes, regressions and hardware receipts.

Recorded candidate evidence includes 98 host groups, forced SD/USB1 shipping builds, and 24 REST-assisted SD hardware gates on firmware 1.2.1 at f96e40b. The latter covered 8SS/TXT public readback, replacement/cancellation and size rejection. The aligned window cut host REU loads for a 60,000-byte long-line navigation case from 1,345,687 to 55,486. Physical post-delete latency and the reported 8TML upload failure remain open. USB1 Publisher had 1,087 bytes free against the 1,024-byte minimum. One ordinary 8TML document did publish through the installed release package on the SD card on 25 Nov 2026 (installed `S64PUB.PRG` 44,863 bytes sha a9a26641..., `SEE64YOU.PRG` 47,129 bytes sha 50f1c1a5...): local `docs/notes.8tml` 200 bytes sha 0af7f0f0262e058e10071194a0a09ec134cc0f82476f8384301acf7229777430 produced **PUBLISHED** with `http://t8wn.com/@keato/notes.8tml`, the authenticated list gained only `notes.8tml`, and the URL reads back HTTP 200 with 216 bytes because the site inserts its own `style: plot.8ss` header line, which Publisher reports as a server warning; leaving that result screen with RUN/STOP showed **PUBLISH CANCELLED** and wrote nothing. The truncated server message in the reported failure did not appear on these screens, so its cause stays open. Receipts `client/build/acceptance-20261125-notes` and `client/build/acceptance-20261125-notes-final3`; ledger at ea3d44e.

## Outstanding release acceptance

- [ ] Capture a disposable version of the failing 8TML against the independent endpoint. Compare disk, normalized and received bytes, signature and stage of failure. Preserve sanitized diagnostics. Inspect remote state before retrying an uncertain write.
- [ ] Exercise native key errors, failed Network target $03 connect, reconnect and socket cleanup. Verify remote paging, cancellation, Settings import and published relative links in an external reader.
- [ ] On the matched package, verify prerequisite guidance, prompt-free startup, shipping Open/overwrite and pending messages, physical keyboard/Link shortcuts and mouse behavior on firmware 1.2.1.
- [ ] Compare fresh and post-delete documents with identical text and cursor. Capture first-key and sustained latency, wrap/reflow, forward/reverse/fast drags, wheel, backward clicks and scrolling. Verify first-body-line Delete, saved source and clean BASIC return.
- [ ] Verify cold-power reopen, interrupted save/recovery and full restoration using disposable data. Resolve stale first-sector readback and verify actual USB-only operation before executing USB binaries.
- [ ] Track independent-server draft changes and validator redistribution licensing. Revisit optional download-to-edit only if an independent implementation offers documented raw-source access over a supported transport.

## Landed in the release candidate

- [x] Startup distinguishes open, keep and discard. Keep protects the snapshot through later Editor actions. Failed deletion shows the DOS result. Save distinguishes a retained backup after installation from a failed rollback and keeps the Editor open to show the outcome.
- [x] Wrapped viewport repositioning avoids repeatedly scanning the same logical line. Paste writes one REU block per clipboard chunk. Save preflight validates source without serializing it to a discard sink. The second existence check remains because the host race fixture shows a destination can appear between probes.
- [x] Publisher uses elapsed idle and total reply deadlines, preserves server error codes separately from messages, and shows the operation and HTTP status. The local picker rescans only on entry or a page change and supports cancellation during enumeration. Native cancellation and reconnect remain release gates.
- [x] Editor Help spells out Undo and Redo and groups the 8TML-only commands. Retained WordPress list generation builds one response per permitted post. Relay body reads have a deadline. CI shares the package inventory check across both roots and shares Editor and Publisher host fixtures with PowerShell.
- [x] The retained Publisher overlay links without unused buffer editing functions. The historical managed Editor overlay is a non-shipping diagnostic with confirmed fit failures at baseline and candidate. The independent package is the shipping gate.
- [x] Measured the retained PHP site's list path with 512 records of 4,116 bytes and 256 published results. Nine local PHP 8.4 calls took 8.1 ms median and 8.8 ms max, with a 2 MiB process allocation peak. Keep the existing storage and draft filtering for this workload.

| Host REU measurement on a 60,000-byte logical line | Audit baseline | Candidate |
| --- | ---: | ---: |
| End-of-line read calls | 55,486 | 5,206 |
| Midpoint paste of 1,024 bytes, read calls | 28,634 | 2,821 |
| Save preflight and output bytes read | 240,128 | 180,096 |

The complete [audit at c419193](https://github.com/dannykeat-projects/SEE64YOU/blob/c419193fecf448ec26bc7aa1ce99da03b328c870/todo.md#code-quality-audit-and-implementation-plan) preserves the findings. Follow the [C64 Ultimate release handover](docs/C64U-RELEASE-HANDOVER.md) for exact-package hardware checks and receipts. Deployment remains gated on the matching SD receipt and both package inventories.

## Follow-up after hardware evidence

- [ ] Use measured first-key, backward click, scroll and drawing timings to decide which remaining Editor path to change. Check post-delete behavior against a fresh identical document before claiming it is fixed.
- [ ] Trace any observed 8TML upload failure across source bytes, challenge, PUT and acknowledgement. Keep the final advertised-size check.
  - **Recorded 25 Nov 2026:** the create path on the *installed release* package passed end to end: PUBLISHED plus the public URL, authenticated-list delta of exactly `notes.8tml`, HTTP 200 readback whose only difference from the local source is the site's inserted `style: plot.8ss` line, and restoration verified after the run (IRQ `$0314` = `31ea`, turbo 0, colours 254/246/14, drives and configs byte-identical, scratch byte 827 back to `00`, empty responsive BASIC). The reported truncated message did not reproduce in those screens; 8TML replace/cancel on the candidate package and physical keyboard/mouse remain unverified.
- [ ] Review launcher lifecycle or legacy code removal only after real-machine restoration and caller evidence. Preserve retained diagnostics and use the independent package for release.
