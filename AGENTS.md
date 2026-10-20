# See64You development instructions

## Product baseline

The approved 8TML migration in `docs/8TML-MIGRATION.md` supersedes the historical `.s64` format references below. Independent Editor persistence now uses complete ASCII/LF `.8tml` source, including editable headers and unknown markup. No legacy importer or fallback. Publisher/Browser conversion is a later milestone; preserve their existing implementations until then.

Storage may be SD or USB: the complete build selects `STORAGE_ROOT` (default `/sd/`) and derives all package/recovery paths in `platform/package_paths.h`. Treat SD-specific paths below as defaults, not an SD-card prerequisite. Force a complete rebuild when changing the root; never mix packages or fall back to another device's config/journals automatically.

See64You is a **C64 Ultimate-only** application. The supported product architecture requires a **16 MiB REU** and deliberately uses Ultimate-specific facilities.

Do not constrain product design to preserve ordinary-C64 compatibility, 64 KiB-only operation, software-IEC loading, original-drive timing or an original-C64 release artifact. Conventional-C64 code may remain temporarily during migration only where it still helps regression testing.

Read docs/C64U-RUNTIME.md before platform/runtime work.

## Priorities

1. Verify reported behaviour before changing it.
2. Preserve a useful, responsive editor on real C64 Ultimate hardware.
3. Use C64U facilities aggressively where they improve capability, reliability or responsiveness.
4. Keep resource use efficient even though more resources are available.
5. Hide UCI/REU/DMA/transport complexity from normal users.
6. Restore every temporary machine setting See64You changes when a clean exit is possible.
7. Keep the editor independent from publishing backends and useful offline.
8. Keep canonical content recoverable from ordinary files.
9. Prefer small, measured solutions over speculative abstractions.

## 8TML-first product direction (approved 2026-10-20)

Read [docs/8TML-ROADMAP.md](docs/8TML-ROADMAP.md) and [todo.md](todo.md) before design work. **8TML is the long-term canonical publishing format even if T8WN stops.** The Editor must work offline with independently readable files and independent hosting. The existing alpha remains `.s64`/SEE64YOU/1, not 8TML. Pin an authoritative 8TML version, encoding rules and fixtures before implementing parsing. There are no old `.s64` documents to preserve; **never implement `.s64` import/export/conversion, dual-format parsing or legacy compatibility tests**. Historical tag/assets and their accurate guide remain intact; new builds owe that format no support. Do not couple the core editor to VVV, any third-party API or PHP.

Format conformance and source-to-HTML hosting are the next prioritised milestones, replacing the earlier prohibition on adding formats. Publisher integration is separate from static file serving and requires secure, portable authentication. Legacy website/browser/WordPress features are frozen and candidates for removal from the new active build once no longer called. Code retirement requires caller tracing, affected tests and applicable real-C64U gates, but **no old-format migration gate**.

## Legacy alpha MVP and stop rules

The **historical shipped alpha scope** is the independent Editor (`SEE64YOU.PRG`), Publisher (`S64PUB.PRG`), Settings (`S64CFG.PRG`) and optional read-only public Browser (`S64WEB.PRG`), plus the existing standalone flat-file PHP site. Editor saves/reopens canonical `.s64` files; Publisher loads/publishes and downloads to a new `.s64`; Settings owns disk config; Browser follows public SEE64YOU text/link pages. Each program launches independently and returns to BASIC. No cross-program live state, history, clipboard, or REU handoff.

The following describes historical alpha behaviour, not new-format compatibility requirements. Preserve proven Launcher→Editor, file safety/recovery, BASIC return, state restoration and resource guards wherever reused. Old document/browser/site protocols may be removed once not called by the new active build. Validate affected paths and hardware replacements; retain only still-needed overlays.

Do not expand WordPress, general HTML browsing, ordinary-C64 support, joystick/mouse, speculative TLS or unrelated features. **Native 8TML-only Editor work is now authorised.** Replace obsolete `.s64` document handling and old-only integrations once safely decoupled and tested, without supporting old formats.

Stop if a fix needs another major cross-program runtime/loader architecture. Two bounded attempts at the same blocker without progress trigger go/no-go reassessment, potentially shelving the project rather than expanding it. Fix existing correctness problems only with the smallest validated changes. Don't weaken fit, disk-integrity or machine-restoration gates.
## C64U runtime rules

