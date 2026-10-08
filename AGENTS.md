# See64You development instructions

## Product baseline

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

## MVP-first execution

The ordered acceptance plan in todo.md governs current work. Finish the persistent-runtime MVP and demonstrate its complete user workflow on real hardware before expanding features or performing broad cleanup. Keep one live queue; do not create competing plans.

Retain the proven Launcher → Editor bootstrap and BASIC-return path. Within the candidate application, Editor owns one runtime/lifecycle while Publisher and Settings are callable cached overlays. Keep the whole-PRG fallback until replacement hardware acceptance; remove only proven-unused dependencies afterward.

Use existing services and small explicit boundaries. Do not weaken memory/integrity/restore guards, add speculative abstractions, rewrite persisted formats or undertake another architecture pivot merely to fit deferred scope. Report user-visible progress and exact validation scope.

## C64U runtime rules

- 40x25 text mode remains the baseline application UI.
- Keyboard operation is mandatory; mouse support is an enhancement.
- Modern USB keyboard users and the machine's native C64 keyboard are both first-class input targets.
- The 16 MiB REU is the primary extended workspace. Introduce a central allocator rather than scattering fixed ad-hoc banks.
- Keep only small hot buffers and currently executing code in conventional C64 RAM when REU-backed storage is more appropriate.
- Prefer Ultimate-DOS and native UCI transfers over IEC/KERNAL file loading.
- Use Ultimate-DOS targets $01 and $02 intentionally as independent filesystem contexts when that simplifies state.
- Validate and cache executable assets in REU. Preserve the trusted initial Launcher → Editor transfer; persistent candidate mode switches call overlays under the same owner rather than restarting whole PRGs.
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
- Evolve document/history/clipboard storage toward the shared REU allocator.
- Large-document design should use paging/pieces/chunks rather than repeatedly shifting an enormous flat REU region.
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

## WordPress rules

- Prefer WordPress-native authentication and permissions.
- Never store a user's normal WordPress password.
- Preserve See64You source in post metadata so editing round-trips exactly.
- Generated HTML is derived from source and can be replaced on every publish.

## Documentation ownership

- todo.md is the **only live work queue**.
- README and subsystem docs describe durable architecture, current contracts and explicitly labelled migration state.
- docs/C64U-RUNTIME.md owns hardware/resource/system-state architecture.
- docs/LOCAL_TESTING.md contains only current reproducible validation procedures.
- docs/ENGINEERING-REFERENCES.md records external evidence, licences and adoption boundaries.
- When behaviour, interfaces, package names, key mappings, transport, storage, resource ownership, setup or validation expectations change, update the corresponding canonical doc in the same change.
- Remove superseded plans rather than leaving contradictory active documentation; Git history is the archive.

## Workflow

- Read todo.md before substantial work.
- Inspect current branch/PR state, source and canonical docs before editing.
- Re-check the branch head immediately before a write because local/Codex work may advance it concurrently.
- Update todo.md when work is completed, deferred, discovered or reprioritised.
- Validate the smallest relevant scope first, then the full available validation required by risk.
- The primary downloadable product artifact is the **current C64U package**, not a conventional-C64 PRG.
- Do not merge/main/deploy solely because CI is green; follow the explicit delivery instruction.
- Do not claim real-hardware validation unless it actually ran on real C64U hardware.
