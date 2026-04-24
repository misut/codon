# Deployment

A deployed codon site is a static directory you can host anywhere.
The shape:

```
dist/
├── index.html        # from codon's shell/ (or your own)
├── phenotype.js      # fetched from misut/phenotype at deploy time
├── reader.wasm       # from codon's release artifacts
├── book.json         # produced by `codon build`
└── assets/           # non-Markdown files copied from docs/src/
```

## GitHub Actions template

```yaml
name: Deploy docs
on:
  push:
    branches: [main]
    paths: ["docs/**", ".github/workflows/pages.yml"]
permissions:
  contents: read
  pages: write
  id-token: write

jobs:
  build:
    runs-on: ubuntu-24.04
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    steps:
      - uses: actions/checkout@v6

      - name: Install wasmtime
        uses: bytecodealliance/actions/wasmtime/setup@v1

      - name: Fetch codon artifacts
        run: |
          gh release download --repo misut/codon \
            --pattern 'codon.wasm' --pattern 'reader.wasm' \
            --pattern 'index.html'
        env:
          GH_TOKEN: ${{ github.token }}

      - name: Build docs
        run: |
          wasmtime --dir=. codon.wasm build docs dist

      - name: Assemble site
        run: |
          cp index.html dist/index.html
          cp reader.wasm dist/reader.wasm
          curl -sSfL \
            https://raw.githubusercontent.com/misut/phenotype/v0.13.0/shim/phenotype.js \
            -o dist/phenotype.js

      - uses: actions/upload-pages-artifact@v3
        with: { path: dist }

      - id: deployment
        uses: actions/deploy-pages@v4
```

Replace `v0.13.0` with the phenotype release you pinned. codon itself
is version-locked by the release you downloaded at the top.

## Local preview

```sh
wasmtime --dir=. codon.wasm build docs dist
python3 -m http.server --directory dist 8000
open http://localhost:8000
```
