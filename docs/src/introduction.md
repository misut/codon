# Introduction

**codon** turns a `docs/` directory of Markdown chapters into a web book
rendered by [phenotype](https://github.com/misut/phenotype). Authoring
is plain Markdown plus a small TOML manifest — codon doesn't care what
language the project being documented is written in.

```
docs/
├── book.toml                 # manifest
└── src/
    ├── SUMMARY.md            # table of contents
    ├── introduction.md
    └── guide/
        └── getting-started.md
```

```sh
wasmtime --dir=. codon.wasm build docs/ dist/
```

The generator runs as a single WASI binary, so GitHub Actions and local
developers invoke the exact same artifact.

## What you get

- A phenotype-rendered reader — sidebar table of contents, content
  pane, top-bar search, prev / next strip at the footer
- An mdBook-shaped input format (`book.toml` + `SUMMARY.md` + nested
  Markdown chapters) so muscle memory transfers
- A static `dist/` directory ready to ship to GitHub Pages or any
  static host

## What codon is not

- A drop-in mdBook replacement. The feature set is intentionally a
  subset; some preprocessors and renderer plugins are deferred.
- A static HTML generator. Output is a phenotype WebAssembly
  application; rendering goes through WebGPU.
