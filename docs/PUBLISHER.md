# Publisher controls and workflow

Save and exit Editor before independently launching `S64PUB.PRG`. Key setup is in [Installation](INSTALL.md); signing, transport and limits are in [Protocol](PROTOCOL.md).

## Home controls

| Key | Action |
| --- | --- |
| 1 | Local files |
| 2 | Browse remote |
| 3 | Publish |
| 4 | Test connection |
| R | Reload Settings |
| 0 / RUN/STOP | Exit to BASIC |

The 40x25 interface uses numbered keyboard actions; legacy letter shortcuts remain available. Mouse input and local folder traversal are not implemented. Digits in filename prompts enter text.

## Select source and target

Both selectors show six rows per page: **1-6** select, **7** types a name, **8/9** change pages and **0** returns. Display truncation does not change stored paths.

Publisher lists `.8tml`, `.8ss` and `.txt` source in `SEE64YOU/docs/`, case-insensitively. Type the complete filename for 8SS or text; an extensionless name retains the `.8tml` convenience. 8SS and text load as literal source without an 8TML header. Other safe Editor suffixes, such as `.md` and `.ini`, remain local; use Save As `.txt` to publish their literal text. Server acceptance of remote types is endpoint-specific.

Publisher never creates the document folder. Follow [upgrade instructions](INSTALL.md) before moving sources from alpha.6's root layout. Settings/program/recovery files are filtered out. Scan limits do not yet bound directory transport/drain time or allow cancellation during enumeration.

Remote selection uses the authenticated standard list and changes only the target. Manual entry remains available if listing fails. Paging repeats a bounded list request and can reflect concurrent server changes.

## Confirm publication

Press **3** to check limits and existing names. Review local source, endpoint and remote target, then **1** confirms create/replacement or **0** cancels. API 1 cannot prevent competing writes between checking existence and replacement. HTTP uploads are public and unencrypted.

Wait for the returned URL, errors and warnings. Inspect remote content before retrying an uncertain upload; writes are never automatically retried. Publisher has no page/post categories, draft visibility, deletion or remote editing.

## Validation limits

Host fixtures, SD/USB fit/manifests and VICE check software paths. Authenticated host tests against T8wn passed 8SS/text create, replacement and byte-exact public-source readback; `.md` was refused with HTTP 415 `bad-type`. These tests do not certify the shipping Publisher's native transport. Exact-package C64U network acceptance and external-reader stylesheet application remain tracked in [todo](../todo.md), separately from physical keys, cold power/interruption and USB-only operation.

Download-to-edit is deferred: API 1 supplies neither source GET nor guaranteed raw-source URLs. Future download requires independent source-access verification, supported transport and collision-safe new-file storage. See [Product scope](PRODUCT.md).
