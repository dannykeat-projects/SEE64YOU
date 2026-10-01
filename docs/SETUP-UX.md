# Setup and configuration UX

This document defines the intended user-facing setup flow for See64You. It is a product/UX specification; unresolved implementation work is tracked in `todo.md`.

## Goals

A new C64 Ultimate user should be able to go from a fresh See64You site and fresh C64U package to a working publisher without understanding UCI targets, REU handoff, relay keys, profiles or firmware transport details.

The normal path should ask only for information the user actually needs. Advanced and diagnostic controls remain available, but should not dominate the first-run experience.

## First-run detection

On entry to Publisher, attempt to load and validate the persistent publishing configuration. If no valid configuration exists, offer:

1. **SET UP WEBSITE** — start the guided setup;
2. **EDITOR ONLY** — return to editing without publishing;
3. **ADVANCED** — open the full configuration screen.

A missing or corrupt configuration must never prevent offline editing. A corrupt configuration should be reported distinctly from a configuration that has never existed.

## Guided setup: stock C64 Ultimate / Standalone

The supported stock-firmware path should be the default and require as few decisions as possible.

### Step 1 — Website address

Prompt for the See64You site URL. Validate its syntax immediately. For firmware 1.1 direct Network `$03`, explain briefly that the C64U connection is direct HTTP and the document body is not encrypted. Require explicit consent before enabling plaintext HTTP; do not silently downgrade an HTTPS URL.

### Step 2 — Publishing key

Prefer **IMPORT SEE64KEY.TXT**. The wizard should establish its own storage path and look for the standard key file without requiring the user to browse there first. Offer **TYPE KEY MANUALLY** as a fallback, not as the primary path.

The site pairing page remains the place where the dedicated direct key is provisioned. The normal site password must not be requested by the stock-firmware direct wizard.

### Step 3 — Verify and save

Validate the URL/key combination and test the direct connection. If the connection succeeds, write the configuration transactionally to persistent storage.

Do not report setup complete merely because the write call returned success. The helper must immediately reopen the persisted configuration, validate its checksum/fields and compare the required values with what was just saved. A failed readback is a setup failure with a clear retry/diagnostics path.

### Step 4 — Complete

Show a compact success screen such as:

```
SEE64YOU IS READY

WEBSITE: example.com
CONNECTION: OK
SETTINGS SAVED: OK

RETURN TO EDITOR
```

Do not expose the direct key itself on this screen.

## Normal settings after setup

The ordinary **Publishing Settings** screen should be a simple status/configuration page rather than the current engineering-style list.

Basic controls:

- Website — show the configured site and allow changing it through the wizard;
- Connection — show configured/not configured and provide **TEST CONNECTION**;
- Publishing key — show configured/not configured and provide **REPLACE KEY**;
- **RUN SETUP AGAIN**;
- **ADVANCED SETTINGS**;
- **SAVE / RETURN** where unsaved basic changes exist.

The user should not normally need to choose a destination type when the installed build only exposes the supported Standalone direct path.

## Advanced settings

Move implementation-specific or uncommon controls here:

- destination/backend selection when multiple supported backends are actually available;
- explicit plaintext-HTTP control;
- manual direct-key entry/disable;
- relay configuration;
- destination profiles;
- WordPress configuration when that backend is enabled;
- transport/storage diagnostics and troubleshooting information.

Advanced settings must not weaken security defaults silently. Changes that affect transport security should require explicit confirmation.

## Error handling

Errors should say what failed and what the user can do next. Distinguish at least:

- site URL invalid;
- key missing/invalid;
- network target unavailable;
- connection/authentication failed;
- settings storage unavailable;
- settings write failed;
- settings readback/verification failed;
- saved configuration corrupt on later startup.

Where useful, provide **RETRY**, **ADVANCED/DIAGNOSTICS**, and **EDITOR ONLY** rather than leaving the user at a dead end.

## Persistence contract

Publishing configuration is persistent state. REU is only a handoff/cache mechanism and must never be treated as durable storage.

A successful setup/save means all of the following are true:

1. configuration values validate in memory;
2. the persistent file write/replace completes;
3. the saved file is reopened from storage;
4. checksum and required values validate on readback;
5. the active session receives the verified settings.

Real-hardware acceptance additionally requires a full C64U power cycle followed by automatic settings reload and a successful connection test.

## Companion transitions

Editor, Publisher and Settings remain separate PRGs for memory headroom. Every transition should immediately clear the previous screen and display an explicit message such as `LOADING PUBLISHER...`, `LOADING SETTINGS...` or `RETURNING TO EDITOR...` until the destination UI is ready. This avoids slow software-IEC loads appearing to be corruption or a hang.

The planned Ultimate-DOS/SD-aware companion loader should improve transition speed and remove the software-IEC device-8 dependency without changing the wizard's user-facing model.

## Acceptance criteria

A first-time user on the supported C64U path can complete setup with only a site URL, the standard direct-key file and explicit HTTP consent; no UCI/DOS terminology is required. Setup survives a full power cycle. Re-running setup can replace a site/key cleanly. Failure at any stage preserves the last known-good persistent configuration. Offline editing remains available throughout.
