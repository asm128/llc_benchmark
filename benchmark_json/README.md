# JSON comparison benchmark

Build `llc_benchmark.slnx` in Release x64. The solution builds `llc.lib` before both applications. `benchmark_json` compiles JsonCpp's three implementation files and includes the pinned nlohmann JSON and RapidJSON headers from the LLB submodules.

Run `benchmark_json.exe --input=corpus.json --iterations=100`. Without `--input`, it uses a small built-in object. CSV output gives document bytes, iteration count, elapsed seconds, and MiB/s. File loading and initial serialization validation are outside the timed loops. Each parse iteration constructs a fresh document. Each write iteration constructs a fresh output buffer from a pre-parsed document.

Use `--requests=N` to set the cached response count separately from cold parse and write iterations. The program repeats the input object eight times to form a block, parses that block once per library, then answers a range request for records 2 through 5. Every timed request constructs a JSON array response. LLC copies the contiguous source span using its cached views; the other libraries select the same four values from their parsed documents and serialize them. All four responses are parsed and checked for semantic equality before timing. The rows omit file I/O and networking. Allocation counts, retained memory, and block parse cost are not included in the request rate.

Each timed response is passed through LLC's CRC function and contributes to the printed checksum sum. This consumes every response byte and adds the same full-byte pass to all four implementations. The request rates therefore measure response construction plus CRC, not network delivery.

LLC's CRC sum does not preserve byte order. Equal sums do not establish byte-identical responses. The benchmark checks semantic JSON equality before timing; object key order may differ between libraries.

For an array input, use `--range-start=N --range-count=N` to select a range. Add `--expected=path.json` to require the selected output to equal an independently saved response semantically before timing. For example, two decoded 128-record Season blocks can be combined into one 256-record input, then `--range-start=124 --range-count=8` selects the eight records spanning the original block boundary. This lets the library comparison produce the same response as a Blitter query.

For an uncompressed Season `.sson` block, pass `--secure-input` with `--input`. This verifies and removes the eight-byte CRC using LLC's `fileToMemorySecure()` before any parser sees the payload. All four libraries then receive identical JSON bytes. The CRC and file read are outside the timed loops.

`llc_tree`, `jsoncpp`, `nlohmann`, and `rapidjson` compare parse plus document construction. `llc_tokens` omits views and tree construction and is a separate workload. The four `_write` rows serialize pre-parsed documents. The benchmark parses each initial serialized output with nlohmann JSON and checks semantic equality before timing writes.

All libraries receive the same bytes. The benchmark runs libraries in a fixed order; repeat it and compare medians before drawing conclusions. The generator's keys and strings avoid escapes because this LLC revision's `jsonWrite()` copies string views without JSON escaping. Arbitrary input containing escaped strings may fail the serialization check.

Blitter's `blit_blitter` branch keeps parsed `SJSONFile` blocks in memory and serves record ranges from their cached views. Its append path updates tokens, views, and nodes incrementally, with the full tree rebuild disabled. The range workload follows its contiguous-view response method, but is not a Blitter application run. Its competitors use each library's DOM and writer; implementations that retain source spans could also avoid serialization. Compare the cold parse, cached response, and write rows as distinct operations rather than treating one row as overall library superiority.
