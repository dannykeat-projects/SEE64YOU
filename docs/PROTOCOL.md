# 8TML publishing service contract

The independent Publisher communicates with the **optional SEE64YOU self-hosted PHP service**. This transport is not part of the 8TML format or a T8WN publishing API. The source remains a standalone `.8tml` document.

## Stored document

Authenticated `POST api.php?action=save&ack=1` accepts JSON fields `type` (`page`/`post`), `slug`, `status` (`draft`/`published`), and `raw8tml` (the complete source, including header). Optional `date` and `order` update stored metadata only when supplied; omission preserves existing values, or uses defaults for new records. The native Publisher omits both. Type, slug, status, date and order are **service metadata**, not inserted into the source.

Source must be ASCII/LF, at most 61,440 bytes, and contain a closed 8TML version-1 header whose first metadata key is `8tml: 1`. Reject invalid source without changing the stored record. The PHP service bounds the complete incoming HTTP body to 128 KiB, and caps outstanding unexpired challenges at 128.

Documents are single guarded JSON/PHP snapshots under `pages/SLUG.8tml.php` or `posts/SLUG.8tml.php`. Each contains exact source and metadata together; temporary writes are renamed under a persistent type-level `.documents.lock`. Neither the guarded file nor the JSON envelope is a source format. Public HTML is currently limited to the safely escaped supported subset of 8TML.

## Create and conditional update

For 8TML saves, omitted `intent` means `create`; an occupied slug is refused. Updating requires `intent=update&identity=SLUG&revision=TOKEN` with identity matching the body slug, an existing target and the current revision. The entire query is HMAC-covered. Compare the token and replace the record under the same write lock; missing/malformed tokens return HTTP 400 `revision_required`, stale tokens HTTP 409 `revision_conflict`. Failed updates preserve source, metadata and revision. Successful 8TML saves return `X-See64You-Revision` with the new token, including acknowledgement responses.

Every save generates a fresh opaque token, including metadata-only or identical-content saves. Existing records without tokens have a stable read-only fallback until their next save. Tokens are service metadata and never enter `.8tml` source. Native Publisher retains the token from open/save; an explicit existing-slug update from a local file obtains it via authenticated HEAD before confirmation. Local files cannot establish a historical remote baseline. On conflict, keep the local source and reopen/review the current remote record; do not retry automatically with a refreshed token. Missing, malformed or duplicate revision response headers are protocol errors. Client and service must be upgraded together; old unconditioned updates fail safely.

## Read and list

- Authenticated `GET api.php?action=get&type=page&slug=example&format=8tml` exports the exact source as `text/8tml; charset=us-ascii`, with a `.8tml` attachment name, `Cache-Control: no-store` and required `X-See64You-Status` (`draft` or `published`).
- Raw GET and authenticated HEAD also return `X-See64You-Revision`, exactly 32 lowercase hexadecimal characters. HEAD returns status/revision headers without source bytes. Without `format=8tml`, GET returns document JSON metadata including `raw8tml` and `revision`.
- List via `GET api.php?action=list&type=page&source=8tml` (or `post`) filters to supported source records. The native client uses bounded summaries and validates remote identity before updating or deleting.
- Protected `POST api.php?action=delete` removes the selected type/slug. Deletion is not required for the core publishing workflow.
- Public site pages display **published** content only. Draft previews must remain authenticated/no-store.

## Authentication and transport

Use modern HTTPS `pair.php` to provision a site-specific 256-bit direct key. The C64U obtains a one-time hex challenge with `GET api.php?action=challenge` and authenticates each protected request using `X-See64You-Challenge` and `X-See64You-Signature` (HMAC-SHA256).

HMAC input is five newline-separated lines:

~~~text
see64you-direct/1
<challenge>
<METHOD>
<exact request URI>
<SHA-256(body) as 64 lowercase hex characters>
~~~

The server checks the key, method, URI and exact body hash and consumes accepted challenges. **Direct HTTP is not encrypted**; require explicit consent for suitable content and networks. Never send a reusable website password/bearer token over this channel or silently downgrade HTTPS. Pairing and browser administration require HTTPS outside loopback testing.

The native URL parser accepts DNS/IPv4 hostname plus optional decimal port 1–65535, default 80; it rejects invalid ports, credentials and header injection before connecting through Network Target `$03`. IPv6 and general TLS are not established features. Remote writes should preserve original source on connection, authentication and validation failure.

## Scope

The PHP endpoint is one optional destination. A compatible external 8TML browser can read hosted source without using this private write API. T8WN/VVV interoperability and complete source-to-HTML conformance require separate verification.
