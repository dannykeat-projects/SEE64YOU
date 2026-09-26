# Editor responsiveness repair plan

Status: planning only, 2026-09-26. No implementation or runtime testing performed for this plan.

## Objective and starting point

Make ordinary typing, cursor movement and editing usable at normal C64 speed throughout the existing 4 KiB document capacity. Preserve the working cursor, capitals, keyboard mappings, document format, selection, undo and recovery. Use C64U-specific resources only if measurements establish a need.

Baseline: commit `173c82d` on `integration-validation-2026-09-24`. The previously built `client/build/see64you.prg` has SHA-256 `C945E05B53B4477EB1661689645CE027C3C0F6B89B9478CF7788829CE4F793ED`. Verify the build under test when implementation resumes; do not assume a PRG matches the checkout.

User results for this checkpoint:

- Blinking cursor visible; text wraps; Left/Right and Up/Down work.
- Backspace and insertion within existing text work functionally.
- Typing starts responsive but becomes essentially unusable by the second line.
- Page Up may behave strangely, but latency prevented a reliable assessment.
- Performance remains failed despite the prior successful host tests and self-hosted integration workflow. Real C64U validation remains outstanding.

## Evidence from the current source

| Area | What is established | What still needs measurement |
| --- | --- | --- |
| `client/src/main.c` | End-of-document character insertion should call `s64_ui_render_append` when wrap is enabled and the viewport does not change. | Whether the failing runtime actually follows that path; full-render frequency. |
| `client/src/editor.c`, `buffer.c`, `history.c` | Ordinary append updates the gap buffer, cursor coordinates and coalesced history without scanning existing text. Scrolling and other edits take different paths. | Unexpected fallback, corrupted state, or expensive generated code. |
| `client/src/ui.c` | Append writes one character, formats the status with `sprintf`, compares 40 status cells, and positions the cursor. Crossing a row also clears the next row. | Cost of status formatting/output and row transitions; actual screen-write count. |
| `client/src/input/input.c` | Blocking `cgetc` is surrounded by cursor enable/disable. | Input buffering, repeat rate, and cursor/conio/KERNAL interactions at a wrap boundary. |
| `client/src/main.c` recovery | `maybe_autosave` runs before reading input; it returns immediately without DOS capability, otherwise may synchronously write recovery. | Whether DOS is detected on the test setup and whether stalls coincide with recovery writes. |

The source does not establish a cause for the new report. In particular, the existing append path does not explain a simple linear slowdown with document length. A constant but excessive processing cost can also accumulate a keyboard backlog and appear to worsen with text length. Treat both explanations as hypotheses.

The current local linker map ends BSS at `$CBAB`; with the existing 1 KiB stack reservation beginning at `$CC00`, only 84 bytes remain before that reservation. This is a static layout observation, not proof of safe runtime stack depth. Avoid adding large buffers or reducing the stack/document/clipboard/history capacities to make an unmeasured solution fit.

## 1. Reproduce and measure before changing behavior

Use the existing WSL/VICE tools and self-hosted runner. Record VICE version, PAL/NTSC, emulation speed, keyboard mapping, PRG hash and DOS capability. Establish a normal-speed baseline with warp/turbo disabled. The user retains practical interactive testing; the implementation agent handles targeted instrumentation, measurements and automated checks.

Compare these cases with identical input:

1. Slow individual keypresses versus continuous typing/key repeat. Stop typing and measure how long queued characters continue appearing.
2. A continuous wrapped line versus short lines separated by Return. Check positions around 39/40/41 and 79/80/81 characters; also 255/256/257 for the undo transaction boundary.
3. Append at approximately 40, 80, 400, 920, 2,000 and 3,900 characters. Separate cases that scroll from those that do not.
4. At the same document length, compare append, Left/Right, Up/Down, Backspace and insertion in the middle. Check whether the delay resets after the input queue drains.

Measure emulated cycles or elapsed frames from key consumption to the visible update and to readiness for the next key. Exclude time spent waiting for a key in `cgetc`. Prefer monitor breakpoints/cycle counts, or small temporary diagnostic markers, over logging on every key. Separate editor/history, status formatting/output, body rendering, and recovery work. Count append versus full-render calls and body cells written.

Use these observations to identify the dominant delay. If timing is inconsistent, check stack high-water use and state integrity, especially around the first wrap. Inspect the installed cc65 conio routines if the boundary implicates cursor or screen-line bookkeeping. Do not rewrite those routines based on suspicion alone.

Deliverable: a short before-measurement table and a verified explanation of the failing path. Stop broad investigation once that evidence is sufficient.

## 2. Apply the smallest measured fix

Choose only the branch supported by phase 1; these are alternatives, not a list of changes to implement together.

