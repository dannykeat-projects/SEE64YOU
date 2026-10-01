# See64You Markup v1

See64You Markup v1 is the frozen source-formatting subset for document bodies. It is intentionally smaller than Markdown so source remains predictable on the 40-column C64U editor and simple backends can render it consistently.

The canonical document remains SEE64YOU/1 source. Markup is stored exactly as source text; generated HTML is disposable.

## Supported block syntax

Only these block forms are defined by v1:

- # Heading — level-1 heading.
- ## Heading — level-2 heading.
- - item — unordered-list item; consecutive items form one list.
- an empty line — blank separation.
- any other line — ordinary paragraph text.

The marker must begin in column 1 and use the required following space. Heading levels 3+ are not part of v1.

## Supported inline syntax

Only these inline forms are defined by v1:

- **bold**
- *italic*
- [label](http://example.com)
- [label](https://example.com)

Link labels must be non-empty and may not contain ]. Link URLs must use HTTP/HTTPS and may not contain whitespace or ).

Bold and italic may combine using the natural star overlap already produced by the editor, for example ***text***. v1 does not otherwise define arbitrary recursive markup.

## Literal and unsupported text

Text that does not match a v1 construct remains literal rather than being discarded or reinterpreted as another Markdown feature.

Examples include:

- ### Heading three
- backtick/code syntax
- ~~strikethrough~~
- block quotes
- ordered lists
- images
- non-HTTP(S) links
- malformed/unterminated markers

Raw HTML is escaped by the standalone renderer.

## Line endings and characters

SEE64YOU/1 currently uses 7-bit ASCII with LF line endings.

The C64U editor stores native PETSCII and converts at the wire boundary. Characters outside the current canonical repertoire are rejected rather than silently changed.

This repertoire is a protocol-v1 decision, not an original-C64 compatibility requirement.

Soft word wrap is display-only and never changes source.

## Editor commands

Formatting commands are conveniences for editing the same source markers:

- Bold: **...**
- Italic: *...*
- Heading 1/2: # / ##
- List item: -
- Link: [label](url)

A source construct never depends on a dedicated shortcut. Users can type the markers directly; menus/shortcuts exist for convenience and discoverability.

The C64U-only architecture may add richer convenience UI without changing Markup v1 semantics.

## Compatibility rule

See64You Markup v1 is frozen.

Backends may add presentation around generated content but must not reinterpret unsupported source as a new v1 construct.

An incompatible syntax/meaning change requires a later markup version.
