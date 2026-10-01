# Setup and configuration UX

This document defines the user-facing setup flow for the C64 Ultimate-only See64You architecture. Implementation work is tracked in todo.md.

## UX goal

A new user should be able to:

1. copy the See64You package to SD;
2. launch one PRG;
3. follow only the hardware instructions that cannot be automated;
4. enter/import website details;
5. test the connection;
6. begin editing.

They should not need to understand UCI targets, REU banks, module caches, DMA transfers or companion PRGs.

Subsequent launches should normally open directly into the editor.

## First launch: platform check

The launcher performs bounded checks before asking publishing questions.

User-facing results should look like:

~~~text
C64 ULTIMATE      READY
COMMAND INTERFACE READY
MEMORY 16 MB      READY
STORAGE           READY
NETWORK           READY
~~~

Technical target numbers belong behind Diagnostics.

Required checks include:

- C64U/UCI availability;
- Ultimate-DOS;
- 16 MiB REU;
- writable/readable application storage;
- Network $03 for direct publishing;
- optional Control/turbo capabilities.

The current C64U editor enforces the exact 16 MiB REU requirement with a deliberately tiny guard. A separate platform-preflight diagnostic exercises UCI, Ultimate-DOS $01, exact 16 MiB REU and writable/readable /sd/ storage with R = RECHECK / X = EXIT so this logic can be hardware-validated without consuming the editor's narrow conventional-RAM safety margin. Its storage check selects an unused temporary filename, writes a fixed marker, reopens and verifies it, then deletes it and verifies absence; existing candidate files are never intentionally overwritten. The final launcher/supervisor will own that full preflight. Launcher integration and real-hardware acceptance remain P0 migration work.

Offline editing should remain available if networking is unavailable.

## Missing machine prerequisite

If See64You cannot enable a prerequisite itself, explain the exact one-time action.

Example:

~~~text
16 MB REU REQUIRED

OPEN THE ULTIMATE MENU
F2 SETTINGS
C64 AND CARTRIDGE SETTINGS
RAM EXPANSION UNIT: ENABLED
REU SIZE: 16 MB

RETURN HERE AND CHOOSE RECHECK
~~~

Do not send the user back to the beginning after changing one setting.

If UCI itself is disabled, See64You cannot use UCI to enable it; guided manual configuration is therefore expected for that bootstrap case.

## Automatic runtime configuration

Once prerequisites exist, See64You may temporarily change runtime state to improve operation.

Normal users should not be asked about temporary turbo modes, REU cache layout or DOS target selection.

The system-state manager must capture prior values, apply temporary settings only where needed and restore them on clean exit.

Setup must clearly distinguish:

- **required persistent machine prerequisites** the user chooses once;
- **temporary See64You runtime changes** that are restored automatically.

## Publishing setup wizard

### Step 1 — Website

Prompt for the See64You site URL.

For the firmware-1.1 direct Network $03 path, explain briefly that direct HTTP does not encrypt the document body. Require explicit consent. Never silently downgrade an HTTPS URL.

### Step 2 — Publishing key

Prefer IMPORT SEE64KEY.TXT.

The wizard establishes its own deterministic storage path and looks for the standard key file. Manual entry remains a fallback.

The normal site password must not be requested by the direct C64U wizard.

### Step 3 — Connection test

Validate syntax and credentials, then perform the actual direct connection/authentication test.

Distinguish:

- network unavailable;
- site unreachable;
- authentication rejected;
- invalid URL;
- unsupported secure transport.

### Step 4 — Save and verify

Persistent configuration is not considered saved until:

1. in-memory values validate;
2. transactional write/replace completes;
3. the final file is reopened;
4. checksum/fields validate;
5. required values match;
6. the verified values become active session state.

### Step 5 — Complete

Example:

~~~text
SEE64YOU IS READY

WEBSITE: example.com
CONNECTION: OK
SETTINGS SAVED: OK

START EDITING
~~~

Do not display the secret key.

## Offline/editor-only path

Publishing setup must never trap the user.

Provide EDIT OFFLINE / SET UP LATER wherever network configuration is optional.

A missing/corrupt publishing configuration is different from a hardware prerequisite needed by the C64U runtime itself.

## Normal Settings screen

After setup, Basic Settings should contain only user-meaningful choices:

- Website/profile;
- Connection status / TEST CONNECTION;
- Publishing key configured/not configured / REPLACE KEY;
- Default page/post behaviour;
- Autosave/recovery preference;
- visual/editor preferences that belong to See64You;
- RUN SETUP AGAIN;
- ADVANCED SETTINGS.

Do not expose UCI target IDs in Basic Settings.

## Advanced Settings

Advanced contains:

- profiles/destinations;
- transport selection when multiple validated transports exist;
- explicit plaintext-HTTP policy;
- relay settings;
- WordPress settings when enabled;
- storage locations;
- performance policy;
- cache/module diagnostics;
- recovery management;
- hardware capability details;
- logs/diagnostic export.

Security-sensitive downgrades require explicit confirmation.

## Application installation/layout

The intended package eventually presents a simple /sd/SEE64YOU/ folder.

Normal users should launch SEE64YOU.PRG; internal Editor/Publisher/Settings modules are implementation details.

The launcher validates module versions/integrity before caching them in REU. Users should not have to keep track of matching S64EDIT/S64PUB/S64CFG files manually once the new package format lands.

## Transition UX

Editor, Publisher and Settings remain executable overlays but appear as one application.

Every transition clears/replaces stale UI immediately and may briefly show:

- OPENING PUBLISHER...
- OPENING SETTINGS...
- RETURNING TO EDITOR...

Normal transitions use validated REU-cached modules, not software IEC.

## Persistence and migration

Persistent settings are durable SD state. REU is fast working/cache state, not the sole durable copy.

The current /sd/SEE64YOU.CFG has passed real-hardware cold persistence testing. Do not move it merely for folder neatness. Migration to the final application directory must write, reopen and verify the new copy before retiring the old location.

## Error handling

Errors should state the problem and next useful action.

Prefer:

**NETWORK IS NOT AVAILABLE — EDIT OFFLINE OR RETRY**

over:

**UCI ERROR 82**

Diagnostics may show the underlying status separately.

Useful actions include:

- RETRY;
- RECHECK HARDWARE;
- EDIT OFFLINE;
- ADVANCED;
- DIAGNOSTICS;
- RESTORE LAST GOOD SETTINGS.

## First-run acceptance

A new supported C64U user can:

- satisfy the one-time UCI/16 MiB prerequisites without developer knowledge;
- launch a single See64You entry point;
- import/enter site information;
- save and verify configuration;
- power cycle and reload configuration;
- edit while offline;
- publish when networking is available;
- exit with temporary machine settings restored.
