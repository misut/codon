# Manifest

`book.toml` lives at the root of your `docs/` directory and describes
the book to codon. The v1 schema only covers what the reader currently
renders; anything else is reserved for future extensions.

## `[book]` — metadata

```toml
[book]
title       = "exon"
authors     = ["Geunryeol Park (misut)"]
language    = "en"
description = "A Cargo-inspired build tool for C++23."
```

| Key | Type | Notes |
|-----|------|-------|
| `title` | string | Required. Shown in the top bar. |
| `authors` | array of strings | Optional. Displayed near the title. |
| `language` | string | Optional. Two-letter code; `en` by default. |
| `description` | string | Optional. Rendered in the top bar sub-line. |

## `[build]` — paths

```toml
[build]
src  = "src"        # defaults to "src"
dest = "book"       # defaults to "book"
```

Both are resolved relative to the `docs/` directory.

## `[output.html]` — reader options

```toml
[output.html]
default-theme = "auto"     # "auto" | "light" | "dark"
search        = true       # emits the inverted index when true
edit-url      = "https://github.com/owner/repo/edit/main/docs/{path}"
```

The `{path}` placeholder is replaced per chapter with the chapter's
`src`-relative path.

## Deferred

These sections are planned but not read by codon today:

- `[preprocessor.*]` — custom preprocessors
- `[output.markdown]` / `[output.pdf]` — alternative renderers
- `[rust]`-style language-specific defaults
