# codon

A documentation-site generator. Reads a `book.toml` manifest plus a
tree of Markdown chapters from a repo's `docs/` directory and emits a
web book readable in any browser. The rendered output is a phenotype
application — built on
[phenotype](https://github.com/misut/phenotype)'s WebGPU 2D renderer
— not static HTML.

```
docs/
├── book.toml                 # manifest
└── src/
    ├── SUMMARY.md            # table of contents
    ├── introduction.md
    └── guide/
        ├── install.md
        └── first-build.md
```

```sh
wasmtime --dir=. codon.wasm build docs/ dist/
python3 -m http.server --directory dist 8000
# open http://localhost:8000
```

## Packages

| Path | Role |
|---|---|
| `codon/` | The generator. Parses manifest and markdown, emits `book.json`. |
| `reader/` | The viewer. A phenotype app that renders `book.json` in the browser. |
| `shell/` | Static assets copied into every generated site (`index.html`, `phenotype.js`, `theme.json`). |
| `docs/` | codon's own documentation, used as a dogfood build target. |

## Building from source

```sh
mise install          # pulls exon + intron at pinned versions
eval "$(intron env)"  # installs llvm / cmake / ninja / wasi-sdk / wasmtime
# Build the generator
cd codon && exon build --target wasm32-wasi
# Build the reader
cd ../reader && exon build --target wasm32-wasi
```

## Using codon in another repo

```yaml
# .github/workflows/pages.yml
- uses: bytecodealliance/actions/wasmtime@v1
- run: |
    gh release download --repo misut/codon \
      --pattern 'codon.wasm' --pattern 'reader.wasm'
    wasmtime --dir=. codon.wasm build docs/ dist/
- uses: actions/upload-pages-artifact@v3
  with: { path: dist }
- uses: actions/deploy-pages@v4
```

## Manifest (`docs/book.toml`)

```toml
[book]
title       = "my project"
authors     = ["..."]
language    = "en"
description = "..."

[build]
src  = "src"
dest = "book"

[output.html]
default-theme = "auto"     # "auto" | "light" | "dark"
search        = true
edit-url      = "https://github.com/owner/repo/edit/main/docs/{path}"
```

## Table of contents (`docs/src/SUMMARY.md`)

`SUMMARY.md` mirrors the mdBook grammar: unnumbered prefix and suffix
chapters, `-` numbered chapters with arbitrary nesting, optional part
titles declared as `# Part name`, draft chapters as `- [Title]()`, and
`---` separators.

## License

MIT
