# Standard 8TML publishing protocol

The independent Publisher targets the **8TML publishing API 1 test draft dated 26 October 2026**. The API is experimental. Independent-server interoperability is a separate acceptance gate; passing a disposable test endpoint does not establish conformance.

## Credentials and transport

Settings imports `8TMLKEY.TXT` from the selected application's folder. It contains ASCII `8tml-publish: 1`, `url:`, `id:` and a 64-hex-digit `key:`. Unknown fields are ignored; malformed, duplicate or missing required fields are rejected. Import replaces all three fields together and clears HTTP consent. The secret is never displayed. Save validates temporary and final configuration readback; failed final rename can leave the complete temporary record for recovery.

A standard destination is explicitly tagged in the existing configuration record. Its otherwise unused relay field holds the key ID; legacy destinations are never silently treated as standard credentials. Existing local documents and legacy configuration are not rewritten by importing a key until the user saves.

Direct transport uses UCI Network `$03`, HTTP/1.0 and HMAC-SHA256. Source is unencrypted and uploaded files are public. Explicit HTTP consent is required. HTTPS credentials can be retained but the native transport refuses HTTPS without downgrading it. Provision and revoke credentials through the independent server's HTTPS interface.

## Requests

- `GET ?op=status`: require API 1, supported HMAC authentication and a valid advertised maximum size.
- `POST ?op=challenge&id=ID`: empty body, a fresh single-use 32-hex challenge.
- `POST ?op=list&id=ID&size=0&c=CHALLENGE`: authenticate the empty content, scan bounded `file: SIZE NAME` lines for the chosen path.
- `POST ?op=put&id=ID&name=NAME&size=N&c=CHALLENGE`: exactly N source bytes followed by 64 lowercase signature digits; `Content-Length` is N+64.

The signed bytes are:

~~~text
8tml-publish/1 LF OP LF ID LF CHALLENGE LF NAME LF SIZE LF CONTENT
~~~

List uses an empty NAME and canonical decimal size 0. Put signs source while streaming it once; no JSON envelope or page/post/draft metadata is sent. Reply lines are ASCII `key: value` with LF, bounded to 255 characters. Unknown fields are ignored. Required fields and HTTP framing are validated before success; errors, warnings and the reported public URL are surfaced.

Returned public URLs may contain `@` in the path (for example `/@keato/index.8tml`); this does not permit user credentials in the URL authority.

Remote paths have at most 64 characters and use letters, digits, hyphen, underscore, dot and slash-separated nonempty segments. Dot and dot-dot segments are refused. The Publisher lowercases the chosen path before checking collisions and showing confirmation, matching standard server normalization. Local paths remain separate.

## Replacement and failure

Publisher checks the advertised size cap and requests the existing-name list before asking to publish or replace. An observed existing name requires affirmative replacement confirmation. The draft has no conditional write operation: another writer can change the destination between list and put. The UI discloses this limitation.

Each signed operation requests a new challenge. Uncertain writes are never retried automatically. A disconnect after transmission can mean the server committed the file; inspect the remote result before another attempt. Local source is never modified by publication.

Legacy CMS, WordPress, relay and custom JSON handlers remain in the repository pending replacement acceptance and caller tracing. They are not the independent Publisher's protocol. Prior release assets retain their original behavior.
