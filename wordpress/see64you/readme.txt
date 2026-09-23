=== See64You ===
Contributors: see64you
Requires at least: 6.0
Requires PHP: 8.0
Stable tag: 0.1.0
License: GPLv2 or later

A small REST publishing adapter for the See64You Commodore 64 Ultimate editor.

== Description ==

See64You exposes a compact document-oriented API for WordPress pages and posts.

The plugin stores the canonical See64You source in post metadata and writes generated HTML to normal WordPress post content. Existing WordPress posts can be opened as a lossy plain-text fallback; after saving from See64You they become source-managed.

Authentication is handled by WordPress. The intended C64U setup uses a WordPress Application Password over HTTPS rather than the user's normal account password.

== Installation ==

1. Copy the see64you folder into wp-content/plugins/.
2. Activate See64You in WordPress.
3. In the WordPress user profile, create an Application Password for the C64U client.
4. Configure the client with the WordPress site URL, username and Application Password.
5. Test GET /wp-json/see64you/v1/status.

== API ==

GET /wp-json/see64you/v1/status
GET /wp-json/see64you/v1/documents?type=post
POST /wp-json/see64you/v1/documents
GET /wp-json/see64you/v1/documents/{id}
POST /wp-json/see64you/v1/documents/{id}
DELETE /wp-json/see64you/v1/documents/{id}

Normal WordPress capabilities are checked for every protected operation.
