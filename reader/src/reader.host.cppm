module;

// Host imports the codon shell provides through phenotype 0.14.0's
// `extraImports` hook in `mount()`. Declared in the global module
// fragment so the `import_module` / `import_name` attributes survive;
// WASI resolves these to the `codon` namespace the shell wires up at
// mount time.

#ifdef __wasi__
#define CODON_IMPORT(name) \
    __attribute__((import_module("codon"), import_name(name)))
#else
#define CODON_IMPORT(name)
#endif

#ifdef __cplusplus
extern "C" {
#endif

CODON_IMPORT("book_size")
unsigned int codon_book_size(void);

CODON_IMPORT("book_read")
unsigned int codon_book_read(char* out, unsigned int cap);

#ifdef __cplusplus
}
#endif

#undef CODON_IMPORT

export module reader.host;
import std;

export namespace reader::host {

// Pull the full book.json payload out of the shell in one blocking
// read. Safe to call repeatedly — the shell keeps the fetched bytes
// alive for the lifetime of the page — but the reader's State only
// calls it once at construction.
inline std::string read_book() {
    auto size = ::codon_book_size();
    std::string out;
    out.resize(size);
    auto actual = ::codon_book_read(out.data(), static_cast<unsigned int>(size));
    out.resize(actual);
    return out;
}

} // namespace reader::host
