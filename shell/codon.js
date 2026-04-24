// codon.js — thin ES-module wrapper around phenotype.js's mount()
// that streams a generator-emitted book.json into reader.wasm.
//
// The reader declares two WASM imports under the `codon` namespace
// (see reader/src/reader.host.cppm):
//
//     codon.book_size() -> u32
//     codon.book_read(ptr: u32, cap: u32) -> u32
//
// phenotype 0.14.0's mount() grew an `extraImports` hook for exactly
// this pattern: we fetch book.json once, keep the bytes alive for
// the page, and service the two imports against the instance's
// linear memory.
//
// Usage from index.html:
//
//     <script type="module">
//       import { mountBook } from './codon.js';
//       mountBook().catch(e => console.error(e));
//     </script>

import { mount } from './phenotype.js';

export async function mountBook(
    wasmUrl = './reader.wasm',
    bookUrl = './book.json',
    rootElement = document.body) {
    const resp = await fetch(bookUrl);
    if (!resp.ok) {
        throw new Error(`codon: failed to fetch ${bookUrl}: ${resp.status} ${resp.statusText}`);
    }
    const bookBytes = new Uint8Array(await resp.arrayBuffer());

    return mount(wasmUrl, rootElement, ({ getMemory }) => ({
        codon: {
            book_size: () => bookBytes.length,

            book_read: (ptr, cap) => {
                const n = Math.min(bookBytes.length, cap);
                const dst = new Uint8Array(getMemory().buffer, ptr, n);
                dst.set(bookBytes.subarray(0, n));
                return n;
            },
        },
    }));
}
