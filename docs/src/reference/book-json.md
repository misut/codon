# `book.json` schema

`book.json` is the contract between the generator and the reader.
Version 1 is documented here; future schema bumps carry a new
`"schema"` integer.

```jsonc
{
  "schema": 1,
  "meta": {
    "title":       "codon",
    "authors":     ["..."],
    "language":    "en",
    "description": "..."
  },
  "output": {
    "defaultTheme": "auto",
    "search":       true,
    "editUrl":      "https://github.com/.../edit/main/docs/{path}"
  },
  "toc": {
    "prefix": { "entries": [ { "title": "Introduction", "id": "introduction" } ] },
    "parts": [
      {
        "title": "Guide",
        "entries": [
          { "title": "Getting started",
            "id":    "guide-getting-started",
            "num":   "1" }
        ]
      }
    ],
    "suffix": { "entries": [ { "title": "About", "id": "about" } ] }
  },
  "chapters": {
    "introduction": {
      "title":  "Introduction",
      "path":   "introduction.md",
      "blocks": [ /* see below */ ]
    }
  },
  "search": {
    "introduction": ["introduction"],
    "codon": ["introduction", "guide-getting-started"]
  }
}
```

## Blocks

Blocks use single-letter keys to keep the payload compact.

| Block kind | `k` | Fields |
|------------|-----|--------|
| Heading | `h` | `l` (level 1-6), `a` (anchor), `s` (inline spans) |
| Paragraph | `p` | `s` (inline spans) |
| Code block | `code` | `lang` (optional), `t` (raw text) |
| List | `list` | `o` (ordered bool), `c` (list items) |
| List item | `li` | `c` (child blocks) |
| Block quote | `quote` | `c` (child blocks) |
| Thematic break | `hr` | — |

## Spans

| Span kind | `k` | Fields |
|-----------|-----|--------|
| Text | `t` | `t` (literal text) |
| Emphasis | `em` | `t` (flat text in v1) |
| Strong | `st` | `t` |
| Inline code | `c` | `t` |
| Link | `a` | `t` (label), `h` (href) |
| Image | `img` | `t` (alt), `h` (src) |
| Hard break | `br` | — |

## Search index

`search` is a flat map: each lowercase token points at the chapter ids
in which it appears. The reader intersects the posting lists across
query tokens; title-substring matches outrank body-only hits.
