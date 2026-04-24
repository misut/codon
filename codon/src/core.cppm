export module core;
import std;

export namespace core {

template <class T>
using Result = std::expected<T, std::string>;

inline std::unexpected<std::string> err(std::string_view msg) {
    return std::unexpected(std::string{msg});
}

template <class... A>
inline std::unexpected<std::string> err(std::format_string<A...> fmt, A&&... a) {
    return std::unexpected(std::format(fmt, std::forward<A>(a)...));
}

inline Result<std::string> read_file(std::filesystem::path const& p) {
    std::ifstream in{p, std::ios::binary};
    if (!in) return err("cannot open '{}'", p.string());
    std::ostringstream ss;
    ss << in.rdbuf();
    if (!in && !in.eof()) return err("read failed: '{}'", p.string());
    return ss.str();
}

inline Result<void> write_file(std::filesystem::path const& p, std::string_view content) {
    std::error_code ec;
    if (auto parent = p.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        if (ec) return err("mkdir '{}': {}", parent.string(), ec.message());
    }
    std::ofstream out{p, std::ios::binary};
    if (!out) return err("cannot open '{}' for write", p.string());
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out) return err("write failed: '{}'", p.string());
    return {};
}

inline Result<void> copy_file(std::filesystem::path const& from,
                              std::filesystem::path const& to) {
    std::error_code ec;
    if (auto parent = to.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        if (ec) return err("mkdir '{}': {}", parent.string(), ec.message());
    }
    std::filesystem::copy_file(
        from, to,
        std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) return err("copy '{}' -> '{}': {}", from.string(), to.string(), ec.message());
    return {};
}

// Lowercase, replace non-alphanumerics with '-', collapse runs, trim edges.
inline std::string slugify(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    bool last_dash = true;
    for (char c : text) {
        unsigned char u = static_cast<unsigned char>(c);
        if ((u >= 'A' && u <= 'Z')) {
            out += static_cast<char>(u - 'A' + 'a');
            last_dash = false;
        } else if ((u >= 'a' && u <= 'z') || (u >= '0' && u <= '9')) {
            out += c;
            last_dash = false;
        } else if (!last_dash) {
            out += '-';
            last_dash = true;
        }
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out;
}

// Canonical chapter id: path relative to src/ with extension stripped,
// '/' replaced by '-', lowercased via slugify to stay URL-safe.
inline std::string chapter_id(std::filesystem::path const& relative) {
    auto s = relative.generic_string();
    if (auto dot = s.rfind('.'); dot != std::string::npos && dot + 1 < s.size()) {
        auto ext = s.substr(dot);
        if (ext == ".md" || ext == ".markdown") s.erase(dot);
    }
    for (auto& c : s) if (c == '/') c = '-';
    return slugify(s);
}

inline std::vector<std::string_view> split_lines(std::string_view content) {
    std::vector<std::string_view> lines;
    lines.reserve(content.size() / 32 + 4);
    std::size_t start = 0;
    for (std::size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\n') {
            auto end = i;
            if (end > start && content[end - 1] == '\r') --end;
            lines.push_back(content.substr(start, end - start));
            start = i + 1;
        }
    }
    if (start < content.size()) {
        auto end = content.size();
        if (end > start && content[end - 1] == '\r') --end;
        lines.push_back(content.substr(start, end - start));
    }
    return lines;
}

inline std::string_view trim(std::string_view s) {
    std::size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    std::size_t j = s.size();
    while (j > i && (s[j - 1] == ' ' || s[j - 1] == '\t')) --j;
    return s.substr(i, j - i);
}

inline bool is_blank(std::string_view s) {
    for (char c : s) if (c != ' ' && c != '\t') return false;
    return true;
}

} // namespace core
