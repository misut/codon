# Table of contents

`src/SUMMARY.md` defines the book's structure. codon mirrors the mdBook
grammar so existing muscle memory transfers.

```markdown
# Summary

[Introduction](introduction.md)         # prefix chapter

---

# Guide                                  # part title

- [Getting started](guide/start.md)
  - [Install](guide/install.md)
  - [First build](guide/first-build.md)
- [Reference](reference.md)
- [Draft]()                              # draft chapter

---

[Appendix](appendix.md)                  # suffix chapter
```

## Kinds of entries

- **Prefix chapters** — bare `[Title](path)` links before the first
  numbered list. Rendered above the numbered spine, without numbering.
- **Numbered chapters** — `- [Title](path)` list items. Nested with
  two-space indent. Numbered `1`, `1.1`, `2`, … in reading order.
- **Part titles** — a level-one heading (`# Part Name`) introduces a
  part. Parts are separated by a single `---` on its own line.
- **Draft chapters** — `- [Title]()` with an empty href. Rendered as
  a disabled sidebar entry.
- **Suffix chapters** — bare `[Title](path)` links after the last
  numbered list. Rendered below the numbered spine, unnumbered.

## Anchors and slugs

Each referenced file becomes a chapter whose id is the slugified
relative path (without the `.md` extension, `/` replaced by `-`). The
active chapter's id appears in the URL fragment once the runtime
location-hash wiring lands.
