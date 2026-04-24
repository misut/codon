# Markdown

codon parses a CommonMark subset sized to the v1 reader. The parser is
in-house with no external dependency, kept small enough to fit in the
WASI generator binary.

## Block elements

- **Headings** — ATX only (`# ... ######`). Auto-slugified anchors.
- **Paragraphs** — consecutive non-blank lines, joined with a single
  space. Terminated by blank line or another block start.
- **Fenced code blocks** — backticks or tildes, optional info string
  (the first word of the info string is stored as the language).
- **Blockquotes** — `> ` prefix. Nested block content.
- **Unordered lists** — `-`, `*`, or `+` followed by a space. Nested
  via indent.
- **Ordered lists** — digits + `.` or `)` followed by a space.
- **Thematic breaks** — `---`, `***`, or `___` on a line of their own.

## Inline spans

- **Emphasis** — `*x*` or `_x_`
- **Strong** — `**x**` or `__x__`
- **Inline code** — `` `x` ``
- **Links** — `[label](href)`
- **Images** — `![alt](src)`

v1 stores inline content as a flat span list — emphasis does not nest
inside strong, for example. The reader renders them as plain text
today; richer styling lands alongside the location-hash runtime work.

## Include directive

codon expands `{{#include path}}` at parse time by reading the file
relative to the chapter. Useful for embedding source files so doc prose
stays in sync with real code.

````markdown
Here is the counter example in full:

```rs
{{#include ../../examples/counter/main.rs}}
```
````
