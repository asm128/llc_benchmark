#include "llc_runtime.h"
#include "llc_append_json.h"
#include "llc_cstring.h"
#include "llc_file.h"
#include "llc_json.h"
#include "llc_parse.h"
#include <rapidjson/document.h>
#include <cstdio>

sttc ::llc::err_t llc_json_generator_entry_point(::llc::SRuntimeValues & runtimeValues);
LLC_OS_ENTRY_POINT(::llc_json_generator_entry_point);

sttc ::llc::err_t llc_json_generator_entry_point(::llc::SRuntimeValues & runtimeValues) {
	cnst ::llc::SCommandLineArgs & args = runtimeValues.EntryPointArgs;
	::llc::aobj<::llc::vcst_t> keys, values;
	if_fail_fe(::llc::argsOptionValues(args, "key", keys));
	if_fail_fe(::llc::argsOptionValues(args, "value", values));
	if_true_fe(keys.size() != values.size());
	::llc::u2_t count = 0;
	::llc::vcst_t option;
	if(0 <= ::llc::argsOptionIndex(args, "count")) {
		if_fail_fe(::llc::argsOptionValue(args, "count", option));
		::llc::u3_t parsedCount = 0;
		if_true_fe(::llc::parseIntegerDecimal(option, parsedCount) != (::llc::err_t)option.size() || parsedCount > 0xFFFFFFFFULL);
		count = (::llc::u2_t)parsedCount;
	}
	::llc::vcst_t keyPrefix = "key_", valuePrefix = "value_", valueKind = "string", outputPath = "corpus.json";
	if(0 <= ::llc::argsOptionIndex(args, "key-prefix")) if_fail_fe(::llc::argsOptionValue(args, "key-prefix", keyPrefix));
	if(0 <= ::llc::argsOptionIndex(args, "value-prefix")) if_fail_fe(::llc::argsOptionValue(args, "value-prefix", valuePrefix));
	if(0 <= ::llc::argsOptionIndex(args, "value-kind")) if_fail_fe(::llc::argsOptionValue(args, "value-kind", valueKind));
	if(0 <= ::llc::argsOptionIndex(args, "output")) if_fail_fe(::llc::argsOptionValue(args, "output", outputPath));
	if(0 <= ::llc::argsOptionIndex(args, "records")) {
		if_fail_fe(::llc::argsOptionValue(args, "records", option));
		::llc::u3_t recordCount = 0;
		if_true_fe(::llc::parseIntegerDecimal(option, recordCount) != (::llc::err_t)option.size() || !recordCount || recordCount > 100000);
		::llc::string business;
		if_fail_fe(business.push_back('['));
		for(::llc::u2_t i = 0; i < recordCount; ++i) {
			char record[2048] = {};
			if_fail_fe(::llc::sprintf_s(record, "{\"user\":{\"id\":\"usr_%u\",\"name\":\"Customer %u\",\"email\":\"customer%u@example.invalid\",\"phone\":\"+1-555-%04u\",\"address\":{\"line1\":\"%u Market Street\",\"city\":\"Northport\",\"region\":\"CA\",\"postal\":\"%05u\"}},\"account\":{\"id\":\"acct_%u\",\"tier\":\"business\"},\"card\":{\"token\":\"tok_%u\",\"last4\":\"%04u\",\"brand\":\"visa\"},\"merchant\":{\"id\":\"mrc_%u\",\"name\":\"Merchant %u\",\"url\":\"https://merchant.example.invalid/%u\"},\"transaction\":{\"id\":\"txn_%u\",\"amount_minor\":%u,\"currency\":\"USD\",\"status\":\"settled\",\"created_at\":\"2026-11-23T12:00:00Z\"},\"inbox\":[{\"id\":\"msg_%u\",\"subject\":\"Payment receipt\",\"href\":\"https://merchant.example.invalid/receipts/%u\"}]}", i, i, i, i % 10000, i % 10000, i % 100000, i, i, i % 10000, i % 100, i % 100, i % 100, i, i * 37 + 99, i, i));
			if(i) if_fail_fe(business.push_back(','));
			if_fail_fe(business.append_string(record));
		}
		if_fail_fe(business.push_back(']'));
		rapidjson::Document syntax;
		syntax.Parse(business.begin(), business.size());
		if_true_fe(syntax.HasParseError());
		::llc::SJSONParseOptions businessOptions = ::llc::JSON_PARSE_STRICT;
		businessOptions.Set(::llc::JSON_PARSE_OPTION_BUILD_TREE, false);
		businessOptions.Set(::llc::JSON_PARSE_OPTION_BUILD_VIEWS, false);
		::llc::SJSONReader businessCheck;
		if_fail_fe(::llc::jsonParse(businessCheck, business, businessOptions));
		if_fail_fe(::llc::fileFromMemory(outputPath, ::llc::vcst_t{business.begin(), business.size()}));
		::std::printf("wrote %u records, %u bytes to %.*s\n", (::llc::u2_t)recordCount, business.size(), (int)outputPath.size(), outputPath.begin());
		rtrn 0;
	}
	if_true_fe(valueKind != "string" && valueKind != "integer" && valueKind != "object");
	::llc::SJSONParseOptions validationOptions = ::llc::JSON_PARSE_STRICT;
	validationOptions.Set(::llc::JSON_PARSE_OPTION_BUILD_TREE, false);
	::llc::string json;
	if_fail_fe(json.push_back('{'));
	for(::llc::u2_t i = 0; i < keys.size(); ++i) {
		rapidjson::Document syntax;
		syntax.Parse(values[i].begin(), values[i].size());
		if_true_fe(syntax.HasParseError());
		::llc::SJSONReader valueCheck;
		if_fail_fe(::llc::jsonParse(valueCheck, values[i], validationOptions));
		if_fail_fe(::llc::appendKeyValue(json, keys[i], values[i], json.size() > 1));
	}
	for(::llc::u2_t i = 0; i < count; ++i) {
		char number[32] = {};
		if_fail_fe(::llc::sprintf_s(number, "%u", i));
		::llc::string key, value;
		if_fail_fe(::llc::append_strings(key, keyPrefix, number));
		if(valueKind == "string") {
			if_fail_fe(::llc::append_strings(value, valuePrefix, number));
			if_fail_fe(::llc::appendKeyString(json, key, value, json.size() > 1));
		}
		else if(valueKind == "object") {
			if_fail_fe(::llc::appendKeyValue(value, "id", number));
			::llc::string label;
			if_fail_fe(::llc::append_strings(label, valuePrefix, number));
			if_fail_fe(::llc::appendKeyString(value, "label", label, true));
			if_fail_fe(::llc::appendKeyObject(json, key, value, json.size() > 1));
		}
		else
			if_fail_fe(::llc::appendKeyValue(json, key, number, json.size() > 1));
	}
	if_fail_fe(json.push_back('}'));
	rapidjson::Document syntax;
	syntax.Parse(json.begin(), json.size());
	if_true_fe(syntax.HasParseError());
	::llc::SJSONReader validation;
	if_fail_fe(::llc::jsonParse(validation, json, validationOptions));
	if_fail_fe(::llc::fileFromMemory(outputPath, ::llc::vcst_t{json.begin(), json.size()}));
	::std::printf("wrote %u bytes to %.*s\n", json.size(), (int)outputPath.size(), outputPath.begin());
	rtrn 0;
}
