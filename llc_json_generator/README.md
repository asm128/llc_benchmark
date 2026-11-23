# JSON corpus generator

`llc_json_generator.exe --output=corpus.json --count=1000 --key-prefix=field_ --value-prefix=item_ --value-kind=string`

Repeat `--key` and `--value` together for literal fields. Values are JSON literals, for example `--key=status --value=true`. Generated values can be `string`, `integer`, or `object`; object values contain an integer `id` and string `label`. `--count=0` is valid. The tool checks literal values and the final document with RapidJSON and LLC's tree-free parser before writing it.

Keys and generated string prefixes must contain characters that are safe inside a JSON string. LLC's existing append helpers do not escape quotes or backslashes; invalid output fails validation and is not saved.

Run `benchmark_json.exe --input=corpus.json --iterations=500` to compare strict tree parsing with JsonCpp, nlohmann JSON, and RapidJSON. The separate `llc_tokens` row parses without building views or a tree, so it is a different workload.
