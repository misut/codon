export module summary;
import std;
import core;

export namespace summary {

struct Entry {
    std::string                 title;
    std::filesystem::path       path;     // empty for draft
    std::vector<Entry>          children;
    std::string                 number;   // "1", "1.2", ...; empty for drafts & prefix/suffix
    std::string                 id;       // slug used for URLs
    bool                        draft = false;
};

struct Part {
    std::optional<std::string>  title;    // optional level-1 heading
    std::vector<Entry>          entries;
};

struct Summary {
    Part                        prefix;   // unnumbered entries before first list
    std::vector<Part>           parts;    // body parts (may be many)
    Part                        suffix;   // unnumbered entries after last list
};

namespace detail {

// Parse a `[text](href)` link at the start of `s`. On success, returns the
// consumed length and fills `text` / `href`. Returns 0 on miss.
inline std::size_t parse_link(std::string_view s, std::string& text, std::string& href) {
    if (s.size() < 2 || s[0] != '[') return 0;
    std::size_t i = 1;
    std::string buf;
    while (i < s.size() && s[i] != ']') {
        if (s[i] == '\\' && i + 1 < s.size()) { buf += s[i + 1]; i += 2; }
        else { buf += s[i]; ++i; }
    }
    if (i >= s.size() || s[i] != ']' || i + 1 >= s.size() || s[i + 1] != '(') return 0;
    text = std::move(buf);
    i += 2;
    std::string hbuf;
    while (i < s.size() && s[i] != ')') {
        if (s[i] == '\\' && i + 1 < s.size()) { hbuf += s[i + 1]; i += 2; }
        else { hbuf += s[i]; ++i; }
    }
    if (i >= s.size() || s[i] != ')') return 0;
    href = std::move(hbuf);
    return i + 1;
}

// Count leading spaces; tabs count as 4 for indentation purposes.
inline std::size_t indent_of(std::string_view s) {
    std::size_t n = 0;
    for (char c : s) {
        if (c == ' ') ++n;
        else if (c == '\t') n += 4;
        else break;
    }
    return n;
}

// Apply hierarchical numbering to a tree of numbered entries.
inline void assign_numbers(std::vector<Entry>& entries, std::string const& prefix_num) {
    std::size_t counter = 0;
    for (auto& e : entries) {
        if (e.draft) continue;
        ++counter;
        e.number = prefix_num.empty()
            ? std::to_string(counter)
            : std::format("{}.{}", prefix_num, counter);
        assign_numbers(e.children, e.number);
    }
}

inline void set_ids(std::vector<Entry>& entries) {
    for (auto& e : entries) {
        if (!e.path.empty()) e.id = core::chapter_id(e.path);
        set_ids(e.children);
    }
}

} // namespace detail

inline core::Result<Summary> parse(std::string_view content) {
    auto lines = core::split_lines(content);

    Summary s;
    enum class Phase { Prefix, Body, Suffix };
    Phase phase = Phase::Prefix;

    // Active numbered-list part (populated during Body phase).
    Part* active_part = nullptr;

    // Stack of (indent, entry*) for nested list items.
    struct StackFrame { std::size_t indent; std::vector<Entry>* children; };
    std::vector<StackFrame> stack;

    auto ensure_part = [&]() -> Part* {
        if (!active_part) {
            s.parts.emplace_back();
            active_part = &s.parts.back();
        }
        return active_part;
    };

    for (std::size_t li = 0; li < lines.size(); ++li) {
        auto line = lines[li];
        auto trimmed = core::trim(line);

        if (trimmed.empty()) continue;

        // '# Summary' at the top is decorative; swallow.
        if (trimmed.starts_with("#")) {
            std::size_t j = 0;
            while (j < trimmed.size() && trimmed[j] == '#') ++j;
            if (j >= trimmed.size() || trimmed[j] == ' ' || trimmed[j] == '\t') {
                auto heading = core::trim(trimmed.substr(j));
                if (phase == Phase::Prefix && s.prefix.entries.empty()
                        && heading == "Summary") {
                    continue;
                }
                // A level-1 heading starts a new part.
                if (j == 1) {
                    s.parts.emplace_back();
                    active_part = &s.parts.back();
                    active_part->title = std::string{heading};
                    phase = Phase::Body;
                    stack.clear();
                    continue;
                }
            }
            // Other heading levels are ignored.
            continue;
        }

        // Horizontal rule — separator.
        if (trimmed == "---" || trimmed == "***" || trimmed == "___") {
            active_part = nullptr;
            stack.clear();
            if (phase == Phase::Prefix) phase = Phase::Body;
            else phase = Phase::Suffix;
            continue;
        }

        // List item (numbered chapter).
        std::size_t indent = detail::indent_of(line);
        auto rest = core::trim(line);
        if (rest.starts_with("- ") || rest.starts_with("* ") || rest.starts_with("+ ")) {
            auto body = core::trim(rest.substr(2));
            std::string t, h;
            auto consumed = detail::parse_link(body, t, h);
            if (consumed == 0) {
                return core::err("SUMMARY.md line {}: expected '[Title](path)' list item", li + 1);
            }

            Entry e;
            e.title = std::move(t);
            if (h.empty()) {
                e.draft = true;
            } else {
                e.path = h;
            }

            auto* part = ensure_part();
            phase = Phase::Body;

            while (!stack.empty() && stack.back().indent >= indent) stack.pop_back();
            auto* target = stack.empty() ? &part->entries : stack.back().children;
            target->push_back(std::move(e));
            stack.push_back({ indent, &target->back().children });
            continue;
        }

        // Bare link: prefix/suffix entry.
        std::string t, h;
        auto consumed = detail::parse_link(trimmed, t, h);
        if (consumed == 0) {
            return core::err("SUMMARY.md line {}: unexpected '{}'", li + 1,
                             std::string{trimmed});
        }
        Entry e;
        e.title = std::move(t);
        e.path  = h;
        if (phase == Phase::Prefix) {
            s.prefix.entries.push_back(std::move(e));
        } else {
            phase = Phase::Suffix;
            active_part = nullptr;
            stack.clear();
            s.suffix.entries.push_back(std::move(e));
        }
    }

    detail::set_ids(s.prefix.entries);
    detail::set_ids(s.suffix.entries);
    for (auto& part : s.parts) detail::set_ids(part.entries);

    std::string prefix_num;
    for (auto& part : s.parts) {
        detail::assign_numbers(part.entries, prefix_num);
    }

    return s;
}

// Visit every entry in reading order (prefix → parts → suffix), calling
// `fn(entry, part)` where `part` is nullptr for prefix/suffix entries.
template <class Fn>
inline void walk(Summary const& s, Fn&& fn) {
    auto recurse = [&](auto& self, Entry const& e, Part const* p) -> void {
        fn(e, p);
        for (auto const& c : e.children) self(self, c, p);
    };
    for (auto const& e : s.prefix.entries) recurse(recurse, e, nullptr);
    for (auto const& p : s.parts)
        for (auto const& e : p.entries) recurse(recurse, e, &p);
    for (auto const& e : s.suffix.entries) recurse(recurse, e, nullptr);
}

} // namespace summary
