export module markdown;
import std;
import core;

export namespace markdown {

using BlockId = std::uint32_t;
inline constexpr BlockId invalid_block = ~BlockId{0};

enum class SpanKind : int { Text, Emph, Strong, Code, Link, Image, HardBreak };

struct Span {
    SpanKind    kind = SpanKind::Text;
    std::string text;   // visible body (alt for Image, label for Link)
    std::string href;   // Link target / Image src
};

enum class BlockKind : int {
    Heading, Paragraph, CodeBlock, List, ListItem, Blockquote, ThematicBreak
};

struct Block {
    BlockKind             kind = BlockKind::Paragraph;
    int                   level = 0;        // Heading: 1-6
    std::string           anchor;           // Heading
    std::vector<Span>     spans;            // Heading, Paragraph
    std::string           language;         // CodeBlock info string
    std::string           text;             // CodeBlock raw body
    bool                  ordered = false;  // List
    std::vector<BlockId>  children;         // List: items; ListItem/Blockquote: blocks
};

struct Document {
    std::vector<Block>    pool;
    std::vector<BlockId>  roots;
};

namespace detail {

// Escape-aware read up to a closing delimiter. Returns the length consumed
// (including the closing delim) and writes the inner body into `out`.
// Returns 0 if unterminated.
inline std::size_t read_until(std::string_view s, std::size_t start, char close,
                              std::string& out) {
    std::string body;
    std::size_t i = start;
    while (i < s.size() && s[i] != close) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            body += s[i + 1];
            i += 2;
        } else {
            body += s[i];
            ++i;
        }
    }
    if (i >= s.size()) return 0;
    out = std::move(body);
    return (i + 1) - start;
}

// Parse inline-level spans from a line/paragraph of text into `spans`.
// Supports: *emph*, **strong**, `code`, [text](href), ![alt](src).
inline void parse_inline(std::string_view text, std::vector<Span>& spans) {
    std::string buffer;
    auto flush_text = [&] {
        if (!buffer.empty()) {
            spans.push_back({ SpanKind::Text, buffer, "" });
            buffer.clear();
        }
    };

    std::size_t i = 0;
    while (i < text.size()) {
        char c = text[i];

        if (c == '\\' && i + 1 < text.size()) {
            buffer += text[i + 1];
            i += 2;
            continue;
        }

        if (c == '`') {
            // Inline code. Match a run of backticks and close on the same run length.
            std::size_t j = i;
            while (j < text.size() && text[j] == '`') ++j;
            auto run = j - i;
            // Find a matching run.
            std::size_t k = j;
            while (k + run <= text.size()) {
                if (std::string_view{text}.substr(k, run) == std::string(run, '`')) {
                    // Ensure it's exactly `run` backticks (not longer).
                    if (k + run == text.size() || text[k + run] != '`') {
                        std::string body{text.substr(j, k - j)};
                        // Strip single leading/trailing space per CommonMark 6.1.
                        if (body.size() >= 2 && body.front() == ' ' && body.back() == ' ') {
                            body = body.substr(1, body.size() - 2);
                        }
                        flush_text();
                        spans.push_back({ SpanKind::Code, body, "" });
                        i = k + run;
                        goto next;
                    }
                }
                ++k;
            }
            // No closing run — treat as literal.
            buffer.append(text.substr(i, run));
            i = j;
        next:
            continue;
        }

        if (c == '!' && i + 1 < text.size() && text[i + 1] == '[') {
            // Image ![alt](src)
            std::string alt;
            auto consumed = read_until(text, i + 2, ']', alt);
            if (consumed > 0 && i + 2 + consumed < text.size()
                    && text[i + 2 + consumed] == '(') {
                std::string src;
                auto href_consumed = read_until(text, i + 2 + consumed + 1, ')', src);
                if (href_consumed > 0) {
                    flush_text();
                    spans.push_back({ SpanKind::Image, alt, src });
                    i += 2 + consumed + 1 + href_consumed;
                    continue;
                }
            }
            buffer += c;
            ++i;
            continue;
        }

        if (c == '[') {
            // Link [text](href)
            std::string label;
            auto consumed = read_until(text, i + 1, ']', label);
            if (consumed > 0 && i + 1 + consumed < text.size()
                    && text[i + 1 + consumed] == '(') {
                std::string href;
                auto href_consumed = read_until(text, i + 1 + consumed + 1, ')', href);
                if (href_consumed > 0) {
                    flush_text();
                    spans.push_back({ SpanKind::Link, label, href });
                    i += 1 + consumed + 1 + href_consumed;
                    continue;
                }
            }
            buffer += c;
            ++i;
            continue;
        }

        if (c == '*' || c == '_') {
            // ** or __ -> strong; single -> emph. No nesting in v1.
            bool doubled = (i + 1 < text.size() && text[i + 1] == c);
            auto marker = doubled ? std::string(2, c) : std::string(1, c);
            std::size_t search_start = i + marker.size();
            auto close = text.find(marker, search_start);
            if (close != std::string::npos && close > search_start) {
                std::string inner{text.substr(search_start, close - search_start)};
                flush_text();
                spans.push_back({
                    doubled ? SpanKind::Strong : SpanKind::Emph,
                    inner, ""
                });
                i = close + marker.size();
                continue;
            }
            buffer += c;
            ++i;
            continue;
        }

        buffer += c;
        ++i;
    }
    flush_text();
}

