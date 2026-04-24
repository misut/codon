# CLI commands

```
codon <command> [args]
```

All codon commands are read-only against the filesystem except `build`
and `init`, which write to the destination directory you pass.

## `codon build <docs-dir> [dist-dir]`

Builds `docs-dir/book.toml` + `docs-dir/src/**.md` into `dist-dir/`.
`dist-dir` defaults to `dist`.

Emits:

- `dist-dir/book.json` — the serialised book document.
- `dist-dir/assets/**` — non-Markdown files from `docs-dir/src/`
  copied one-for-one, preserving relative paths.

Exits non-zero on parse errors, missing SUMMARY entries, and broken
local links.

## `codon check <docs-dir>`

Parses everything `build` would but does not touch the filesystem.
Intended for CI and pre-commit hooks.

## `codon init <docs-dir>`

Scaffolds a starter `book.toml` + `src/SUMMARY.md` + two sample
chapters under `docs-dir`. Existing files are not overwritten.

## `codon version`

Prints the running codon version.
