export module manifest;
import std;
import toml;
import core;

export namespace manifest {

struct Output {
    std::string default_theme = "auto"; // "auto" | "light" | "dark"
    bool        search        = true;
    std::string edit_url;               // {path} placeholder gets replaced per chapter
};

struct Build {
    std::filesystem::path src  = "src";
    std::filesystem::path dest = "book";
};

struct Book {
    std::string              title;
    std::vector<std::string> authors;
    std::string              language    = "en";
    std::string              description;
};

struct Manifest {
    Book   book;
    Build  build;
    Output output;
};

inline core::Result<Manifest> load(std::filesystem::path const& path) {
    auto content = core::read_file(path);
    if (!content) return std::unexpected(content.error());

    // On native builds tomlcpp throws ParseError; we catch and convert to
    // Result. On WASM (-fno-exceptions) tomlcpp prints a diagnostic and
    // aborts via detail::fail() — so this call is abort-or-succeed there.
    toml::Table root;
#if __cpp_exceptions
    try {
        root = toml::parse(*content);
    } catch (toml::ParseError const& e) {
        return core::err("{}: {}", path.string(), e.what());
    } catch (std::exception const& e) {
        return core::err("{}: {}", path.string(), e.what());
    }
#else
    root = toml::parse(*content);
#endif

    Manifest m;

    if (!root.contains("book")) {
        return core::err("{}: missing [book] table", path.string());
    }
    auto const& book_val = root.at("book");
    if (!book_val.is_table()) {
        return core::err("{}: [book] must be a table", path.string());
    }
    auto const& book = book_val.as_table();

    if (auto it = book.find("title"); it != book.end()) {
        if (!it->second.is_string()) return core::err("book.title must be a string");
        m.book.title = it->second.as_string();
    }
    if (auto it = book.find("language"); it != book.end()) {
        if (!it->second.is_string()) return core::err("book.language must be a string");
        m.book.language = it->second.as_string();
    }
    if (auto it = book.find("description"); it != book.end()) {
        if (!it->second.is_string()) return core::err("book.description must be a string");
        m.book.description = it->second.as_string();
    }
    if (auto it = book.find("authors"); it != book.end()) {
        if (!it->second.is_array()) return core::err("book.authors must be an array");
        for (auto const& v : it->second.as_array()) {
            if (!v.is_string()) return core::err("book.authors entries must be strings");
            m.book.authors.push_back(v.as_string());
        }
    }

    if (auto it = root.find("build"); it != root.end()) {
        if (!it->second.is_table()) return core::err("[build] must be a table");
        auto const& bt = it->second.as_table();
        if (auto s = bt.find("src"); s != bt.end()) {
            if (!s->second.is_string()) return core::err("build.src must be a string");
            m.build.src = s->second.as_string();
        }
        if (auto d = bt.find("dest"); d != bt.end()) {
            if (!d->second.is_string()) return core::err("build.dest must be a string");
            m.build.dest = d->second.as_string();
        }
    }

    if (auto it = root.find("output"); it != root.end()) {
        if (!it->second.is_table()) return core::err("[output] must be a table");
        auto const& out = it->second.as_table();
        if (auto h = out.find("html"); h != out.end()) {
            if (!h->second.is_table()) return core::err("[output.html] must be a table");
            auto const& html = h->second.as_table();
            if (auto t = html.find("default-theme"); t != html.end()) {
                if (!t->second.is_string()) return core::err("output.html.default-theme must be a string");
                auto const& v = t->second.as_string();
                if (v != "auto" && v != "light" && v != "dark") {
                    return core::err("output.html.default-theme must be 'auto', 'light', or 'dark'");
                }
                m.output.default_theme = v;
            }
            if (auto s = html.find("search"); s != html.end()) {
                if (!s->second.is_bool()) return core::err("output.html.search must be a bool");
                m.output.search = s->second.as_bool();
            }
            if (auto e = html.find("edit-url"); e != html.end()) {
                if (!e->second.is_string()) return core::err("output.html.edit-url must be a string");
                m.output.edit_url = e->second.as_string();
            }
        }
    }

    if (m.book.title.empty()) {
        return core::err("{}: book.title is required", path.string());
    }
    return m;
}

} // namespace manifest
