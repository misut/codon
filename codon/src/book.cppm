export module book;
import std;
import core;
import manifest;
import summary;
import markdown;

export namespace book {

struct Chapter {
    std::string             id;              // slug used for URLs
    std::string             title;
    std::filesystem::path   relative_path;   // e.g. "guide/install.md"
    markdown::Document      doc;
};

struct Book {
    manifest::Manifest               meta;
    summary::Summary                 toc;
    std::vector<Chapter>             chapters;
    std::map<std::string, std::size_t> by_id;   // id -> index in chapters
};

inline core::Result<Book> assemble(std::filesystem::path const& book_toml) {
    Book b;

    auto m = manifest::load(book_toml);
    if (!m) return std::unexpected(m.error());
    b.meta = std::move(*m);

    auto root = book_toml.parent_path();
    auto src_dir = root / b.meta.build.src;

    auto summary_path = src_dir / "SUMMARY.md";
    auto summary_text = core::read_file(summary_path);
    if (!summary_text) return std::unexpected(summary_text.error());
    auto s = summary::parse(*summary_text);
    if (!s) return std::unexpected(s.error());
    b.toc = std::move(*s);

    std::vector<std::tuple<std::string, std::string, std::filesystem::path>> flat;
    auto collect = [&](auto& self, std::vector<summary::Entry> const& es) -> void {
        for (auto const& e : es) {
            if (!e.draft && !e.path.empty()) {
                flat.emplace_back(e.id, e.title, e.path);
            }
            self(self, e.children);
        }
    };
    collect(collect, b.toc.prefix.entries);
    for (auto const& part : b.toc.parts) collect(collect, part.entries);
    collect(collect, b.toc.suffix.entries);

    b.chapters.reserve(flat.size());
    for (auto const& [id, title, rel] : flat) {
        auto file = src_dir / rel;
        auto doc = markdown::parse_file(file);
        if (!doc) {
            return core::err("{}: {}", file.string(), doc.error());
        }
        Chapter ch;
        ch.id = id;
        ch.title = title;
        ch.relative_path = rel;
        ch.doc = std::move(*doc);
        b.by_id[id] = b.chapters.size();
        b.chapters.push_back(std::move(ch));
    }

    return b;
}

} // namespace book
