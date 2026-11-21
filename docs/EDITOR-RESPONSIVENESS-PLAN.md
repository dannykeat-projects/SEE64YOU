# Editor responsiveness — implementation plan

Status: proposed; no performance code changes or new benchmarks are implied by this document. Source baseline: `main` at `92354815bf4e61c902d2190a4a46a4753e8b883d` (2026-11-21). The live tasks and gate status belong in [todo.md](../todo.md); this document defines investigation order and acceptance methods, not a second work queue.

## Objective and boundaries

Make everyday typing, cursor movement and mouse selection feel immediate and **predictably** responsive, including inside populated documents, near the end of the viewport and during word-wrap reflow. Optimise measured end-to-end input-to-visible-screen delay, not just host CPU time or one fast append case. Keep changes small enough to identify regressions and roll back an individual optimisation.

Preserve the single 40×25 C64 Ultimate editor, REU-backed 61,440-byte gap buffer, 16-step history, PETSCII/ASCII-LF conversion, 8TML/8SS/plain-text behaviour, whole-word wrapping, source fidelity, keyboard-first operation, mouse support and existing menus. Save remains explicit: no background autosave. Do not introduce a new editor, document representation, persistent line index, full-screen cache, input scheduler, dependency or turbo policy without measured justification. Maintain source safety, recovery, journals, original system settings and a clean BASIC return.

**Preconditions for any implementation PR:** inspect its current base, `AGENTS.md`, `todo.md`, relevant tests and map files, open PRs, branch/worktree state and unrelated changes. Source and test evidence override the hypotheses below. The five historical branches visible at planning time are not permission to delete or reuse them.

## Evidence and candidate costs

| Area | Verified implementation / earlier evidence | Hypothesis to test, not a measured conclusion |
| --- | --- | --- |
| Overlays and redraw | The shipping overlay-fit generator puts `editor.c` in MODEL and `ui.c` in FILE. `ui.c` calls `s64_editor_has_selection()` while rendering body and status; FILE's generated bridge calls MODEL and restores FILE. The function is invoked even without an active selection. | Avoidable cross-overlay reloads dominate some mouse-drag and mid-edit redraws. Count overlay selects/bytes per event before replacing the boundary. |
| Append typing | `main.c` has a narrow `s64_ui_render_append()` path; that renderer is resident. Earlier REST-assisted C64U testing reported median **78 ms → 46 ms** for the last six of eight appended keys, using a specific candidate and conditions. | The first character after navigation and ordinary middle edits miss this fast path. Previous results are not a fresh baseline for a new package. |
| Mouse selection | `input/input.c` emits drag on crossed 8×8 character-cell boundaries. `main.c` calls `s64_editor_point()` then `s64_ui_render()` for each accepted click/drag. Earlier alpha.8-based ten-step same-line drag test reduced REU reads **610 → 50** while maintaining selection bounds. | Even when text is unchanged, repeated full document-row reads and screen repaint are significant; current physical drag latency is unverified. |
| Model navigation | `editor.c` uses wrapped-row searches and `s64_editor_recalculate_position()`. Backward pointing can invalidate position and rescan from the start of the document; long-line/horizontal navigation scans also exist. | Rescans become costly at 16–60 KiB or with long lines, especially during selection and near viewport boundaries. |
| Middle insertion | `finish_content_edit()` recalculates position and visibility. Most edits in `main.c` end in `s64_ui_render()`; `s64_ui_render_edit()` exists but is not used by the shipping main loop. | A safe dirty-row/partial-render path can help edits that do not change layout. Whole-word wrapping still requires correct downstream reflow. |
| REU and history | `buffer_reu.c:move_gap()` relocates in 64-byte load/store pairs, lazily on mutation. Logical cursor movement itself does not move the gap. `history_reu.c` already coalesces consecutive inserts and uses REU metadata. | The first edit after a distant cursor move can be transfer-bound. Bulk moves/paste need overlap and fault-safety testing; do not undo PR #51/#53 optimisations. |
| Input and storage | `s64_input_read()` polls pointing devices before reading queued keyboard input. Save performs validation and then source output passes in `write_document_file`. | Input scheduling and Save passes might matter; measure them separately instead of assuming they cause typing lag. |

