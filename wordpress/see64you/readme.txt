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

Authentication is handled by WordPress. Use a WordPress Application Password rather than the user's normal account password. For current C64U firmware, remote HTTPS publishing should go through the See64You trusted-LAN relay so TLS certificate verification happens on the relay-to-WordPress leg.

== Installation ==

1. Copy the see64you folder into wp-content/plugins/.
2. Activate See64You in WordPress.
3. In the WordPress user profile, create an Application Password for the C64U client.
4. Configure the client with the WordPress site URL, username and Application Password.
5. Test authenticated GET /wp-json/see64you/v1/connection.

== API ==

GET /wp-json/see64you/v1/status
GET /wp-json/see64you/v1/connection
GET /wp-json/see64you/v1/documents?type=post&managed=1
POST /wp-json/see64you/v1/documents?ack=1
GET /wp-json/see64you/v1/documents/{id}?chunks=1
PATCH /wp-json/see64you/v1/documents/{id}?ack=1
DELETE /wp-json/see64you/v1/documents/{id}

Normal WordPress capabilities are checked for every protected operation, including per-document edit/delete checks. See64You-managed C64 lists expose only `draft` and `published` documents that remain within current C64 client limits.

Create/update accepts the same `application/x-see64you` source document used by the standalone backend. Changing a published document to `draft` unpublishes it. Slug collisions are rejected instead of allowing WordPress to silently rename the canonical slug.
