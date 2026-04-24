export module reader.view;
import std;
import phenotype;
import reader.book;

// The reader's phenotype view, split out so the state/update/runner
// wiring in main.cpp stays readable. Expects a `State` type defined
// below plus a `Msg` variant with Navigate / Search / ToggleSidebar /
// CycleTheme / OpenExternal alternatives.

export namespace reader::view {

// Flatten spans into a single string for v1 — inline emphasis, strong,
// and inline-code styling aren't specialised yet. Link spans contribute
// their label; image spans contribute their alt text.
inline std::string flatten_spans(std::vector<Span> const& spans) {
    std::string out;
    for (auto const& s : spans) {
        switch (s.kind) {
            case SpanKind::Code:
            case SpanKind::Text:
            case SpanKind::Emph:
            case SpanKind::Strong:
                out += s.text;
                break;
            case SpanKind::Link:
                out += s.text;
                break;
            case SpanKind::Image:
                out += "[";
                out += s.text;
                out += "]";
                break;
            case SpanKind::HardBreak:
                out += "\n";
                break;
        }
    }
    return out;
}

// Render a block and its descendants to the active phenotype column.
// BlockId is an index into `chapter.pool` built by reader::load.
template <class Msg>
inline void render_block(Chapter const& ch, BlockId id) {
    using namespace phenotype;
    auto const& b = ch.pool[id];
    switch (b.kind) {
        case BlockKind::Heading: {
            auto text = flatten_spans(b.spans);
            // Without heading-size variants yet, a blank spacer above
            // the heading text is the quickest visual cue.
            layout::spacer(static_cast<float>(24 - (b.level - 1) * 4));
            widget::text(text);
            break;
        }
        case BlockKind::Paragraph:
            widget::text(flatten_spans(b.spans));
            break;
        case BlockKind::CodeBlock:
            widget::code(b.text);
            break;
        case BlockKind::List:
            layout::list_items([&] {
                for (auto child : b.children) {
                    auto const& item = ch.pool[child];
                    std::string label;
                    for (auto grand : item.children) {
                        auto const& g = ch.pool[grand];
                        if (!label.empty()) label += " ";
                        label += flatten_spans(g.spans);
                    }
                    layout::item(label);
                }
            });
            break;
        case BlockKind::ListItem:
            // Rendered by its parent List; skip standalone.
            break;
        case BlockKind::Blockquote:
            layout::card([&] {
                for (auto child : b.children) render_block<Msg>(ch, child);
            });
            break;
        case BlockKind::ThematicBreak:
            layout::divider();
            break;
    }
}

template <class Msg, class NavMsg>
inline void render_toc_entries(std::vector<TocEntry> const& entries,
                               std::string const& active_id) {
    using namespace phenotype;
    for (auto const& e : entries) {
        std::string label;
        if (!e.number.empty()) {
            label += e.number;
            label += ". ";
        }
        label += e.title;
        if (e.draft) {
            widget::text(label + " (draft)");
        } else if (e.id == active_id) {
            // Current chapter — render as plain text so it reads like
            // a breadcrumb rather than a clickable item.
            widget::text("> " + label);
        } else {
            widget::button<Msg>(label, NavMsg{e.id});
        }
        if (!e.children.empty()) {
            layout::column([&] {
                render_toc_entries<Msg, NavMsg>(e.children, active_id);
            });
        }
    }
}

} // namespace reader::view
