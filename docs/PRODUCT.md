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

The PHP site serves HTML and a read-only published-source URL: `source.php?type=page&slug=SLUG` (or `type=post`). It returns exact ASCII/LF bytes as `text/8tml`, without redirects or a static export. Drafts, unpublished/deleted entries and invalid identities return 404, even with authentication or preview parameters. Responses use `no-store` and `nosniff`; authenticated API export remains separate.

The external 8TML/T8WN browser is still being tested according to the user (22 October). HTTP bytes/privacy are validated independently; actual browser URL, MIME and link interoperability remain unverified until a working implementation is available. Do not build a competing SEE64YOU browser.

## Implemented publication safety and remaining work

New 8TML API saves default to create-only and refuse an occupied page/post slug. Explicit updates require a matching remote identity and an existing target, checked inside the same directory write lock as replacement. Publisher asks whether a local upload targets an existing slug and confirms replacement, displaying the selected slug. Portable source contains no service metadata. Reopening a local file can explicitly target its existing remote slug without downloading over the local source.

Host/PHP tests cover create collisions, matching/missing identities, missing update targets and simultaneous creators; native transport tests cover collisions, missing-target refusal and explicit updates. Real-C64U acceptance remains pending. The primary screen now leads with choosing a local file, publication options and Publish/Update; remote download and maintenance commands are secondary. Publish tests the configured key when needed. A stale-revision precondition remains to be implemented; confirmed updates currently replace the latest version under the lock.

## Code boundaries and simplification

- Editor owns source, cursor, buffer/history, file safety and reversible local editing.
- Publisher owns network authorization, remote identity and optional upload/download; do not duplicate document serialization or disk recovery.
- Settings owns configuration and key provisioning.
- A small server adapter may translate authenticated operations to self-hosted PHP. The 8TML format does **not** depend on this PHP API, a particular service or T8WN.
- The 8TML/T8WN ecosystem owns the separate compatible browser. The active source-package build omits `S64WEB.PRG`; optional diagnostic/browser source and site-side protocol remain until their actual callers can be removed safely.

Remove dead code **incrementally after caller tracing**, not by deleting large directories speculatively. Shipping launcher/module manifest dependencies, resource headroom, disk recovery and real-hardware restoration are release gates. This document specifies the target design; the present binaries still contain several extra Publisher operations.

See [todo](../todo.md) for the next bounded work and [architecture](ARCHITECTURE.md) for implemented boundaries.
