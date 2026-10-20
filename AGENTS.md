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
- New documents are canonical ASCII/LF 8TML 1 (.8tml) source. No .s64 compatibility work or new .s64 documents. Generated HTML is never the editing source. Preserve unknown 8TML metadata and markup on round trips.
- Network/publisher code must sit behind a narrow interface so offline editing remains usable.
- Reuse generic design lessons from Terra64, but do not couple either repository or copy game-specific code.

## C64 client rules

- Treat 40x25 text mode as the baseline UI.
- Keyboard operation is mandatory; mouse support is optional enhancement.
- Avoid heap allocation in hot editor paths where a fixed buffer is practical.
- Keep editor/model code platform-light so it can be host-tested.
- Avoid full-screen redraws for every keystroke once incremental rendering is available.
- C64 keyboard and modern USB keyboard mappings must both remain possible.
- REU-backed large-document storage is a planned enhancement; do not make basic editing depend on it.

## Web rules

- Standalone public pages must work without JavaScript.
- Plain theme must work without CSS.
- No database is required for the standalone site.
- Validate slugs and document types before constructing paths.
- Escape generated HTML by default. Only the intentionally supported markup subset may create HTML.
- Store password hashes, never plaintext site passwords.
- Authentication tokens must expire and should only be used over HTTPS in production.
- Writes should be atomic where the hosting filesystem permits.

## WordPress rules

- Prefer WordPress-native authentication and permissions.
- Never store a user's normal WordPress password.
- Preserve the See64You source in post metadata so editing round-trips exactly.
- Generated HTML is derived from source and can be replaced on every publish.

## Workflow

- Read `todo.md` before starting substantial work.
- Update `todo.md` when work is completed, deferred, discovered, or reprioritised.
- Keep documentation concise and authoritative; do not create duplicate planning files.
- Validate the smallest relevant scope first, then run the full available validation.
- Do not claim C64U hardware validation unless it actually ran on C64U hardware.

## 8TML migration

- Follow `docs/8TML-MIGRATION.md` and `todo.md`; the externally supplied 2026-10-20 8TML 1 draft governs format behavior.
- Do not copy the externally supplied reference implementation until its licence is clarified.
- Reconcile actual maintained C64U builds/branches before modifying save/open. Do not assume the default-branch placeholder is the latest working editor.
- Require ASCII/LF byte-level tests and independent validator output before declaring a new document compatible; hardware confirmation is a separate gate.
