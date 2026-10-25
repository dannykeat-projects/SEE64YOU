# Self-hosted SEE64YOU site

The optional site is a PHP 8.1+ flat-file publishing service and a simple modern HTML presentation. No database, central account or JavaScript is required for ordinary public pages. It can be installed under your own domain. It is **not** a complete 8TML renderer or a T8WN API server. Published source is available at `source.php?type=page&slug=SLUG` (or `type=post`). Small-page endpoint reading and relative-link navigation work with VVV 1.0 on C64U; see the bounded compatibility details below.

## Setup

Upload the complete packaged `see64you-site/` directory to an unused web folder, including access-control files. The server must write configuration, `pages/`, `posts/` and `data/`. Deny direct access to `data/` with the included Apache `.htaccess` or equivalent server rules. Open `start.php` / `setup.php` in a modern browser; the setup marker enables first-run configuration and is removed after setup. Do not expose setup again on a configured site.

Provision the C64U direct key over **HTTPS** in `pair.php`. The site password stays in the modern browser; C64U publishing uses the dedicated direct key and explicit plain-HTTP consent. See [installation](INSTALL.md) and [protocol](PROTOCOL.md).

## Files and rendering

`index.php`, `page.php`, `posts.php` and `post.php` render published entries. `api.php` handles authenticated source operations. Each `pages/` or `posts/` record is one guarded PHP/JSON file holding the exact `.8tml` source plus publication metadata; direct access to draft records is blocked. File slugs are validated lowercase letters, digits and hyphens. Temporary writes and per-type locks protect replacement. Backup/export should be used before server moves.

HTML is derived on request, with unsafe source escaped. Only a documented subset of 8TML is presented today; unsupported constructs must not be advertised as fully rendered. Layout/themes are optional. Public pages work without JavaScript; the plain theme should remain usable without CSS. Theme editing uses authenticated browser sessions and CSRF protection.

For isolated local testing:

~~~sh
php -S 127.0.0.1:8080 -t site
~~~

This binds the host's loopback interface, not a network address reachable by the C64U. Do not use a production configuration or real credentials in disposable tests.

To package the tracked site sources for deployment:

~~~sh
php tools/package-site.php
~~~

## Publication write intent

Authenticated 8TML `POST api.php?action=save` defaults to `intent=create` and refuses an occupied page/post slug. Updates use `intent=update&identity=SLUG`; identity must match the body slug and the target must already exist. These query parameters are covered by the existing HMAC signature. Existence checks and replacement share the directory write lock, so concurrent creates cannot silently replace one another and a deleted target cannot be recreated by an update. Invalid intent, mismatched identity and collisions fail without replacing content. Deploy the matching client and service together; older clients' unlabelled updates will safely fail as create collisions.

Updates additionally require `revision=TOKEN`, a 32-character lowercase hexadecimal token covered by the signed URI. Compare-and-replace occurs under the directory write lock. Missing or malformed tokens return `revision_required`; an old token returns HTTP 409 `revision_conflict`, preserving the entire stored record. Every 8TML save changes the token, including metadata-only and identical-content saves. Older records receive a stable read-only fallback token and acquire a random token on their next save. Deploy the matching client and service together: older clients without a revision safely fail updates.

## Public source

`GET source.php?type=page&slug=SLUG` (or `type=post`) returns the exact published ASCII/LF source as `text/8tml; charset=us-ascii`, with a byte Content-Length, no redirect, `nosniff` and `Cache-Control: no-store`. HEAD returns the same headers without a body. Other methods return 405; missing, invalid, draft, deleted, corrupt and non-8TML records return 404. Authentication or preview tokens never expose drafts through this URL. No exported static copy or separate index can survive unpublish/delete. Relative links are preserved verbatim and resolve against the endpoint URL's site directory; generated HTML URLs are not rewritten into source URLs.

VVV 1.0 (`a3c87cf`) has been exercised on a real C64U with ACIA/SwiftLink at `$DE00/NMI`, SwiftLink hardware mode and RTS handshake enabled. Open the full plain-HTTP source endpoint with F1. A small published page served by Apache was fetched as HTTP/1.0, accepted with `text/8tml`, parsed and displayed; a relative `source.php?type=page&slug=second` link fetched the second page successfully. The same source passed the VVV reference validator with zero errors/warnings. Keep VVV installed and launched separately; no SEE64YOU browser handoff is required.

This is bounded interoperability, not full 8TML conformance or complete Editor/Publisher acceptance. Independent internet hosting, redirects, large documents, physical browser controls, cold restart and clean browser exit/restoration remain unverified. The current pre-alpha VVV build documents roughly 7 KB of page memory shared with rendering records, substantially below the Editor source limit. Existing cached responses outside the server cannot be recalled; this endpoint instructs clients not to store responses.
