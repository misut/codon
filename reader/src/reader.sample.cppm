export module reader.sample;
import std;

// A tiny sample `book.json` embedded as a string literal. Lets the
// reader build + run meaningfully without the shim-side fetch pipeline
// in place yet. The follow-up phenotype host-import PR will add a
// `load_book_json` import so this literal can be replaced by fetched
// bytes.

export namespace reader::sample {

inline constexpr std::string_view book_json = R"JSON({
  "schema": 1,
  "meta": {
    "title": "codon reader",
    "authors": ["Geunryeol Park (misut)"],
    "language": "en",
    "description": "Placeholder book bundled with the reader for bootstrap."
  },
  "output": { "defaultTheme": "auto", "search": true },
  "toc": {
    "prefix":   { "entries": [ { "title": "Introduction", "id": "introduction" } ] },
    "parts":    [
      {
        "title": "Guide",
        "entries": [
          { "title": "Reader overview", "id": "guide-overview",  "num": "1" },
          { "title": "Navigation",      "id": "guide-navigation", "num": "2" }
        ]
      }
    ],
    "suffix":   { "entries": [ { "title": "About", "id": "about" } ] }
  },
  "chapters": {
    "introduction": {
      "title": "Introduction",
      "path":  "introduction.md",
      "blocks": [
        { "k": "h", "l": 1, "a": "introduction",
          "s": [ { "k": "t", "t": "Introduction" } ] },
        { "k": "p",
          "s": [ { "k": "t", "t": "This is the codon reader bundled with a placeholder book so the binary runs end-to-end." } ] }
      ]
    },
    "guide-overview": {
      "title": "Reader overview",
      "path":  "guide/overview.md",
      "blocks": [
        { "k": "h", "l": 1, "a": "reader-overview",
          "s": [ { "k": "t", "t": "Reader overview" } ] },
        { "k": "p",
          "s": [ { "k": "t", "t": "Content comes from a typed block tree. The reader walks it and emits phenotype widgets." } ] },
        { "k": "code", "lang": "cpp",
          "t": "phenotype::run<State, Msg>(view, update);" }
      ]
    },
    "guide-navigation": {
      "title": "Navigation",
      "path":  "guide/navigation.md",
      "blocks": [
        { "k": "h", "l": 1, "a": "navigation",
          "s": [ { "k": "t", "t": "Navigation" } ] },
        { "k": "p",
          "s": [ { "k": "t", "t": "Click a sidebar entry to jump to that chapter. Use prev / next at the footer to walk the spine." } ] }
      ]
    },
    "about": {
      "title": "About",
      "path":  "about.md",
      "blocks": [
        { "k": "h", "l": 1, "a": "about",
          "s": [ { "k": "t", "t": "About" } ] },
        { "k": "p",
          "s": [ { "k": "t", "t": "codon is an mdBook-shaped documentation-site generator for phenotype-based C++23 projects." } ] }
      ]
    }
  }
})JSON";

} // namespace reader::sample
