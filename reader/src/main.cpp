#include <concepts>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

import std;
import phenotype;
import reader.book;
import reader.load;
import reader.view;
import reader.sample;

namespace reader::app {

// ============================================================
// Messages — every user-facing event the reader emits.
// ============================================================

struct Navigate      { std::string id; };
struct Search        { std::string query; };
struct ToggleSidebar {};
struct CycleTheme    {};

using Msg = std::variant<Navigate, Search, ToggleSidebar, CycleTheme>;

// ============================================================
// State — the runner's single source of truth.
// ============================================================

struct State {
    reader::Book book;
    std::string  active_id;
    std::string  query;
    bool         sidebar_open = true;
    int          theme_step   = 0; // 0=auto, 1=light, 2=dark

    State() {
        // phenotype::run<State, Msg>(view, update) default-constructs the
        // state internally, so the book has to be loaded here instead of
        // via a separate factory. A follow-up PR will replace the bundled
        // sample with bytes fetched through a phenotype host import.
        book = reader::load(reader::sample::book_json);
        auto order = reader::flatten(book.toc);
        if (!order.empty()) active_id = order.front()->id;
    }
};

// Ranks chapters against a lowercase query using the book's inverted
// index; chapters whose title matches outrank body-only hits.
inline std::vector<std::string> search_hits(reader::Book const& b,
                                            std::string const& q) {
    if (q.empty()) return {};
    std::string lower;
    for (char c : q) {
        unsigned char u = static_cast<unsigned char>(c);
        lower += (u >= 'A' && u <= 'Z') ? static_cast<char>(u - 'A' + 'a') : c;
    }
    std::map<std::string, int> score;
    if (auto it = b.search_index.find(lower); it != b.search_index.end()) {
        for (auto const& cid : it->second) score[cid] += 1;
    }
    for (auto const& ch : b.chapters) {
        std::string t;
        for (char c : ch.title) {
            unsigned char u = static_cast<unsigned char>(c);
            t += (u >= 'A' && u <= 'Z') ? static_cast<char>(u - 'A' + 'a') : c;
        }
        if (t.find(lower) != std::string::npos) score[ch.id] += 10;
    }
    std::vector<std::pair<std::string, int>> ranked(score.begin(), score.end());
    std::sort(ranked.begin(), ranked.end(),
              [](auto const& a, auto const& b) { return a.second > b.second; });
    std::vector<std::string> out;
    out.reserve(ranked.size());
    for (auto const& [id, _] : ranked) out.push_back(id);
    return out;
}

// ============================================================
// update — pure state transition.
// ============================================================

inline void update(State& state, Msg msg) {
    std::visit([&](auto const& m) {
        using T = std::decay_t<decltype(m)>;
        if constexpr (std::same_as<T, Navigate>) {
            if (state.book.chapter(m.id)) state.active_id = m.id;
        } else if constexpr (std::same_as<T, Search>) {
            state.query = m.query;
        } else if constexpr (std::same_as<T, ToggleSidebar>) {
            state.sidebar_open = !state.sidebar_open;
        } else if constexpr (std::same_as<T, CycleTheme>) {
            state.theme_step = (state.theme_step + 1) % 3;
        }
    }, msg);
}

// ============================================================
// view — pure state → UI tree.
// ============================================================

inline std::size_t index_of(std::vector<reader::TocEntry const*> const& order,
                            std::string const& id) {
    for (std::size_t i = 0; i < order.size(); ++i)
        if (order[i]->id == id) return i;
    return order.size();
}

inline void view(State const& state) {
    using namespace phenotype;
    auto const& book = state.book;
    auto order = reader::flatten(book.toc);

    auto idx = index_of(order, state.active_id);
    auto const* chapter = book.chapter(state.active_id);

    layout::scaffold(
        // Top bar — title + sidebar toggle + theme toggle + search
        [&] {
            widget::text(book.meta.title);
            if (!book.meta.description.empty()) {
                widget::text(book.meta.description);
            }
            layout::row(
                [&] { widget::button<Msg>("☰ sidebar", ToggleSidebar{}); },
                [&] {
                    std::string label = "theme: ";
                    label += (state.theme_step == 0 ? "auto"
                            : state.theme_step == 1 ? "light" : "dark");
                    widget::button<Msg>(label, CycleTheme{});
                },
                [&] {
                    widget::text_field<Msg>(
                        "search…", state.query,
                        +[](std::string q) -> Msg { return Search{std::move(q)}; });
                }
            );
        },
        // Main content — sidebar + chapter pane, or search results
        [&] {
            layout::row(
                // Sidebar
                [&] {
                    if (!state.sidebar_open) return;
                    layout::sized_box(260.0f, [&] {
                        layout::column([&] {
                            if (!book.toc.prefix.entries.empty()) {
                                reader::view::render_toc_entries<Msg, Navigate>(
                                    book.toc.prefix.entries, state.active_id);
                                layout::divider();
                            }
                            for (auto const& part : book.toc.parts) {
                                if (part.title) widget::text(*part.title);
                                reader::view::render_toc_entries<Msg, Navigate>(
                                    part.entries, state.active_id);
                                layout::divider();
                            }
                            if (!book.toc.suffix.entries.empty()) {
                                reader::view::render_toc_entries<Msg, Navigate>(
                                    book.toc.suffix.entries, state.active_id);
                            }
                        });
                    });
                },
                // Content pane
                [&] {
                    layout::column([&] {
                        if (!state.query.empty()) {
                            widget::text("Search results:");
                            auto hits = search_hits(book, state.query);
                            if (hits.empty()) widget::text("(no matches)");
                            for (auto const& cid : hits) {
                                auto const* ch = book.chapter(cid);
                                if (!ch) continue;
                                widget::button<Msg>(ch->title, Navigate{cid});
                            }
                            layout::divider();
                        }
                        if (chapter) {
                            widget::text(chapter->title);
                            for (auto r : chapter->roots) {
                                reader::view::render_block<Msg>(*chapter, r);
                            }
                        } else {
                            widget::text("(select a chapter)");
                        }
                    });
                }
            );
        },
        // Bottom — prev/next strip
        [&] {
            layout::row(
                [&] {
                    if (idx > 0 && idx != order.size()) {
                        auto const* prev = order[idx - 1];
                        widget::button<Msg>(std::string{"← "} + prev->title,
                                            Navigate{prev->id});
                    }
                },
                [&] { widget::text(" · "); },
                [&] {
                    if (idx + 1 < order.size()) {
                        auto const* next = order[idx + 1];
                        widget::button<Msg>(next->title + std::string{" →"},
                                            Navigate{next->id});
                    }
                }
            );
        }
    );
}

} // namespace reader::app

int main() {
    phenotype::run<reader::app::State, reader::app::Msg>(
        reader::app::view, reader::app::update);
    return 0;
}
