# Product scope and interface direction

SEE64YOU's product is a **small, fast, dependable offline 8TML source editor** on the Commodore 64 Ultimate. Its optional publishing workflow exists to make the documents available elsewhere, not to create another content platform or browser.

## User experience

1. Open Editor, write or reopen a `.8tml` file, and save it explicitly.
2. Copy the portable file directly to any suitable 8TML host, **or** open Publisher to send it to a configured site.
3. Read the published document using a compatible 8TML/T8WN browser, or an independently generated HTML view.

Keep the primary Editor useful without Settings, Publisher, a server or internet access. Every component must launch and exit independently. Preserve complete source text and unknown markup; avoid a proprietary source format, hidden publishing metadata in documents, or mandatory accounts.

## Publisher: minimum useful scope

Target a short, guided workflow rather than a general document manager:

- **Keep:** select one existing local source file; show its filename/title and chosen site; choose page/post and draft/published; safely publish a new slug or update a **confirmed matching remote identity**; show clear success, URL or failure; retain dedicated-key authentication, input validation, bounded network I/O and no-write-on-failure safety.
- **Keep separate:** one-time website/key/transport setup in Settings. Never ask for credentials on each publish.
- **Consider as secondary:** explicit **Download source** to a new local filename for backup/re-editing. It is useful for portability, but should not occupy the primary publishing screen.
- **Review and remove from the active Publisher workflow once safe:** remote list/open as a browsing interface and destructive deletion. Preview, text import/export, WordPress and Editor/Settings handoffs are already excluded from the independent Publisher UI, but related source/build paths still need caller analysis before deletion. Do not remove functions needed to identify and safely update a remote item.
- **Do not build:** an SEE64YOU-specific 8TML browser, independent page rendering engine for the C64U, social features, multiple-format conversion, or a CMS administration interface.

A sensible UI is **file → destination / identity → page-or-post + draft-or-public → Publish/Update → result**. The initial screen should lead with “Choose file” and “Publish”, not numbered technical commands. Show a confirmation before replacing remote content; display the actual destination and whether this is a new page or an update. Do not add UI complexity to conceal unsafe HTTP limitations.

## Public reading contract

The PHP site currently serves HTML, but `api.php?action=get&format=8tml` is authenticated and its guarded record files are not public. Therefore a visitor or an external 8TML browser does **not** currently have a stable unauthenticated 8TML source URL.

For external browser interoperability, design a public read-only URL for **published** 8TML source, with exact bytes, appropriate response type and bounded size. Drafts, unpublished content, authentication keys and internal guarded records must remain inaccessible. Confirm the actual 8TML/T8WN browser's URL, content type, redirect and naming expectations before choosing a route or adding exported static copies. Test public retrieval, draft unpublish/delete, invalid slug, disabled listing, stale caches and cross-origin/client behavior. No new SEE64YOU browser.

## Confirmed publication safety gap

In `client/src/publisher/main.c`, a new local load clears the current remote binding, while a first upload asks for a slug and sends a save request. `s64_publish_state_can_update()` only changes the client-side operation label/URL choice; the PHP save action currently replaces an existing same-format slug without checking whether the user intended creation or update. A restarted Publisher also loses its remote association because that identity is not embedded in a portable document.

Before streamlining the Publisher UI, implement explicit, server-enforced create/update intent under the existing write lock. Creation must refuse an occupied slug; update must require a deliberate remote identity, a clear overwrite confirmation and preferably a remote revision/precondition to refuse stale edits. An unbound upload must never silently overwrite. Continue to save the source independently, and preserve network failure/partial-buffer safety. Test this at the PHP layer and native Publisher transport before hardware acceptance.

## Code boundaries and simplification

- Editor owns source, cursor, buffer/history, file safety and reversible local editing.
- Publisher owns network authorization, remote identity and optional upload/download; do not duplicate document serialization or disk recovery.
- Settings owns configuration and key provisioning.
- A small server adapter may translate authenticated operations to self-hosted PHP. The 8TML format does **not** depend on this PHP API, a particular service or T8WN.
- The 8TML/T8WN ecosystem owns the separate compatible browser. The active source-package build omits `S64WEB.PRG`; optional diagnostic/browser source and site-side protocol remain until their actual callers can be removed safely.

Remove dead code **incrementally after caller tracing**, not by deleting large directories speculatively. Shipping launcher/module manifest dependencies, resource headroom, disk recovery and real-hardware restoration are release gates. This document specifies the target design; the present binaries still contain several extra Publisher operations.

See [todo](../todo.md) for the next bounded work and [architecture](ARCHITECTURE.md) for implemented boundaries.
