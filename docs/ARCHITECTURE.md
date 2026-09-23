# Architecture

## Components

See64You deliberately separates editing from publishing.

```
+----------------------------+
| C64/C64U editor            |
|                            |
| document <-> editor <-> UI |
|       \                    |
|        publisher interface |
+---------------|------------+
                |
          HTTP transport
          /            \
+----------------+  +------------------+
| Standalone PHP |  | WordPress plugin |
| flat-file CMS  |  | REST adapter     |
+----------------+  +------------------+
```

The editor remains useful without a network connection. A publisher receives and returns documents; it does not own editor state.

## Canonical document

The canonical editable representation is source text plus metadata:

- type: `page` or `post`
- slug
- title
- status: initially `draft` or `published`
- date
- order (primarily pages)
- body source

Generated HTML is disposable output. Backends must retain the source exactly enough for a later edit.

## Client

The first editor uses a fixed-capacity gap buffer. This gives cheap insertion/deletion around the cursor without repeated whole-document shifts. The initial capacity is deliberately modest so the first build can work in normal C64 address space. REU storage can later extend capacity behind the same document/editor interfaces.

The UI baseline is 40x25:
- row 0: menu/identity bar
- rows 1-23: editor viewport
- row 24: status

The first implementation prioritises correctness and a runnable shell. Incremental rendering and selection follow after baseline VICE validation.

## Transport

Publishing operations should depend on a narrow transport layer rather than direct UCI calls scattered through UI code. The intended C64U implementation is UCI HTTP where available, with room for a development/compatibility bridge if needed.

No transport should be required for local document editing.

## Standalone web backend

The standalone site:
- requires PHP but no database;
- stores pages under `pages/`;
- stores posts under `posts/`;
- stores canonical See64You document files rather than generated HTML;
- renders HTML on request;
- exposes an authenticated API for the C64 client.

Public rendering is intentionally server-side and JavaScript-free.

## WordPress backend

The WordPress plugin exposes a deliberately small document API. WordPress remains responsible for accounts, permissions and normal REST authentication. See64You source is stored in post metadata while generated HTML is stored as the post content.

## Security boundaries

The C64 client cannot make an untrusted server safe. Backends therefore validate document type, slug and authentication independently.

Standalone-site login returns a short-lived signed token. Production publishing should use HTTPS because possession of a valid token grants publishing access until expiry.