- 40x25 text mode remains the baseline application UI.
- Keyboard operation is mandatory; mouse support is an enhancement.
- The native C64 keyboard is the MVP acceptance input. Existing USB-keyboard support must not regress, but new USB acceptance work is deferred until feedback.
- The 16 MiB REU is the primary extended workspace. Introduce a central allocator rather than scattering fixed ad-hoc banks.
- Keep only small hot buffers and currently executing code in conventional C64 RAM when REU-backed storage is more appropriate.
- Prefer Ultimate-DOS and native UCI transfers over IEC/KERNAL file loading.
- Use Ultimate-DOS targets $01 and $02 intentionally as independent filesystem contexts when that simplifies state.
- Validate executable assets in REU where useful. Preserve trusted initial Launcher → Editor transfer and clean BASIC return. Publisher and Settings run independently without relying on cached state or Editor-owned lifecycle.
- Do not resurrect the $C800-$CBFF resident-loader reservation as a production constraint unless later evidence makes it necessary.
- Capability-probe hardware features. Do not enable a feature solely because a firmware version is expected to have it.
- Real C64U hardware is the final authority for UCI, DMA/REU, turbo, USB-keyboard and timing behaviour.

## System-state ownership

All temporary machine changes must go through a common system-state manager.

For any setting See64You changes:

1. read/capture the prior value when possible;
2. persist enough session state to recover from an interrupted run where practical;
3. apply the temporary setting;
4. use it only for the necessary scope;
5. restore the prior value on clean exit/module teardown;
6. verify restoration where the interface permits.

Examples include turbo/badline mode, runtime palette if ever changed, interrupt/memory-map state around transfers, temporary DOS contexts and network handles.

Do not automatically rewrite unrelated persistent Ultimate configuration such as SID/audio, HDMI preferences, drive configuration, home directory, DHCP/static network settings or palette merely because the API can reach them.

If a prerequisite cannot safely be enabled by software, guide the user through the exact one-time Ultimate menu setting and recheck it.

## Performance rules

- Do not run permanently at maximum turbo merely because it exists.
- Define central modes such as interactive, CPU-intensive and I/O-safe.
- Use the fastest hardware-proven mode for CPU-heavy work.
- Restore the user's prior turbo state after temporary acceleration.
- Keep timing-sensitive I/O at a proven-safe mode until hardware testing demonstrates faster operation is reliable.
- Continue incremental screen redraws and streaming data paths; extra hardware is not permission for unnecessary copying.

## Editor/model rules

- Keep editor/model logic platform-light enough for host tests.
- Avoid heap allocation in hot paths when bounded structures are simpler, but do not preserve tiny original-C64 limits that no longer serve the product.
- Preserve current REU-backed editing/document behaviour; new allocator, large-document and history work is deferred beyond the MVP.
- Generated HTML is never the editing source.

## Web rules

- Standalone public pages must work without JavaScript.
- Plain theme must work without CSS.
- The standalone site requires no database.
- Validate slugs and document types before constructing paths.
- Escape generated HTML by default; only the frozen supported markup subset may create HTML.
- Store password hashes, never plaintext site passwords.
- Reusable passwords/bearer tokens require HTTPS or the trusted relay outside controlled testing.
- The firmware-1.1 direct path may use explicit plaintext HTTP only with the dedicated challenge/HMAC protocol; never silently downgrade HTTPS.
- Writes should be transactional/atomic where the storage environment permits.

## Deferred integrations

The existing WordPress adapter and trusted relay are outside the initial release acceptance scope. Do not expand or refactor these integrations just for completeness. Preserve credential safety and established on-disk formats if an MVP fix touches common code.
## Documentation ownership

- todo.md is the **only live work queue**; historical MVP work is archived under docs/archive and not active.
- README and subsystem docs distinguish active 8TML-only architecture from historically released alpha; no old-document migration is planned.
- docs/C64U-RUNTIME.md owns hardware/resource/system-state architecture.
- docs/LOCAL_TESTING.md contains only current reproducible validation procedures.
- docs/ENGINEERING-REFERENCES.md records external evidence, licences and adoption boundaries.
- When behaviour, interfaces, package names, key mappings, transport, storage, resource ownership, setup or validation expectations change, update the corresponding canonical doc in the same change.
- Keep README, architecture and current validation procedures concise. Retire contradictory managed-runtime prose; Git history preserves obsolete experiments. Do not discard unique recovery or validation instructions before checking their active callers.

## Workflow

- Read todo.md before substantial work.
- Inspect current branch/PR state, source and canonical docs before editing.
- Re-check the branch head immediately before a write because local/Codex work may advance it concurrently.
- Update todo.md when work is completed, deferred, discovered or reprioritised.
- Validate the smallest relevant scope first, then the full available validation required by risk.
- The existing alpha target is its version-matched eleven-file independent package plus legacy PHP website. Future 8TML-only packages follow staged implementation gates, never overwriting historical alpha assets. Managed runtime targets remain historical fallbacks, not release outputs.
- Do not merge, deploy, replace a public release/tag/assets or touch existing SD data solely because CI is green; follow explicit delivery authorization.
- Do not claim real-hardware validation unless it actually ran on real C64U hardware.
