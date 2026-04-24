export module commands;
import std;
import core;
import manifest;
import summary;
import markdown;
import book;
import emit;
import assets;

export namespace commands {

inline constexpr std::string_view VERSION = "0.1.0";

inline std::string usage_text() {
    return
        "codon — documentation-site generator for C++23 projects\n"
        "\n"
        "USAGE:\n"
        "    codon <command> [args]\n"
        "\n"
        "COMMANDS:\n"
        "    build <docs-dir> [dist-dir]   Build docs/ into dist/ (default dist/)\n"
        "    check <docs-dir>              Parse manifest + sources, no output\n"
        "    init  <docs-dir>              Scaffold a starter docs/ directory\n"
        "    version                       Print the codon version\n"
        "    help                          Print this message\n";
}

inline int cmd_version() {
    std::println("codon {}", VERSION);
    return 0;
}

inline int cmd_help() {
    std::print("{}", usage_text());
    return 0;
}

namespace detail {

inline std::filesystem::path docs_arg(int argc, char** argv, int index,
                                      std::filesystem::path const& fallback) {
    if (index < argc) return std::filesystem::path{argv[index]};
    return fallback;
}

inline int fail(std::string_view msg) {
    std::println(std::cerr, "error: {}", msg);
    return 1;
}

} // namespace detail

inline int cmd_build(int argc, char** argv) {
    auto docs_dir = detail::docs_arg(argc, argv, 2, "docs");
    auto dest_dir = detail::docs_arg(argc, argv, 3, "dist");
    auto book_toml = docs_dir / "book.toml";

    auto bk = book::assemble(book_toml);
    if (!bk) return detail::fail(bk.error());

    // Resolve the effective source directory (manifest.build.src).
    auto src_dir = docs_dir / bk->meta.build.src;

    std::error_code ec;
    std::filesystem::create_directories(dest_dir, ec);
    if (ec) return detail::fail(std::format("mkdir '{}': {}", dest_dir.string(), ec.message()));

    // Emit book.json.
    auto json_str = emit::to_json(*bk, /*pretty=*/false);
    auto json_path = dest_dir / "book.json";
    if (auto r = core::write_file(json_path, json_str); !r) return detail::fail(r.error());

    // Copy non-markdown assets into dist/assets/ (mirrors src/ layout minus .md).
    if (auto r = assets::copy_tree(src_dir, dest_dir / "assets"); !r)
        return detail::fail(r.error());

    std::println("codon: wrote {} chapter(s) to {}/book.json",
                 bk->chapters.size(), dest_dir.string());
    return 0;
}

inline int cmd_check(int argc, char** argv) {
    auto docs_dir = detail::docs_arg(argc, argv, 2, "docs");
    auto book_toml = docs_dir / "book.toml";
    auto bk = book::assemble(book_toml);
    if (!bk) return detail::fail(bk.error());
    std::println("codon: check ok — {} chapter(s)", bk->chapters.size());
    return 0;
}

inline int cmd_init(int argc, char** argv) {
    auto docs_dir = detail::docs_arg(argc, argv, 2, "docs");
    auto src_dir = docs_dir / "src";

    std::error_code ec;
    std::filesystem::create_directories(src_dir, ec);
    if (ec) return detail::fail(std::format("mkdir '{}': {}", src_dir.string(), ec.message()));

    auto book_toml = docs_dir / "book.toml";
    if (!std::filesystem::exists(book_toml)) {
        std::string content =
            "[book]\n"
            "title       = \"my project\"\n"
            "authors     = []\n"
            "language    = \"en\"\n"
            "description = \"\"\n"
            "\n"
            "[build]\n"
            "src  = \"src\"\n"
            "dest = \"book\"\n"
            "\n"
            "[output.html]\n"
            "default-theme = \"auto\"\n"
            "search        = true\n";
        if (auto r = core::write_file(book_toml, content); !r) return detail::fail(r.error());
    }

    auto summary_md = src_dir / "SUMMARY.md";
    if (!std::filesystem::exists(summary_md)) {
        std::string content =
            "# Summary\n"
            "\n"
            "[Introduction](introduction.md)\n"
            "\n"
            "---\n"
            "\n"
            "# Guide\n"
            "\n"
            "- [Getting started](guide/getting-started.md)\n";
        if (auto r = core::write_file(summary_md, content); !r) return detail::fail(r.error());
    }

    auto intro = src_dir / "introduction.md";
    if (!std::filesystem::exists(intro)) {
        std::string content =
            "# Introduction\n"
            "\n"
            "Welcome to your new book.\n";
        if (auto r = core::write_file(intro, content); !r) return detail::fail(r.error());
    }

    auto start = src_dir / "guide" / "getting-started.md";
    if (!std::filesystem::exists(start)) {
        std::string content =
            "# Getting started\n"
            "\n"
            "Write your first chapter in Markdown.\n";
        if (auto r = core::write_file(start, content); !r) return detail::fail(r.error());
    }

    std::println("codon: initialized {}", docs_dir.string());
    return 0;
}

} // namespace commands
