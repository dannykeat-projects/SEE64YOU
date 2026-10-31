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

Current main lists ordinary `.8tml` source in `SEE64YOU/docs/`; released alpha.6 uses the root. Publisher never creates the folder. Follow [upgrade instructions](INSTALL.md) before moving sources. Settings/program/recovery files are filtered out. Scan limits do not yet bound directory transport/drain time or allow cancellation during enumeration.

Remote selection uses the authenticated standard list and changes only the target. Manual entry remains available if listing fails. Paging repeats a bounded list request and can reflect concurrent server changes.

## Confirm publication

Press **3** to check limits and existing names. Review local source, endpoint and remote target, then **1** confirms create/replacement or **0** cancels. API 1 cannot prevent competing writes between checking existence and replacement. HTTP uploads are public and unencrypted.

Wait for the returned URL, errors and warnings. Inspect remote content before retrying an uncertain upload; writes are never automatically retried. Publisher has no page/post categories, draft visibility, deletion or remote editing.

## Validation limits

Host fixtures, SD/USB fit/manifests and VICE check software paths. Earlier REST-assisted SD packages passed selection, confirmation, create/replace/cancel, byte-exact readback and size/key refusals. Alpha.6 exact-package network acceptance covered status/list/target selection, not uploads. The document-folder build still needs hardware acceptance. Physical keys, cold power/interruption and USB-only operation remain separate gates in [todo](../todo.md).

Download-to-edit is deferred: API 1 supplies neither source GET nor guaranteed raw-source URLs. Future download requires independent source-access verification, supported transport and collision-safe new-file storage. See [Product scope](PRODUCT.md).
