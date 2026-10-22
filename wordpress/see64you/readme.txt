=== See64You ===
Contributors: see64you
Requires at least: 6.0
Requires PHP: 8.0
Stable tag: 0.1.0
License: GPLv2 or later

A compact REST publishing adapter for the See64You C64 Ultimate editor.

== Description ==

See64You is a C64 Ultimate-native editor/publishing environment. The WordPress plugin exposes document-oriented REST operations for pages and posts while keeping the client independent of WordPress internals.

The plugin stores canonical See64You source in post metadata and writes generated HTML to normal WordPress post content. Existing WordPress posts can be opened as a lossy plain-text fallback; after saving from See64You they become source-managed.

Authentication is handled by WordPress. Use a WordPress Application Password rather than the user's normal account password.

On the current firmware-1.1 C64U baseline, HTTPS-required WordPress publishing should use the See64You trusted-LAN relay until an on-device TLS path is separately validated.

The C64U client's 16 MiB REU/module-cache architecture does not change the WordPress wire contract.

== Installation ==

1. Copy the see64you folder into wp-content/plugins/.
2. Activate See64You in WordPress.
3. In the WordPress user profile, create an Application Password for the See64You client.
4. Configure a WordPress profile in See64You Advanced Settings.
5. Use the trusted-LAN relay when HTTPS is required by the current client transport.
6. Test authenticated GET /wp-json/see64you/v1/connection.

== API ==

GET /wp-json/see64you/v1/status
GET /wp-json/see64you/v1/connection
GET /wp-json/see64you/v1/documents?type=post&managed=1
POST /wp-json/see64you/v1/documents?ack=1
GET /wp-json/see64you/v1/documents/{id}?chunks=1
PATCH /wp-json/see64you/v1/documents/{id}?ack=1
DELETE /wp-json/see64you/v1/documents/{id}

Normal WordPress capabilities are checked for every protected operation, including per-document edit/delete checks.

Managed lists expose supported draft/published See64You documents. Create/update accepts the same application/x-see64you canonical source used by the standalone backend.

Changing a published document to draft unpublishes it.

Slug collisions are rejected instead of allowing WordPress to silently rename the canonical slug.