- **Status dominates:** update changed status fields and use a small bounded number formatter where it removes measured overhead. Avoid rebuilding the entire status string on every ordinary key. Keep current messages, mode indicators and cursor positioning correct.
- **Unexpected full render or document scan:** repair the predicate/state transition that bypasses append handling. Preserve the existing fast append path and prove that ordinary non-scrolling append does not visit existing document text.
- **Screen/conio work dominates:** first bound work to the affected cells/rows. Consider a compact C64-specific screen-write routine only if measured necessary. It must handle native-character to screen-code conversion, reverse selection, colour and cursor restoration correctly.
- **Input or wrap-boundary interaction dominates:** repair the verified cursor, input or screen-line bookkeeping issue while retaining capitals, shifted cursor keys and blinking. Do not substitute a custom keyboard scanner without evidence that the current mechanism is the bottleneck.
- **Recovery dominates:** confirm the correlation with DOS/recovery first; then bound or defer the expensive operation appropriately. Preserve recoverability and avoid silently disabling autosave as a production fix.
- **Runtime corruption:** resolve the verified bounds, stack or cursor-state error before optimizing its symptoms.

After the first correction, repeat the same measurements. Optimize another area only if it still prevents usable editing. Scrolling and edits that reflow text may need more work than a single append, but repeated keystrokes must not build an ever-growing queue. Page movement currently repeats row movement; measure its repeated recalculation cost if navigation remains slow.

## 3. Resolve the uncertain navigation behavior

Once latency permits meaningful testing, reproduce Page Up/Page Down using the implemented C64 mapping: Commodore + cursor Up/Down (verify the host-to-C64 mapping in VICE). A PC Page Up key is not by itself proof that the editor received its page command.

Check visual row boundaries, especially 39/40/41-column lines, exact-width lines followed by Return, blank lines, the bottom visible row, and returning to previously scrolled text. Compare cached cursor coordinates, rendered cursor position and document position after movement, editing and undo. Fix only reproduced failures, with focused host regression cases for the model and a runtime check for rendering/input.

## 4. Validation and completion gates

Proposed acceptance targets at normal emulation speed:

- Ordinary non-scrolling append should normally complete within two frames; interactive movement and local edits should remain under roughly 100 ms. Record actual numbers and explain any exception rather than declaring success from a build.
- Sustained normal typing should show no accumulating delay or lost/reordered characters. Non-scrolling append cost should stay approximately flat as the document grows.
- Both screenfuls remain reachable; scrolling/page operations preserve content and cursor position. No stale selection, stray characters or new flicker in routine typing.
- No document corruption or changed save/undo behavior; the final release build fits with a checked stack margin.

Run the focused editor tests first, then the repository-supported full validation and cc65 build. On Windows-mounted files, account for CRLF shell scripts; use the clean Linux CI checkout or temporary normalized scripts rather than unrelated tracked line-ending changes. The known full command is `sh ./tools/validate.sh` in a Linux checkout; the PRG build is `make -C client clean all`.

Keep the existing private self-hosted workflow on `terra64-wsl`. It already builds and publishes `see64you-prg` before full validation. Do not install packages in workflow jobs. Remove temporary profiling instrumentation, recheck release timings and the linker map, review the final diff, and identify the final PRG by commit and hash.

User retest, in order:

1. Cursor and Shift/capitals; type past the second line, then through two screenfuls.
2. Stop after rapid typing: the display should catch up immediately, without a long backlog.
3. Repeat at a nearly full document; compare append, arrows, Backspace and middle insertion.
4. Page back to earlier text and forward again; check wrap boundaries and Return.
5. Brief selection, copy/paste, undo/redo and Save/Open smoke checks. Test recovery separately where DOS is available.

Record VICE and real C64U results separately. Automated correctness tests alone do not close the responsiveness task.

## C64U decision

Start with the existing conventional-C64 path. There is no measured evidence yet that this editor requires additional processing power. The repository's planned REU storage addresses document/clipboard/history capacity; it is not an established cure for per-key latency.

If the measured baseline still misses the targets after the narrow repair, verify the actual C64U model, firmware and available documented acceleration or offload interfaces before proposing their use. Keep any supported enhancement optional and preserve usable basic editing without it. A higher-speed mode may be a diagnostic comparison, but does not establish that the underlying defect is fixed. Do not begin REU migration, offload infrastructure or a second editor architecture within this repair.

## Handoff for the next session

Read this plan, `todo.md`, current Git state and the named source functions. Start at phase 1 with the exact build; avoid repeating another speculative renderer rewrite. No code changes, builds, new tests, commits or pushes were made for this planning request. When implementation is authorized, complete the measured fix, validation and user handoff; keep this document as the single repair plan and `todo.md` as the work queue.
