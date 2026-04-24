export module reader.load;
import std;
import json;
import reader.book;

export namespace reader {

namespace detail {

inline SpanKind span_kind_from(std::string const& k) {
    if (k == "em")  return SpanKind::Emph;
    if (k == "st")  return SpanKind::Strong;
    if (k == "c")   return SpanKind::Code;
    if (k == "a")   return SpanKind::Link;
    if (k == "img") return SpanKind::Image;
    if (k == "br")  return SpanKind::HardBreak;
    return SpanKind::Text;
}

inline BlockKind block_kind_from(std::string const& k) {
    if (k == "p")     return BlockKind::Paragraph;
    if (k == "code")  return BlockKind::CodeBlock;
    if (k == "list")  return BlockKind::List;
    if (k == "li")    return BlockKind::ListItem;
    if (k == "quote") return BlockKind::Blockquote;
    if (k == "hr")    return BlockKind::ThematicBreak;
    if (k == "h")     return BlockKind::Heading;
    return BlockKind::Paragraph;
}

inline std::string str_or(json::Value const& v, std::string const& key, std::string def = {}) {
    if (!v.is_object()) return def;
    auto const& o = v.as_object();
    auto it = o.find(key);
    if (it == o.end() || !it->second.is_string()) return def;
    return it->second.as_string();
}

inline bool bool_or(json::Value const& v, std::string const& key, bool def) {
    if (!v.is_object()) return def;
    auto const& o = v.as_object();
    auto it = o.find(key);
    if (it == o.end() || !it->second.is_bool()) return def;
    return it->second.as_bool();
}

inline std::vector<Span> load_spans(json::Value const& arr) {
    std::vector<Span> out;
    if (!arr.is_array()) return out;
    out.reserve(arr.as_array().size());
    for (auto const& v : arr.as_array()) {
        Span s;
        s.kind = span_kind_from(str_or(v, "k", "t"));
        s.text = str_or(v, "t");
        s.href = str_or(v, "h");
        out.push_back(std::move(s));
    }
    return out;
}

inline BlockId load_block(json::Value const& v, std::vector<Block>& pool);

inline std::vector<BlockId> load_block_array(json::Value const& arr,
                                             std::vector<Block>& pool) {
    std::vector<BlockId> out;
    if (!arr.is_array()) return out;
    out.reserve(arr.as_array().size());
    for (auto const& child : arr.as_array()) out.push_back(load_block(child, pool));
    return out;
}

inline BlockId load_block(json::Value const& v, std::vector<Block>& pool) {
    Block b;
    b.kind = block_kind_from(str_or(v, "k", "p"));
    if (v.is_object()) {
        auto const& o = v.as_object();
        if (auto it = o.find("l"); it != o.end() && it->second.is_integer())
            b.level = static_cast<int>(it->second.as_integer());
        b.anchor   = str_or(v, "a");
        b.language = str_or(v, "lang");
        b.text     = str_or(v, "t");
        if (auto it = o.find("s"); it != o.end())
            b.spans = load_spans(it->second);
        b.ordered = bool_or(v, "o", false);
        if (auto it = o.find("c"); it != o.end())
            b.children = load_block_array(it->second, pool);
    }
    auto id = static_cast<BlockId>(pool.size());
    pool.push_back(std::move(b));
    return id;
}

inline TocEntry load_toc_entry(json::Value const& v) {
    TocEntry e;
    e.title  = str_or(v, "title");
    e.id     = str_or(v, "id");
    e.number = str_or(v, "num");
    e.draft  = bool_or(v, "draft", false);
    if (v.is_object()) {
        auto const& o = v.as_object();
        if (auto it = o.find("children"); it != o.end() && it->second.is_array()) {
            for (auto const& c : it->second.as_array())
                e.children.push_back(load_toc_entry(c));
        }
    }
    return e;
}

inline TocPart load_toc_part(json::Value const& v) {
    TocPart p;
    if (!v.is_object()) return p;
    auto const& o = v.as_object();
    if (auto it = o.find("title"); it != o.end() && it->second.is_string())
        p.title = it->second.as_string();
    if (auto it = o.find("entries"); it != o.end() && it->second.is_array()) {
        for (auto const& e : it->second.as_array())
            p.entries.push_back(load_toc_entry(e));
    }
    return p;
}

} // namespace detail

// Parse a book.json payload into a Book. Uses jsoncpp, which under
// -fno-exceptions aborts with a `json: line N: <msg>` diagnostic on
// malformed input — a reasonable fallback for a reader that controls
// its own input pipeline.
inline Book load(std::string_view json_text) {
    auto root = json::parse(json_text);

    Book b;
    if (!root.is_object()) return b;
    auto const& o = root.as_object();

    if (auto it = o.find("meta"); it != o.end() && it->second.is_object()) {
        auto const& m = it->second.as_object();
        if (auto i = m.find("title");       i != m.end() && i->second.is_string()) b.meta.title       = i->second.as_string();
        if (auto i = m.find("language");    i != m.end() && i->second.is_string()) b.meta.language    = i->second.as_string();
        if (auto i = m.find("description"); i != m.end() && i->second.is_string()) b.meta.description = i->second.as_string();
        if (auto i = m.find("authors");     i != m.end() && i->second.is_array()) {
            for (auto const& a : i->second.as_array())
                if (a.is_string()) b.meta.authors.push_back(a.as_string());
        }
    }

    if (auto it = o.find("output"); it != o.end() && it->second.is_object()) {
        b.output.default_theme = detail::str_or(it->second, "defaultTheme", "auto");
        b.output.search        = detail::bool_or(it->second, "search", true);
        b.output.edit_url      = detail::str_or(it->second, "editUrl");
    }

    if (auto it = o.find("toc"); it != o.end() && it->second.is_object()) {
        auto const& t = it->second.as_object();
        if (auto p = t.find("prefix"); p != t.end()) b.toc.prefix = detail::load_toc_part(p->second);
        if (auto p = t.find("suffix"); p != t.end()) b.toc.suffix = detail::load_toc_part(p->second);
        if (auto p = t.find("parts"); p != t.end() && p->second.is_array()) {
            for (auto const& v : p->second.as_array())
                b.toc.parts.push_back(detail::load_toc_part(v));
        }
    }

    if (auto it = o.find("chapters"); it != o.end() && it->second.is_object()) {
        for (auto const& [cid, cv] : it->second.as_object()) {
            Chapter ch;
            ch.id    = cid;
            ch.title = detail::str_or(cv, "title");
            ch.path  = detail::str_or(cv, "path");
            if (cv.is_object()) {
                auto const& co = cv.as_object();
                if (auto bi = co.find("blocks"); bi != co.end() && bi->second.is_array()) {
                    for (auto const& blk : bi->second.as_array())
                        ch.roots.push_back(detail::load_block(blk, ch.pool));
                }
            }
            b.by_id[cid] = b.chapters.size();
            b.chapters.push_back(std::move(ch));
        }
    }

    if (auto it = o.find("search"); it != o.end() && it->second.is_object()) {
        for (auto const& [tok, arr] : it->second.as_object()) {
            if (!arr.is_array()) continue;
            std::vector<std::string> ids;
            for (auto const& v : arr.as_array())
                if (v.is_string()) ids.push_back(v.as_string());
            b.search_index.emplace(tok, std::move(ids));
        }
    }

    return b;
}

} // namespace reader
