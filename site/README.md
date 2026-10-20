# SEE64YOU v0.1.0-alpha.1 — PHP website installation

> **Legacy software.** The released site reads SEE64YOU/1 `.s64` and does not implement 8TML or the T8WN API. Keep it available for existing alpha users, but new development is directed to [8TML portability and independent hosting](../docs/8TML-ROADMAP.md). Do not migrate existing users by replacing this package in place.

**Early alpha: this may not work on your hosting setup.** Bugs and incomplete
testing may cause failures or data loss. Back up any existing website and use a
new disposable directory and non-sensitive test content first. This is an
experimental prerelease, not a dependable public beta. Complete native C64U
first-user acceptance remains unfinished.

1. Use PHP 8.1+ hosting with HTTPS available for setup and pairing. No database
   is needed. The C64U programs are supplied in the separate C64U ZIP.
2. Upload the contents of `see64you-site/` to a new website directory, including
   hidden files. Do not overwrite an existing site's configuration or content.
   The package includes starter pages and an `ENABLE_SETUP` marker for a fresh
   installation, and excludes private `config.php` and sessions.
3. Allow PHP to write its configuration, `data/`, `pages/` and `posts/`. Deny
   direct web access to `data/`: Apache can use its included `.htaccess`; other
   servers need equivalent access controls.
4. Open `start.php` in a modern browser and follow the guide. `setup.php` creates
   the site and removes `ENABLE_SETUP` after successful setup. Preserve an
   existing configured site; do not re-enable setup on it.
5. Open `pair.php` over HTTPS and obtain the dedicated publishing key. Import
   it in C64U Settings, save and verify. Keep the key private and remove any
   temporary key-import file when finished. Never enter the website password
   into the C64U publishing program.

Firmware-1.1 C64U publication uses explicit HTTP with challenge/HMAC
authentication: requests have integrity and replay protection, but document
text is **not encrypted**. The client never silently downgrades HTTPS. Your host
must allow the configured direct HTTP endpoint; a redirect to HTTPS will not
work with that direct path. Normal browser administration and pairing use HTTPS.

Keep the matching C64U package together and follow the included `FIRST-PAGE.md`
or [versioned first-page guide](https://github.com/dannykeat-projects/SEE64YOU/blob/v0.1.0-alpha.1/docs/FIRST-PAGE.md).
Back up canonical `.s64` documents and site content. Native Editor
reopen/edit/update, physical-keyboard and cold-power observations remain open;
USB keyboard and uncontrolled power/SD interruption acceptance are deferred.