inline BlockId emit(Document& doc, Block&& b) {
    auto id = static_cast<BlockId>(doc.pool.size());
    doc.pool.push_back(std::move(b));
    return id;
}

struct LineCursor {
    std::vector<std::string_view> const& lines;
    std::size_t i = 0;
    bool at_end() const { return i >= lines.size(); }
    std::string_view peek() const { return lines[i]; }
    std::string_view take() { return lines[i++]; }
};

// Fenced code fence? Returns language (or empty) and the fence char/run.
struct Fence {
    bool    match   = false;
    char    ch      = '`';
    std::size_t length = 0;
    std::string lang;
};

inline Fence detect_fence(std::string_view line) {
    auto t = core::trim(line);
    Fence f;
    if (t.size() < 3) return f;
    char c = t.front();
    if (c != '`' && c != '~') return f;
    std::size_t n = 0;
    while (n < t.size() && t[n] == c) ++n;
    if (n < 3) return f;
    f.match = true;
    f.ch = c;
    f.length = n;
    f.lang = std::string{core::trim(t.substr(n))};
    return f;
}

inline bool is_thematic_break(std::string_view line) {
    auto t = core::trim(line);
    if (t.size() < 3) return false;
    char c = t.front();
    if (c != '-' && c != '*' && c != '_') return false;
    std::size_t count = 0;
    for (char ch : t) {
        if (ch == c) ++count;
        else if (ch == ' ' || ch == '\t') continue;
        else return false;
    }
    return count >= 3;
}

inline int heading_level(std::string_view line, std::string& body) {
    std::size_t i = 0;
    while (i < line.size() && line[i] == ' ') ++i;
    std::size_t h = 0;
    while (i + h < line.size() && line[i + h] == '#') ++h;
    if (h < 1 || h > 6) return 0;
    if (i + h < line.size() && line[i + h] != ' ' && line[i + h] != '\t') return 0;
    auto rest = core::trim(line.substr(i + h));
    // Strip trailing '#' run (optional closing sequence).
    while (!rest.empty() && rest.back() == '#') rest.remove_suffix(1);
    body = std::string{core::trim(rest)};
    return static_cast<int>(h);
}

struct BlockquoteScan {
    bool match = false;
    std::string_view body;
};
inline BlockquoteScan detect_blockquote(std::string_view line) {
    auto t = line;
    std::size_t i = 0;
    while (i < t.size() && (t[i] == ' ')) ++i;
    if (i < t.size() && t[i] == '>') {
        ++i;
        if (i < t.size() && t[i] == ' ') ++i;
        return { true, t.substr(i) };
    }
    return {};
}

struct ListItemScan {
    bool        match    = false;
    bool        ordered  = false;
    std::size_t indent   = 0;      // leading space count
    std::size_t marker_width = 0;  // '- ' = 2, '1. ' = 3, etc.
    std::string_view body;
};
inline ListItemScan detect_list_item(std::string_view line) {
    ListItemScan r;
    std::size_t i = 0;
    while (i < line.size() && line[i] == ' ') ++i;
    r.indent = i;
    if (i >= line.size()) return r;
    // Unordered: -, *, +
    if ((line[i] == '-' || line[i] == '*' || line[i] == '+')
            && i + 1 < line.size() && line[i + 1] == ' ') {
        r.match = true;
        r.ordered = false;
        r.marker_width = 2;
        r.body = line.substr(i + 2);
        return r;
    }
    // Ordered: digits + '.' or ')'
    std::size_t n = i;
    while (n < line.size() && line[n] >= '0' && line[n] <= '9') ++n;
    if (n > i && n < line.size() && (line[n] == '.' || line[n] == ')')
            && n + 1 < line.size() && line[n + 1] == ' ') {
        r.match = true;
        r.ordered = true;
        r.marker_width = (n - i) + 2;
        r.body = line.substr(n + 2);
        return r;
    }
    return r;
}

