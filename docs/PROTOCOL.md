# Publishing protocol

The standalone backend defines the reference See64You document operations. The WordPress adapter should expose equivalent semantics where WordPress permits them.

## Authentication

Standalone:

`POST api.php?action=login`

Fields:
- `password`

Success returns JSON containing a bearer token and expiry. Subsequent protected calls send:

`Authorization: Bearer <token>`

Use HTTPS outside trusted local testing.

WordPress uses WordPress-native REST authentication. Application Passwords are the preferred initial mechanism.

## Operations

### Status

`GET api.php?action=status`

Unauthenticated. Returns backend/protocol information without secrets.

### List

`GET api.php?action=list&type=page`

`GET api.php?action=list&type=post`

Requires authentication.

### Get

`GET api.php?action=get&type=post&slug=hello-world`

Requires authentication.

### Save

`POST api.php?action=save`

Form or JSON fields:
- `type`: `page` or `post`
- `slug`
- `title`
- `body`
- `status`: `draft` or `published`
- `date` (optional)
- `order` (optional)

Requires authentication.

### Delete

`POST api.php?action=delete`

Fields:
- `type`
- `slug`

Requires authentication.

## Response shape

Responses are small JSON objects:

```json
{"ok":true,"document":{...}}
```

Errors:

```json
{"ok":false,"error":"invalid_slug"}
```

The C64 client should map stable error identifiers to short user-facing messages.

## Versioning

The initial protocol identifier is `see64you/1`. Additive fields may be introduced without changing it. Incompatible operation/meaning changes require a new protocol version.