Prior optimisation work: [incremental screen diff PR #13](https://github.com/dannykeat-projects/SEE64YOU/pull/13), [logical cursor PR #51](https://github.com/dannykeat-projects/SEE64YOU/pull/51), [REU history PR #53](https://github.com/dannykeat-projects/SEE64YOU/pull/53) and [wrap/responsiveness PR #78](https://github.com/dannykeat-projects/SEE64YOU/pull/78). Examine their final state and any superseding code before attempting similar changes. **Do not simply restore PR #13's whole-screen cache**: the shipping resident headroom is constrained and its retained-cache cost may exceed the benefit.

## Phase 0 — Reproduce, instrument and establish comparable baselines

1. Confirm actual firmware, PAL/NTSC, C64U model, REU size, turbo/machine settings, SD storage root, keyboard and mouse configuration, package versions and exact Git SHA. Keep the currently installed package unchanged until an authorised, backed-up hardware test.
2. Start with existing `tools/test_editor_performance.py --output client/build/editor-audit/profile.json`, `tools/test_editor_latency_vice.py --rom-dir LOCAL_ROM_DIRECTORY`, `client/diagnostics/ui_probe.c` and `docs/LOCAL_TESTING.md`. Capture currently available REU counts, codec passes, VICE jiffies and UI checkpoints; note which omit overlays and physical input.
3. Add **test-only** bounded counters for code-overlay selects (active vs reload and bytes), REU read/write transactions and bytes, logical-position rescans, row reads/flushes, and per-event timing. Prefer host fixture wrappers or diagnostic builds; do not allocate a production-sized profiler, add unbounded logs or alter input timing. Associate counters with scenarios so counts cannot be misread as latency.
4. Profile the *same shipping UI path* in VICE where feasible using labelled functions, cycle counts and the monitor's `profile on/off/flat/graph` commands. Validate whether the VICE model exercises the same overlays, then use actual C64U for end-to-end latency. Capture source-to-screen timing separately for REST-assisted injection and physical typing/mouse: they are not equivalent.
5. Fixture matrix: new/empty file; 1 KiB, 16 KiB and approximately 60 KiB; short lines, several wrapped lines, single very long line, blank lines, exact 39/40/41-character wrap boundaries, mixed spaces and newline edges. Include all three file kinds where applicable. Use deterministic byte-exact sources, and run the same interaction sequence on baseline and candidate.
6. Interaction matrix: first key after launch/menu; eight successive appends; 20 sustained mid-line inserts (initial gap at EOF); first edit after jumping far forward/back; backspace/delete, overwrite and reflow; cursor/word/page movement; clicking visible text; ten-cell same-line drag in both directions; multi-line drag across viewport and bottom-edge scrolling; mouse release and click outside body; keyboard MARK selection; 64/256/1,024-byte paste, undo/redo; horizontal mode; Open/Save/Save As and recovery. Record source/content hashes where relevant.
7. Store raw results privately under ignored `client/build/` with build hashes and timestamps. Report median, p95 and maximum response, repeat count, variance and transaction counts **per scenario**. Separate cold first event from steady-state events. Hardware end-to-end measurements must be reproducible with explicit event and observation points and account for REST polling granularity.

**Phase gate:** at least one reproducible slow path has baseline counts and end-to-end timings; the overlay-switching hypothesis is confirmed or rejected. If C64U is unavailable, host/VICE diagnostic preparation may proceed but hardware gains stay **unverified**.

## Phase 1 — Eliminate redundant rendering/model overlay transitions (first code candidate)

- Trace actual overlay wrappers from `tools/build_editor_overlay_fit.py`, `client/src/ui.c` and the generated map. Count `FILE → MODEL → FILE` switches separately for no-selection render, selection render, status refresh and cursor-only updates.
- Remove redundant selection accessor crossings on the hot rendering path using the smallest **single authoritative read-only selection-state calculation** available within memory/overlay constraints. Evaluate a lightweight resident helper versus directly reading the existing fields; avoid creating divergent selection semantics. Existing editor API should continue to enforce selection invariants on mutations.
- Re-check generated `BRIDGES`, imports, cross-overlay validation and entry paths. Do not remove a bridge used by a different FILE caller; retain the generated wrapper design and return safety.
- Add automated counts and screen regression for selection off/on, reversed anchor, collapsed selection, MARK mode, undo/reset, menus and modal repaint. Verify correct selected screen cells and status-row labels.

**Accept if:** overlay switches and transfer bytes drop for representative redraws, screen parity is byte-/cell-exact, no degradation is measured on cursor/append, and resident/overlay fit stays legal. Otherwise retain the old path.

## Phase 2 — Make pointer movement and selection incremental

- For a simple click that leaves viewport/text/selection layout unchanged, prefer a cursor-only move plus correct status/old-selection cleanup instead of a 23-row render. Explicitly handle clicks that clear a prior selection.
- For drag selection on an unchanged viewport, update *only* the changed highlight span(s) and cursor/status. Use the existing character-cell event granularity; avoid redrawing identical cells and avoid per-cell REU fetches. Consider direct reverse-bit changes to **text cells only** if measured cheaper, but coordinate with the editor's inverted cursor bit, mouse pointer rendering, PETSCII screen codes and modal invalidation. Conio full-row redraw remains a correct fallback.
- Recompute/repaint on actual viewport shift, word-wrap/horizontal-mode change, content mutation, modal close, mouse-edge auto-scroll if implemented, and any invalid cache/visibility state. Keep release/cancel and off-screen pointer events safe.
- Verify fast forward and reverse drags, selecting empty rows, across wrap boundaries and off-screen edges; drag should not alter saved source. Test mouse + keyboard alternation and status/selection highlight after copy, cut, undo and search.

**Accept if:** same-line and multi-line drag lower rendered cells and end-to-end latency against matched baseline, without stale reverse-video artefacts or missed clicks. Do not implement a generic GUI cache for this.

## Phase 3 — Mid-document edits and correct reflow

- Trace one key through `s64_input_read → editor_insert_value → finish_content_edit → s64_editor_recalculate_position → s64_editor_ensure_cursor_visible → s64_ui_render`. Attribute costs: input, history, gap movement, metadata, layout, overlay and visible rendering.
- Evaluate the existing `s64_ui_render_edit()` facility against shipping constraints. Enable a bounded fast path for a single-character insertion, replacement or deletion **only** when the affected viewport row is known and following rows/word wraps remain unchanged. Keep the full-render path for newline, reflow, selection replacement, viewport shift, modes and ambiguous boundaries.
- Reduce logically redundant position/visibility scans using maintained cursor/line/column state where correctness is provable. Guard 0/EOF, first-body-line Delete, 40-column boundary, tabs/unsupported bytes (still rejected), 8TML header, and long unbroken words. Never change saved LF or word-wrap layout to hide latency.
- Include middle inserts followed by undo/redo, navigation away/back and Save; assert exact source, cursor, status and all 23 rows.

**Accept if:** sustained middle typing and first post-navigation edits improve in both REU/overlay counts and observed latency while wrap and undo are unchanged.

## Phase 4 — Optimise remaining measured model and REU costs

Proceed selectively, in separate PRs, **only for demonstrated hot paths**:

- Replace expensive backward cursor-position rescans and repeated wrapped-row walks with chunked or viewport-local calculations, reusing existing 128-byte scan and 40/41-character wrap logic where possible. A full-document line index is deliberately out of scope.
- Benchmark `buffer_reu.c:move_gap()` at 64 bytes and larger bounded chunks under identical CPU/turbo conditions; the source/destination regions can overlap, so test both movement directions and REU fault injection before considering a chunk change. Do not assume DMA is always the dominant cost.
- Examine `s64_editor_paste`, history swap and coalescing for avoidable per-byte transfers; require all-or-fail/failure-recovery behaviour no worse than documented current partial failure handling.
- Check joystick/mouse polling and keyboard queue responsiveness **only if instrumentation implicates them**. Preserve raw-key modifier behaviour, repeats and native physical keyboard acceptance; do not add sleeps or drop input to make averages look better.
- Keep Save/Open optimisation separate from interactive latency. If Save is proven slow, test the redundant validation/serialization passes and existence checks; do not remove preflight validation, temporary-file atomicity, collision protection or failure recovery merely to reduce scans.

**Accept if:** the changed scenario improves without a larger regression elsewhere, affected REU fault/error tests pass, and fit/stack limits hold. Defer unproven speculative optimisations.

## Performance and correctness gates

**Performance targets (goals, not existing results):** On the same C64U setup and package class, aim to reduce median *and* p95 input-to-screen delay by at least 25% for each targeted slow-path scenario, with no material regression to append, keyboard navigation or other scenarios. Treat any >10% repeatable slowdown or increased worst-case lag as grounds to investigate/revert, not a tradeoff accepted silently. Do not impose an arbitrary universal millisecond threshold until physical baseline is established. If a candidate saves transactions but makes interaction slower, do not ship it.

**Minimum automated gates for each code PR:**
- Narrow host tests for the modified function, plus Editor model, REU buffer/history, input, wrap and UI regressions relevant to the change.
- `sh tools/validate.sh --host-only` for the complete host gate; relevant VICE UI/interaction and overlay tests, including `tools/test_code_overlay_vice.py` and shipped module lifecycle where the boundary changes.
- `make -C client independent-package`; complete SD and forced USB-root build, overlay fit, package manifests, ten-file identity and consistent hashes. Retain Editor resident reserve ≥1,024 bytes (most recent tested candidate reported **1,039 SD / 1,037 USB1 bytes**), plus launcher/Publisher ≥1,024 and all overlay sizes. Historical reserve numbers do **not** certify a new build.
- Compare changed files and generated churn, inspect map/symbol locations and stack/zero-page use, and run failure injection around REU read, paste/undo, save rollback and recovery where affected.

**Physical hardware gate before merge/release of interactive code:** use a backed-up, byte-verified *matching* candidate package; capture root, file list/hash, REU/config, turbo, IRQ, colours, DOS contexts, journal and BASIC state; run paired baseline/candidate exercises for actual C64/USB keyboard and compatible mouse, plus VICE-based screen parity. Check mouse selection and no stuck input at viewport edges and after menus. Verify source/recovery files byte-exact, responsive BASIC and restoration after clean exit. Keep hardware receipts private; never reset an uncertain in-flight write or overwrite user files. REST injection does not replace physical testing, and SD hardware does not certify USB-only operation or cold-power interruption.

**Functional regression matrix:** empty/new, 8TML header and unknown constructs, literal 8SS, general text with explicit suffix; insert/delete/overwrite, whole-word wrap (39/40/41), no-wrap/horizontal scrolling, cursor at EOF and document start, mouse reversed/multiline/empty selection, MARK, search/replace, format-only-8TML commands, undo/redo/history rollover, copy/cut/paste capacity and partial-failure states, menu/help/exit, Save/Save As/Open, .tmp/.bak collision, protected recovery and REU read error. On-screen column/line, colours, cursor and selected cells must remain correct.

**Gate notation:** report **passed / failed / blocked / unverified** separately for host, VICE, SD fit, USB fit, real keyboard, real mouse, persistence, source safety and machine restoration, with exact SHA/firmware and measurements. Never cite a previous commit's pass as proof for the new code.

## Delivery and stop rules

Work as a sequence of small, benchmarked PRs: instrumentation → overlay boundary → pointer/selection → middle edits → optional model/REU/Save paths. Each PR contains a short before/after table with scenario, package hashes, median/p95/max, REU and overlay counts, test gates, memory reserve, affected paths and any user-only hardware checks. Keep the measurement fixture while removing diagnostic-only production code before release. Update this plan only for a material change of direction; record completed and remaining tasks in `todo.md`, and user-facing behaviour in `docs/EDITOR.md` or `docs/LOCAL_TESTING.md` only as warranted. Never merge, deploy, install or publish a performance candidate when required gates fail or remain inappropriately unverified. Do not commit benchmark receipts, user documents, keys, personal network addresses or local paths.

Stop expanding scope when the target interactions improve and all required gates pass. If reduced overlays and targeted redraw already solve the complaint, leave the more invasive buffer/history and turbo changes out. Record any residual latency by scenario rather than making an unverified claim of general responsiveness.

## External engineering references

Research informs the *candidate techniques*, not the claimed performance of this application:

- [cc65 `conio` function reference](https://cc65.github.io/doc/funcref.html) — `cputc`, `cputs`, cursor/reverse behaviour; validate any direct screen RAM approach against PETSCII and the actual UI.
- [VICE monitor documentation, profiling commands](https://vice-emu.sourceforge.io/vice_12.html) — `profile on/off/flat/graph`, labels, breakpoints and cycle-oriented inspection.
- [Commodore REU access and DMA register example](https://codebase.c64.org/doku.php?id=base:reu_read_and_write) — block-transfer mechanics; benchmark application-specific batching and avoid overlap assumptions.
- [Ultimate turbo mode documentation](https://1541u-documentation.readthedocs.io/en/latest/config/turbo_mode.html) — turbo/device/badline constraints; preserve settings and treat acceleration as optional, measured and reversible.
