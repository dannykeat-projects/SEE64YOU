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

Publisher never creates the document folder. Follow [upgrade instructions](INSTALL.md) before moving sources from alpha.6's root layout. Settings/program/recovery files are filtered out. Press RUN/STOP during enumeration to cancel. A page change starts a fresh scan. Other keys leave the displayed page in place.

Remote selection uses the authenticated standard list and changes only the target. Manual entry remains available if listing fails. Paging repeats a bounded list request and can reflect concurrent server changes.

## Confirm publication

Press **3** to check limits and existing names. Review local source, endpoint and remote target, then **1** confirms create/replacement or **0** cancels. API 1 cannot prevent competing writes between checking existence and replacement. HTTP uploads are public and unencrypted.

A confirmed upload displays `PUBLISHED` and the URL returned by the server. A return to the home screen with an error is not confirmation. On the experiment branch, the home screen displays up to 80 characters of status across its bottom three rows. On a server, network or protocol failure, read the operation and any HTTP status, server error code and explanation. Press a key to return. Use **2 Browse remote** to check the target name after a failure. The local source stays on disk. Inspect remote content before retrying an uncertain upload; writes are never automatically retried. Publisher has no page/post categories, draft visibility, deletion or remote editing.

## Validation limits

The client accepts `.8tml`, `.8ss` and `.txt` for upload, subject to server validation. Recent independent-server hardware coverage includes 8SS and TXT; equivalent 8TML coverage and the reported upload failure remain open. See [todo](../todo.md) for current acceptance evidence and remaining gates.

Download-to-edit is deferred: API 1 supplies neither source GET nor guaranteed raw-source URLs. Future download requires independent source-access verification, supported transport and collision-safe new-file storage. See [Product scope](PRODUCT.md).
