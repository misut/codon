# codon

A documentation-site generator. Reads a manifest plus Markdown chapters
from a repo's `docs/` directory and emits a phenotype-rendered web book
— the same shape of site as `doc.rust-lang.org/stable/book/` but drawn
by phenotype's WebGPU backend instead of static HTML.

The authoring side (Markdown + `book.toml`) is language-agnostic; the
runtime side happens to be phenotype because that's what misut already
uses to render web UI. Internal implementation notes about codon's own
C++23 codebase are kept in this AGENTS file rather than the user docs.

## Packages

- `codon/` — the generator CLI (WASI binary). Parses `book.toml` and markdown,
  emits `book.json` + copied assets into a `dist/` directory.
- `reader/` — a project-agnostic phenotype application (WASI binary). Loads
  `book.json` at boot and renders the book UI (sidebar TOC, content pane,
  search, keyboard navigation).

## Build

```sh
mise install
eval "$(intron env)"
exon build --target wasm32-wasi   # run inside codon/ or reader/
```

## Code Style

- C++23, `import std;` not `#include`. No external libraries beyond the exon
  dependencies declared in each sub-package's `exon.toml`
  (`tomlcpp`, `jsoncpp`, `cppx`, `phenotype`).
- Module file naming follows the dot-matches-module convention used in
  phenotype (`foo.bar.cppm` for `export module foo.bar;`).
- Within `codon/` the generator uses flat, exon-style module names
  (`manifest`, `summary`, `markdown`, `book`, `emit`, `search`, `assets`).
  Within `reader/` modules live under the `reader.*` namespace.

## Conventions

- Commits and PR titles follow [Conventional Commits](https://www.conventionalcommits.org/).
  No `Co-Authored-By` trailers.
- Branches: `feat/`, `fix/`, `chore/`, `docs/`, `refactor/` prefix.
- Remote: `git@github.com:misut/codon.git`
- Conversations: Korean. Documentation, commits, PRs: English.

## Worktree policy

All work happens in a git worktree under
`$HOME/Private/.worktrees/codon/<branch>`. The main checkout stays on `main`
and is shared between agent sessions.

```sh
git -C $HOME/Private/codon worktree add \
  $HOME/Private/.worktrees/codon/feat/<slug> -b feat/<slug>
```

## Release artifacts

- `codon.wasm` — the generator, invoked as
  `wasmtime --dir=. codon.wasm build docs/ dist/`.
- `reader.wasm` — the viewer, copied into `dist/` alongside `phenotype.js`
  and the generated `book.json`.
