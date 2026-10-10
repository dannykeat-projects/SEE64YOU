# Setup and first-publication UX

The current product uses three independent programs. Settings saves the canonical disk configuration; Editor saves canonical `.s64` files; Publisher reads them and publishes. Each returns to BASIC. Setup guidance must preserve that boundary.

## Guided path

The website's `start.php` is a six-step, server-rendered walkthrough: hardware/package preparation, website creation, pairing and verified settings save, editing/local save, first page/post publication, then checking the result and later updates. It is available before site configuration, requires no JavaScript or CSS, and collects no credentials. Page/post choice and current step are carried in the URL. Back, Next and direct step links let users resume without a new persisted format. Navigation is a user acknowledgement, never a claimed machine or publishing test.

Website `setup.php` links to the guide and hands successful setup to pairing. HTTPS-only `pair.php` links back to the settings step and forward to writing. Pairing retains its existing password/key security boundary; the guide never displays a publishing key.

Independent `S64CFG.PRG` opens its three-step setup wizard when the site or direct key is missing. W on Basic Settings runs it again. Existing configured users retain the Basic screen on launch.

1. Website: instructions to create the site in a browser and use `start.php`; 1 edits the site-folder URL, N advances when nonempty.
2. Key: HTTPS pairing instructions and private `/sd/SEE64YOU/SEE64KEY.TXT` import (1), or masked manual entry (2). N requires a nonempty, structurally valid key/settings record.
3. Review: explains unencrypted direct HTTP, 1 invokes the existing explicit Y/N consent prompt, S uses existing validation, temporary-file verification, replacement and final readback. No save is attempted without a Standalone site/key and explicit HTTP consent. Existing transport services remain the authority when Publisher connects; the wizard never rewrites an HTTPS URL into HTTP.

B goes back; W returns to Basic while retaining in-memory edits; STOP cancels against the launch-time original and exits to BASIC without saving. Offline editing is available independently. Advanced Settings remains the route for existing relay/other destination configuration.

Only a successful verified disk save displays `SETTINGS SAVED AND VERIFIED`, followed by Return to BASIC. Save errors retain the wizard and its inputs. The completion screen explicitly says connection has not been tested. It names Editor and Publisher and explains the remaining write/save/load/connect/type/state/publish/browser checks; it does not automatically chain programs or transfer documents.

## Required machine setup

Editor retains the shared platform-preflight implementation: UCI, Ultimate-DOS, exact 16 MiB REU and writable/readable SD checks, with exact prerequisite guidance, R recheck and X exit. See C64U-RUNTIME.md. Missing network setup must not prevent offline editing. Persistent prerequisites are user choices; temporary runtime changes belong to the system-state manager and must be restored.

## Files and security

Copy the complete matching independent package to `/sd/SEE64YOU/`. Launch `SEE64YOU.PRG`, `S64CFG.PRG` or `S64PUB.PRG` independently; other PRGs/BINs are internal assets. Configuration is authoritative at `/sd/SEE64YOU/SEE64YOU.CFG`, with existing verified legacy migration. Source files use the SD root; key import prefers the application folder and retains the legacy root fallback. Remove the private key-import file when no longer needed.

The browser publishing password is never requested on the C64U. Pairing requires HTTPS. Firmware-1.1 direct HTTP uses the dedicated challenge/HMAC key; it authenticates requests but does not encrypt document bodies. Consent remains explicit and there is no silent secure-transport downgrade.

## Instructions and validation

[FIRST-PAGE.md](FIRST-PAGE.md) is the printable first-publication checklist. README links it and the browser wizard. LOCAL_TESTING owns reproducible tests and their scope; todo.md owns remaining work. The original three-program hardware checkpoint remains valid for its exact older package; the new wizard separately passes bounded real-C64U first-run/cancel/import/consent/verified-save/relaunch and Publisher-auth checks using remote plain keys, with exact original SD and live turbo restoration. Physical wizard key input, a new cold power cycle, USB and physical interruption coverage remain separate.
