# JSON comparison benchmark

Build `llc_benchmark.slnx` in Release x64. The solution builds `llc.lib` before both applications. `benchmark_json` compiles JsonCpp's three implementation files and includes the pinned nlohmann JSON and RapidJSON headers from the LLB submodules.

Run `benchmark_json.exe --input=corpus.json --iterations=100`. Without `--input`, it uses a small built-in object. CSV output gives document bytes, iteration count, elapsed seconds, and MiB/s. File loading and initial serialization validation are outside the timed loops. Each parse iteration constructs a fresh document. Each write iteration constructs a fresh output buffer from a pre-parsed document.

`llc_tree`, `jsoncpp`, `nlohmann`, and `rapidjson` compare parse plus document construction. `llc_tokens` omits views and tree construction and is a separate workload. The four `_write` rows serialize pre-parsed documents. The benchmark parses each initial serialized output with nlohmann JSON and checks semantic equality before timing writes.

All libraries receive the same bytes. The benchmark runs libraries in a fixed order; repeat it and compare medians before drawing conclusions. The generator's keys and strings avoid escapes because this LLC revision's `jsonWrite()` copies string views without JSON escaping. Arbitrary input containing escaped strings may fail the serialization check.
