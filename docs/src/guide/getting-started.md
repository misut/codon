# Getting started

## Install the WASI binary

codon publishes `codon.wasm` and `reader.wasm` as artifacts on each
GitHub release.

```sh
gh release download --repo misut/codon \
  --pattern 'codon.wasm' --pattern 'reader.wasm'
```

Any WebAssembly runtime works. The reference runtime in the GitHub
Actions workflow below is [wasmtime](https://wasmtime.dev/).

## Scaffold a new book

```sh
wasmtime --dir=. codon.wasm init docs
```

This creates `docs/book.toml`, `docs/src/SUMMARY.md`, and two starter
chapters so you can see the expected shape.

## Build

```sh
wasmtime --dir=. codon.wasm build docs dist
```

codon produces a `book.json` plus any non-Markdown assets copied from
`docs/src/`. Combined with `reader.wasm`, `phenotype.js`, and the shell
`index.html`, that's your deployable site.

## Check without writing output

```sh
wasmtime --dir=. codon.wasm check docs
```

Useful in pre-commit hooks — it parses the full book and exits non-zero
if anything is broken, without touching the output directory.