// Forward declaration: recursive parser.
void parse_blocks(LineCursor& cur, Document& doc, std::vector<BlockId>& out);

inline BlockId parse_list(LineCursor& cur, Document& doc) {
    // Pre: current line is a list-item line.
    auto first = detect_list_item(cur.peek());
    Block list;
    list.kind = BlockKind::List;
    list.ordered = first.ordered;
    auto base_indent = first.indent;

    while (!cur.at_end()) {
        auto line = cur.peek();
        auto scan = detect_list_item(line);
        if (!scan.match || scan.indent != base_indent) break;
        if (scan.ordered != list.ordered) break;

        cur.take();
        // Collect continuation lines belonging to this item (lines indented
        // at least `scan.indent + scan.marker_width`, or blank lines
        // followed by more indented content).
        std::vector<std::string_view> item_lines;
        item_lines.push_back(scan.body);
        std::size_t cont_indent = scan.indent + scan.marker_width;
        while (!cur.at_end()) {
            auto next = cur.peek();
            if (core::is_blank(next)) {
                // Peek one past the blank — continuation only if the line after is
                // indented or another list item under this one.
                if (cur.i + 1 >= cur.lines.size()) break;
                auto after = cur.lines[cur.i + 1];
                auto after_indent = 0u;
                for (char c : after) {
                    if (c == ' ') ++after_indent;
                    else break;
                }
                auto next_item = detect_list_item(after);
                if (after_indent >= cont_indent ||
                    (next_item.match && next_item.indent > base_indent)) {
                    cur.take(); // consume blank
                    item_lines.push_back("");
                    continue;
                }
                break;
            }
            auto nindent = 0u;
            for (char c : next) {
                if (c == ' ') ++nindent;
                else break;
            }
            auto sub = detect_list_item(next);
            if (sub.match && sub.indent == base_indent) break;       // sibling
            if (nindent < cont_indent && !sub.match) break;          // dedent
            cur.take();
            if (nindent >= cont_indent) {
                item_lines.push_back(next.substr(cont_indent));
            } else {
                // nested list-item line keeps its own indent
                item_lines.push_back(next);
            }
        }

        Block item;
        item.kind = BlockKind::ListItem;
        LineCursor sub_cur{ item_lines, 0 };
        parse_blocks(sub_cur, doc, item.children);
        list.children.push_back(emit(doc, std::move(item)));
    }

    return emit(doc, std::move(list));
}

inline BlockId parse_blockquote(LineCursor& cur, Document& doc) {
    std::vector<std::string_view> inner;
    // Owning buffer for stripped '> ' prefixes.
    std::vector<std::string> storage;
    while (!cur.at_end()) {
        auto scan = detect_blockquote(cur.peek());
        if (!scan.match) break;
        cur.take();
        storage.emplace_back(scan.body);
    }
    inner.reserve(storage.size());
    for (auto const& s : storage) inner.push_back(s);

    Block bq;
    bq.kind = BlockKind::Blockquote;
    LineCursor sub{ inner, 0 };
    parse_blocks(sub, doc, bq.children);
    return emit(doc, std::move(bq));
}

inline void parse_blocks(LineCursor& cur, Document& doc, std::vector<BlockId>& out) {
    while (!cur.at_end()) {
        auto line = cur.peek();

        if (core::is_blank(line)) {
            cur.take();
            continue;
        }

        // Fenced code.
        if (auto f = detect_fence(line); f.match) {
            cur.take();
            Block cb;
            cb.kind = BlockKind::CodeBlock;
            cb.language = f.lang;
            std::string body;
            while (!cur.at_end()) {
                auto cl = cur.take();
                auto t = core::trim(cl);
                if (t.size() >= f.length) {
                    bool all_fence = true;
                    for (std::size_t k = 0; k < t.size(); ++k) {
                        if (t[k] != f.ch) { all_fence = false; break; }
                    }
                    if (all_fence && t.size() >= f.length) break;
                }
                body.append(cl);
                body.push_back('\n');
            }
            if (!body.empty() && body.back() == '\n') body.pop_back();
            cb.text = std::move(body);
            out.push_back(emit(doc, std::move(cb)));
            continue;
        }

        // ATX heading.
        {
            std::string body;
            if (auto lvl = heading_level(line, body); lvl > 0) {
                cur.take();
                Block h;
                h.kind = BlockKind::Heading;
                h.level = lvl;
                h.anchor = core::slugify(body);
                detail::parse_inline(body, h.spans);
                out.push_back(emit(doc, std::move(h)));
                continue;
            }
        }

        // Thematic break.
        if (is_thematic_break(line)) {
            cur.take();
            Block tb; tb.kind = BlockKind::ThematicBreak;
            out.push_back(emit(doc, std::move(tb)));
            continue;
        }

        // Blockquote.
        if (detect_blockquote(line).match) {
            out.push_back(parse_blockquote(cur, doc));
            continue;
        }

        // List.
        if (detect_list_item(line).match) {
            out.push_back(parse_list(cur, doc));
            continue;
        }

        // Paragraph — consume non-blank, non-block-starting lines.
        std::string paragraph;
        while (!cur.at_end()) {
            auto pl = cur.peek();
            if (core::is_blank(pl)) break;
            if (detect_fence(pl).match) break;
            {
                std::string ignored;
                if (heading_level(pl, ignored) > 0) break;
            }
            if (is_thematic_break(pl)) break;
            if (detect_blockquote(pl).match) break;
            if (detect_list_item(pl).match) break;
            if (!paragraph.empty()) paragraph.push_back(' ');
            paragraph.append(core::trim(pl));
            cur.take();
        }
        Block p;
        p.kind = BlockKind::Paragraph;
        detail::parse_inline(paragraph, p.spans);
        out.push_back(emit(doc, std::move(p)));
    }
}

