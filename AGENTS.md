# See64You development instructions

## Priorities

1. Verify reported behaviour before changing it.
2. Preserve a useful, responsive editor on real C64/C64U hardware.
3. Keep the C64 client independent from WordPress and the standalone site.
4. Keep published content recoverable from ordinary files.
5. Prefer small, readable solutions over speculative abstractions.

## Architecture

- `client/` is the cc65 C64/C64U application.
- `site/` is a self-contained flat-file PHP site and API.
- `wordpress/` is the WordPress adapter.
- Documents are canonical source text. Generated HTML is never the editing source.
- Network/publisher code must sit behind a narrow interface so offline editing remains usable.
- Reuse generic design lessons from Terra64, but do not couple either repository or copy game-specific code.

## C64 client rules

- Treat 40x25 text mode as the baseline UI.
- Keyboard operation is mandatory; mouse support is optional enhancement.
- Avoid heap allocation in hot editor paths where a fixed buffer is practical.
- Keep editor/model code platform-light so it can be host-tested.
- Avoid full-screen redraws for routine typing.
- C64 keyboard and modern USB keyboard mappings must both remain possible.
- The C64U build may use REU-backed document, clipboard and history storage, but conventional-C64 editing must remain independent of REU.
- On C64U, use temporary turbo acceleration only for verified CPU-bound work that materially improves responsiveness. Preserve and restore the prior turbo state and keep timing-sensitive I/O outside accelerated regions unless hardware testing proves otherwise.

## Web rules

- Standalone public pages must work without JavaScript.
- Plain theme must work without CSS.
- No database is required for the standalone site.
- Validate slugs and document types before constructing paths.
- Escape generated HTML by default. Only the intentionally supported markup subset may create HTML.
- Store password hashes, never plaintext site passwords.
- Reusable passwords/bearer tokens must use HTTPS or the trusted relay outside controlled testing.
- The firmware-1.1 direct path may use explicit plaintext HTTP only with the dedicated one-time challenge/HMAC authentication protocol; do not silently downgrade HTTPS.
- Writes should be atomic where the hosting filesystem permits.

## WordPress rules

- Prefer WordPress-native authentication and permissions.
- Never store a user's normal WordPress password.
- Preserve the See64You source in post metadata so editing round-trips exactly.
- Generated HTML is derived from source and can be replaced on every publish.

## Documentation ownership

- `todo.md` is the **only live work queue**. Put unresolved tasks, deferred work and validation gates there.
- README and subsystem docs describe **implemented behaviour and durable architecture**, not temporary incidents or speculative next steps.
- `docs/LOCAL_TESTING.md` contains only current reproducible test procedures. Completed diagnostic narratives belong in Git history, not the active runbook.
- `docs/ENGINEERING-REFERENCES.md` records external evidence, licences and adoption boundaries; it is not a second TODO list.
- When a code change alters behaviour, interfaces, file names, key mappings, transport, storage, packaging or validation expectations, update the corresponding canonical doc in the same change.
- When hardware/VICE testing resolves a problem, update `todo.md` immediately and remove or rewrite any now-stale diagnostic instructions in the same PR.
- Before committing, explicitly review documentation impact. If no docs change is needed, record why in the PR description/checklist.
- Prefer deleting superseded planning documents once their work is complete; Git history is the archive.

## Workflow

- Read `todo.md` before starting substantial work.
- Inspect current source, branch/PR state and relevant canonical docs before editing.
- Update `todo.md` when work is completed, deferred, discovered or reprioritised.
- Validate the smallest relevant scope first, then run the full available validation required by risk/project rules.
- Keep a downloadable `see64you.prg` for the current integration version: integration validation must build it first and publish it as the `see64you-prg` artifact whenever the C64 build succeeds.
- Do not claim C64U hardware validation unless it actually ran on C64U hardware.
