# Self-hosted SEE64YOU site

**Legacy component:** retained for existing installations and regression tests, outside the current product. Current Settings/Publisher do not use this service or relay. Instructions below apply only to legacy clients.

The optional site is a PHP 8.1+ flat-file publishing service and a simple modern HTML presentation. No database, central account or JavaScript is required for ordinary public pages. It can be installed under your own domain. It is **not** a complete 8TML renderer or a T8WN API server.

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
