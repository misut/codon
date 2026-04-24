export module reader.book;
import std;

// In-memory representation of a codon `book.json`. Mirrors the schema
// emitted by the generator sub-package (`schema: 1`) but intentionally
// lives in its own namespace so the reader stays decoupled from the
// generator's C++ types — the contract is the JSON shape.

export namespace reader {

enum class SpanKind : int { Text, Emph, Strong, Code, Link, Image, HardBreak };
enum class BlockKind : int {
    Heading, Paragraph, CodeBlock, List, ListItem, Blockquote, ThematicBreak
};

struct Span {
    SpanKind    kind = SpanKind::Text;
    std::string text;
    std::string href;
};

// Blocks live in a flat pool addressed by index, matching the generator's
// arena layout. Avoids any pointer or std::variant recursion.
using BlockId = std::uint32_t;
inline constexpr BlockId invalid_block = ~BlockId{0};

struct Block {
    BlockKind             kind = BlockKind::Paragraph;
    int                   level = 0;
    std::string           anchor;
    std::vector<Span>     spans;
    std::string           language;
    std::string           text;
    bool                  ordered = false;
    std::vector<BlockId>  children;
};

struct Chapter {
    std::string           id;
    std::string           title;
    std::string           path;
    std::vector<Block>    pool;     // all blocks
    std::vector<BlockId>  roots;    // top-level block indices
};

struct TocEntry {
    std::string              title;
    std::string              id;
    std::string              number;
    bool                     draft = false;
    std::vector<TocEntry>    children;
};

struct TocPart {
    std::optional<std::string> title;
    std::vector<TocEntry>      entries;
};

struct Toc {
    TocPart                prefix;
    std::vector<TocPart>   parts;
    TocPart                suffix;
};

struct Meta {
    std::string               title;
    std::vector<std::string>  authors;
    std::string               language;
    std::string               description;
};

struct Output {
    std::string               default_theme = "auto";
    bool                      search        = true;
    std::string               edit_url;
};

struct Book {
    int                                            schema = 1;
    Meta                                           meta;
    Output                                         output;
    Toc                                            toc;
    std::vector<Chapter>                           chapters;
    std::map<std::string, std::size_t>             by_id;  // chapter id -> index

    // Search postings: lowercase token -> chapter ids.
    std::map<std::string, std::vector<std::string>> search_index;

    Chapter const* chapter(std::string const& id) const {
        auto it = by_id.find(id);
        if (it == by_id.end()) return nullptr;
        return &chapters[it->second];
    }
};

// Build a flat, reading-order list of (entry, part-title-or-null) pairs
// across prefix → parts → suffix, skipping drafts. Used by the reader's
// sidebar and prev/next navigation.
inline std::vector<TocEntry const*> flatten(Toc const& toc) {
    std::vector<TocEntry const*> out;
    auto recurse = [&](auto& self, TocEntry const& e) -> void {
        if (!e.draft) out.push_back(&e);
        for (auto const& c : e.children) self(self, c);
    };
    for (auto const& e : toc.prefix.entries) recurse(recurse, e);
    for (auto const& p : toc.parts)
        for (auto const& e : p.entries) recurse(recurse, e);
    for (auto const& e : toc.suffix.entries) recurse(recurse, e);
    return out;
}

} // namespace reader
