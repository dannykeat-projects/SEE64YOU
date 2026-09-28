# See64You Markup v1

See64You Markup v1 is the frozen source-formatting subset for document bodies. It is intentionally smaller than Markdown so the same source can be edited predictably on a 40-column C64 screen and rendered consistently by simple backends.

The canonical document remains the `SEE64YOU/1` source document. Markup is stored exactly as source text; generated HTML is disposable output.

## Supported block syntax

Only these block forms are defined by v1:

- `# Heading` — level-1 heading.
- `## Heading` — level-2 heading.
- `- item` — unordered-list item. Consecutive list-item lines form one list.
- An empty line — blank separation.
- Any other line — ordinary paragraph text.

The marker must begin in column 1 and must be followed by the shown space. Heading levels 3 and above are not part of v1.

## Supported inline syntax

Only these inline forms are defined by v1:

- `**bold**`
- `*italic*`
- `[label](http://example.com)`
- `[label](https://example.com)`

Link labels must be non-empty and may not contain `]`. Link URLs must use HTTP or HTTPS and may not contain whitespace or `)`.

Bold and italic may be combined using the natural star-marker overlap already produced by the editor, for example `***text***`. v1 does not otherwise define arbitrary nested or recursive markup.

## Literal and unsupported text

See64You is source-oriented. Text that does not match a v1 construct remains literal text rather than being discarded or interpreted as another Markdown feature.

Examples that remain literal in v1 include:

- `### Heading three`
- backtick/code syntax
- strikethrough such as `~~text~~`
- block quotes
- ordered lists
- images
- FTP or other non-HTTP(S) links
- malformed or unterminated formatting markers

Raw HTML is never treated as markup by the standalone renderer; it is escaped and displayed as text.

## Line endings and characters

The canonical `SEE64YOU/1` wire form uses 7-bit ASCII with LF line endings. The C64 editor stores native PETSCII and converts at the wire boundary. Characters that cannot be represented by the current canonical source repertoire are rejected instead of silently changed.

Soft word wrap is a display concern only and never changes the source.

## Editor commands

Formatting commands are conveniences for inserting/removing the same source markers:

- Bold: `**...**`
- Italic: `*...*`
- Heading 1/2: `# ` / `## `
- List item: `- `
- Link: `[label](url)`

A source construct does not depend on having a dedicated shortcut or menu item. Users can always type the markers directly. This is important on the conventional C64, where resident RAM limits which convenience UI can be linked.

## Compatibility rule

See64You Markup v1 is frozen. Backends may add presentation around the generated content, but they must not reinterpret unsupported source as a new v1 construct.

Any incompatible change to the syntax or meaning requires a later markup version rather than silently changing v1.
