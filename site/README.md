# SEE64YOU PHP site

An optional **self-hosted PHP 8.1+ service** for publishing 8TML source and displaying a simple HTML view. No database is required. It is not a complete 8TML renderer or a T8WN-compatible write API.

1. Deploy the packaged `see64you-site/` files to an unused site directory, including hidden files. Back up any current configuration and content before changing a site.
2. Grant PHP write access to configuration, `data/`, `pages/` and `posts/`. Deny direct web access to `data/` with the included Apache `.htaccess` or equivalent server configuration.
3. Visit `start.php` / `setup.php` in a modern browser to create the site. Setup is enabled only while the `ENABLE_SETUP` marker is present and is disabled when setup succeeds.
4. Visit `pair.php` **over HTTPS** and provision a dedicated direct-publishing key for the C64U. Enter the site URL and key in `S64CFG.PRG`; keep credentials private and remove temporary key-import files.
5. Use `S64PUB.PRG` to upload a `.8tml` page/post and check published HTML in your browser. Raw source remains available through authenticated export.

Direct C64U firmware-1.1 HTTP uses challenge/HMAC for authentication and replay protection, **not encryption of the document**. Explicit plaintext consent is required. The site password must not be sent as a direct C64U publishing credential. Public display supports a limited escaped subset of 8TML, so check rendered content; unverified markup may not display as intended.

See [installation](https://github.com/dannykeat-projects/SEE64YOU/blob/main/docs/INSTALL.md), [site architecture](https://github.com/dannykeat-projects/SEE64YOU/blob/main/docs/SITE.md) and [service protocol](https://github.com/dannykeat-projects/SEE64YOU/blob/main/docs/PROTOCOL.md).
