export module emit;
import std;
import json;
import core;
import manifest;
import summary;
import markdown;
import book;

export namespace emit {

namespace detail {

inline std::string span_kind_str(markdown::SpanKind k) {
    using K = markdown::SpanKind;
    switch (k) {
        case K::Text:      return "t";
        case K::Emph:      return "em";
        case K::Strong:    return "st";
        case K::Code:      return "c";
        case K::Link:      return "a";
        case K::Image:     return "img";
        case K::HardBreak: return "br";
    }
    return "t";
}

inline std::string block_kind_str(markdown::BlockKind k) {
    using K = markdown::BlockKind;
    switch (k) {
        case K::Heading:       return "h";
        case K::Paragraph:     return "p";
        case K::CodeBlock:     return "code";
        case K::List:          return "list";
        case K::ListItem:      return "li";
        case K::Blockquote:    return "quote";
        case K::ThematicBreak: return "hr";
    }
    return "p";
}

inline json::Array spans_to_json(std::vector<markdown::Span> const& spans) {
    json::Array arr;
    arr.reserve(spans.size());
    for (auto const& s : spans) {
        json::Object o;
        o.emplace("k", json::Value{span_kind_str(s.kind)});
        if (!s.text.empty()) o.emplace("t", json::Value{s.text});
        if (!s.href.empty()) o.emplace("h", json::Value{s.href});
        arr.push_back(json::Value{std::move(o)});
    }
    return arr;
}

inline json::Value block_to_json(markdown::Document const& doc,
                                  markdown::BlockId id);

inline json::Array children_to_json(markdown::Document const& doc,
                                     std::vector<markdown::BlockId> const& kids) {
    json::Array arr;
    arr.reserve(kids.size());
    for (auto child : kids) arr.push_back(block_to_json(doc, child));
    return arr;
}

inline json::Value block_to_json(markdown::Document const& doc,
                                  markdown::BlockId id) {
    auto const& b = doc.pool[id];
    json::Object o;
    o.emplace("k", json::Value{block_kind_str(b.kind)});
    switch (b.kind) {
        case markdown::BlockKind::Heading:
            o.emplace("l", json::Value{static_cast<std::int64_t>(b.level)});
            if (!b.anchor.empty()) o.emplace("a", json::Value{b.anchor});
            o.emplace("s", json::Value{spans_to_json(b.spans)});
            break;
        case markdown::BlockKind::Paragraph:
            o.emplace("s", json::Value{spans_to_json(b.spans)});
            break;
        case markdown::BlockKind::CodeBlock:
            if (!b.language.empty()) o.emplace("lang", json::Value{b.language});
            o.emplace("t", json::Value{b.text});
            break;
        case markdown::BlockKind::List:
            o.emplace("o", json::Value{b.ordered});
            o.emplace("c", json::Value{children_to_json(doc, b.children)});
            break;
        case markdown::BlockKind::ListItem:
        case markdown::BlockKind::Blockquote:
            o.emplace("c", json::Value{children_to_json(doc, b.children)});
            break;
        case markdown::BlockKind::ThematicBreak:
            break;
    }
    return json::Value{std::move(o)};
}

inline json::Value toc_entry_to_json(summary::Entry const& e);

inline json::Array toc_entries_to_json(std::vector<summary::Entry> const& es) {
    json::Array arr;
    arr.reserve(es.size());
    for (auto const& e : es) arr.push_back(toc_entry_to_json(e));
    return arr;
}

inline json::Value toc_entry_to_json(summary::Entry const& e) {
    json::Object o;
    o.emplace("title", json::Value{e.title});
    if (!e.id.empty())    o.emplace("id", json::Value{e.id});
    if (!e.number.empty())o.emplace("num", json::Value{e.number});
    if (e.draft)          o.emplace("draft", json::Value{true});
    if (!e.children.empty())
        o.emplace("children", json::Value{toc_entries_to_json(e.children)});
    return json::Value{std::move(o)};
}

inline json::Value part_to_json(summary::Part const& p) {
    json::Object o;
    if (p.title) o.emplace("title", json::Value{*p.title});
    o.emplace("entries", json::Value{toc_entries_to_json(p.entries)});
    return json::Value{std::move(o)};
}

} // namespace detail

// Build a tiny inverted index over chapter text: lowercase token -> list of
// chapter ids it appears in. Skips CodeBlock bodies to keep the index small.
inline json::Object build_search_index(book::Book const& bk) {
    auto collect_text = [](markdown::Document const& doc,
                           markdown::BlockId id,
                           auto& self,
                           std::string& sink) -> void {
        auto const& b = doc.pool[id];
        switch (b.kind) {
            case markdown::BlockKind::Heading:
            case markdown::BlockKind::Paragraph:
                for (auto const& sp : b.spans) {
                    if (!sp.text.empty()) { sink.append(sp.text); sink.push_back(' '); }
                }
                break;
            case markdown::BlockKind::CodeBlock:
                break;
            case markdown::BlockKind::List:
            case markdown::BlockKind::ListItem:
            case markdown::BlockKind::Blockquote:
                for (auto child : b.children) self(doc, child, self, sink);
                break;
            case markdown::BlockKind::ThematicBreak:
                break;
        }
    };

    std::map<std::string, std::set<std::string>> postings;
    for (auto const& ch : bk.chapters) {
        std::string text;
        for (auto r : ch.doc.roots) collect_text(ch.doc, r, collect_text, text);
        // Tokenize: lowercase, split on non-alphanumerics, drop < 2 chars.
        std::string token;
        auto flush = [&] {
            if (token.size() >= 2) postings[token].insert(ch.id);
            token.clear();
        };
        for (char c : text) {
            unsigned char u = static_cast<unsigned char>(c);
            if (u >= 'A' && u <= 'Z') token += static_cast<char>(u - 'A' + 'a');
            else if ((u >= 'a' && u <= 'z') || (u >= '0' && u <= '9')) token += c;
            else flush();
        }
        flush();
    }

    json::Object out;
    for (auto const& [tok, chs] : postings) {
        json::Array arr;
        arr.reserve(chs.size());
        for (auto const& cid : chs) arr.push_back(json::Value{cid});
        out.emplace(tok, json::Value{std::move(arr)});
    }
    return out;
}

inline std::string to_json(book::Book const& bk, bool pretty = false) {
    json::Object meta;
    meta.emplace("title",       json::Value{bk.meta.book.title});
    meta.emplace("language",    json::Value{bk.meta.book.language});
    meta.emplace("description", json::Value{bk.meta.book.description});
    json::Array authors;
    for (auto const& a : bk.meta.book.authors) authors.push_back(json::Value{a});
    meta.emplace("authors", json::Value{std::move(authors)});

    json::Object output;
    output.emplace("defaultTheme", json::Value{bk.meta.output.default_theme});
    output.emplace("search",       json::Value{bk.meta.output.search});
    if (!bk.meta.output.edit_url.empty())
        output.emplace("editUrl", json::Value{bk.meta.output.edit_url});

    json::Object toc;
    toc.emplace("prefix", detail::part_to_json(bk.toc.prefix));
    json::Array parts;
    for (auto const& p : bk.toc.parts) parts.push_back(detail::part_to_json(p));
    toc.emplace("parts",  json::Value{std::move(parts)});
    toc.emplace("suffix", detail::part_to_json(bk.toc.suffix));

    json::Object chapters;
    for (auto const& ch : bk.chapters) {
        json::Object co;
        co.emplace("title", json::Value{ch.title});
        co.emplace("path",  json::Value{ch.relative_path.generic_string()});
        json::Array blocks;
        for (auto r : ch.doc.roots) blocks.push_back(detail::block_to_json(ch.doc, r));
        co.emplace("blocks", json::Value{std::move(blocks)});
        chapters.emplace(ch.id, json::Value{std::move(co)});
    }

    json::Object root;
    root.emplace("schema",   json::Value{std::int64_t{1}});
    root.emplace("meta",     json::Value{std::move(meta)});
    root.emplace("output",   json::Value{std::move(output)});
    root.emplace("toc",      json::Value{std::move(toc)});
    root.emplace("chapters", json::Value{std::move(chapters)});
    if (bk.meta.output.search) {
        root.emplace("search", json::Value{build_search_index(bk)});
    }

    return json::emit(json::Value{std::move(root)}, pretty);
}

} // namespace emit
