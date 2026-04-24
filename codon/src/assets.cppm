export module assets;
import std;
import core;

export namespace assets {

// Copy every non-markdown file under `src` into `dest`, preserving the
// relative path. SUMMARY.md and .md/.markdown files are skipped — the
// generator consumes those directly.
inline core::Result<void> copy_tree(std::filesystem::path const& src,
                                    std::filesystem::path const& dest) {
    std::error_code ec;
    if (!std::filesystem::exists(src, ec)) return {};
    std::filesystem::create_directories(dest, ec);
    if (ec) return core::err("mkdir '{}': {}", dest.string(), ec.message());

    for (auto const& entry :
         std::filesystem::recursive_directory_iterator(src, ec)) {
        if (ec) return core::err("walking '{}': {}", src.string(), ec.message());
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().string();
        if (ext == ".md" || ext == ".markdown") continue;
        if (entry.path().filename() == "SUMMARY.md") continue;

        auto rel = std::filesystem::relative(entry.path(), src, ec);
        if (ec) return core::err("relpath '{}': {}", entry.path().string(), ec.message());
        auto target = dest / rel;

        auto parent = target.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, ec);
            if (ec) return core::err("mkdir '{}': {}", parent.string(), ec.message());
        }
        std::filesystem::copy_file(
            entry.path(), target,
            std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) return core::err("copy '{}' -> '{}': {}",
                                 entry.path().string(), target.string(), ec.message());
    }
    return {};
}

} // namespace assets