// Resolve `{{#include path}}` directives by inlining file contents from
// `chapter_dir`. Skips occurrences inside fenced code blocks and inline
// code spans so docs that describe the directive literally still parse.
inline core::Result<std::string> expand_includes(
        std::string const& content,
        std::filesystem::path const& chapter_dir) {
    auto lines = core::split_lines(content);
    std::string out;
    out.reserve(content.size());

    std::string fence;  // empty while not inside a fenced block

    auto emit_with_inline_code_skipped =
        [&](std::string_view line, std::size_t line_no) -> core::Result<void> {
        std::size_t i = 0;
        while (i < line.size()) {
            if (line[i] == '`') {
                std::size_t j = i;
                while (j < line.size() && line[j] == '`') ++j;
                auto run = j - i;
                std::size_t close = std::string::npos;
                for (std::size_t k = j; k + run <= line.size(); ++k) {
                    bool match = true;
                    for (std::size_t n = 0; n < run; ++n)
                        if (line[k + n] != '`') { match = false; break; }
                    if (!match) continue;
                    if (k + run != line.size() && line[k + run] == '`') continue;
                    close = k;
                    break;
                }
                if (close != std::string::npos) {
                    out.append(line.substr(i, close + run - i));
                    i = close + run;
                    continue;
                }
                out.append(line.substr(i, run));
                i = j;
                continue;
            }
            if (line.substr(i).starts_with("{{#include")) {
                auto close = line.find("}}", i);
                if (close == std::string::npos)
                    return core::err("line {}: unterminated {{{{#include}}}}", line_no);
                auto inner = line.substr(i + 10, close - (i + 10));
                auto target = std::string{core::trim(inner)};
                if (target.empty())
                    return core::err("line {}: empty {{{{#include}}}} path", line_no);
                auto body = core::read_file(chapter_dir / target);
                if (!body) return std::unexpected(body.error());
                out.append(*body);
                i = close + 2;
                continue;
            }
            out.push_back(line[i]);
            ++i;
        }
        return {};
    };

    for (std::size_t li = 0; li < lines.size(); ++li) {
        auto line = lines[li];
        auto trimmed = core::trim(line);

        if (!fence.empty()) {
            out.append(line);
            out.push_back('\n');
            if (trimmed.size() >= fence.size()) {
                bool all = true;
                for (char c : trimmed) if (c != fence[0]) { all = false; break; }
                if (all) fence.clear();
            }
            continue;
        }

        if (trimmed.size() >= 3 &&
            (trimmed[0] == '`' || trimmed[0] == '~')) {
            char c = trimmed[0];
            std::size_t n = 0;
            while (n < trimmed.size() && trimmed[n] == c) ++n;
            if (n >= 3) {
                fence = std::string(n, c);
                out.append(line);
                out.push_back('\n');
                continue;
            }
        }

        if (auto r = emit_with_inline_code_skipped(line, li + 1); !r)
            return std::unexpected(r.error());
        out.push_back('\n');
    }

    return out;
}

} // namespace detail

inline core::Result<Document> parse(std::string_view content) {
    Document doc;
    auto lines = core::split_lines(content);
    detail::LineCursor cur{ lines, 0 };
    detail::parse_blocks(cur, doc, doc.roots);
    return doc;
}

inline core::Result<Document> parse_file(std::filesystem::path const& file) {
    auto content = core::read_file(file);
    if (!content) return std::unexpected(content.error());
    auto expanded = detail::expand_includes(*content, file.parent_path());
    if (!expanded) return std::unexpected(expanded.error());
    return parse(*expanded);
}

} // namespace markdown
