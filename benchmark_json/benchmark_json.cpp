#include "llc_runtime.h"
#include "llc_file.h"
#include "llc_file_pack.h"
#include "llc_crc.h"
#include "llc_json.h"
#include "llc_parse.h"
#include "llc_timer.h"
#include <json/json.h>
#include <nlohmann/json.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <cstdio>
#include <memory>
#include <string>

sttc ::llc::err_t benchmark_json_entry_point(::llc::SRuntimeValues & runtimeValues);
LLC_OS_ENTRY_POINT(::benchmark_json_entry_point);

template<typename TOperation>
sttc ::llc::err_t benchmarkParse(cnst char * name, ::llc::u2_t iterations, size_t bytes, TOperation operation) {
	::llc::STimer timer;
	for(::llc::u2_t i = 0; i < iterations; ++i)
		if_fail_fe(operation());
	cnst double seconds = timer.Frame();
	if_true_fe(seconds <= 0);
	::std::printf("%s,%zu,%u,%.6f,%.2f\n", name, bytes, iterations, seconds, double(bytes) * iterations / seconds / 1048576.0);
	rtrn 0;
}

template<typename TOperation, typename TOutput>
sttc ::llc::err_t benchmarkRequest(cnst char * name, ::llc::u2_t iterations, TOperation operation, TOutput output) {
	::llc::STimer timer;
	::llc::u3_t checksum = 0;
	for(::llc::u2_t i = 0; i < iterations; ++i) {
		if_fail_fe(operation());
		::llc::u3_t crc = 0;
		if_fail_fe(::llc::crcGenerate(output(), crc));
		checksum += crc;
	}
	cnst double seconds = timer.Frame();
	if_true_fe(seconds <= 0);
	::std::printf("%s,%u,%.6f,%.2f,%llu\n", name, iterations, seconds, iterations / seconds, checksum);
	rtrn 0;
}

sttc ::llc::err_t benchmark_json_entry_point(::llc::SRuntimeValues & runtimeValues) {
	::llc::string input;
	::llc::vcst_t inputPath;
	if(0 <= ::llc::argsOptionIndex(runtimeValues.EntryPointArgs, "input")) {
		if_fail_fe(::llc::argsOptionValue(runtimeValues.EntryPointArgs, "input", inputPath));
		if(0 <= ::llc::argsOptionIndex(runtimeValues.EntryPointArgs, "secure-input")) {
			::llc::SLoadCache cache;
			if_fail_fe(::llc::fileToMemorySecure(cache, inputPath, {}, false, input));
		}
		else
			if_fail_fe(::llc::fileToMemory(inputPath, input));
	}
	else
		if_fail_fe(input.append_string("{\"id\":1,\"name\":\"example\",\"active\":true}"));
	if_true_fe(0 == input.size());
	::llc::u2_t iterations = 1000;
	::llc::vcst_t iterationText;
	if(0 <= ::llc::argsOptionIndex(runtimeValues.EntryPointArgs, "iterations")) {
		if_fail_fe(::llc::argsOptionValue(runtimeValues.EntryPointArgs, "iterations", iterationText));
		::llc::u3_t parsedIterations = 0;
		if_true_fe(::llc::parseIntegerDecimal(iterationText, parsedIterations) != (::llc::err_t)iterationText.size() || !parsedIterations || parsedIterations > 0xFFFFFFFFULL);
		iterations = (::llc::u2_t)parsedIterations;
	}
	::llc::u2_t requests = iterations;
	if(0 <= ::llc::argsOptionIndex(runtimeValues.EntryPointArgs, "requests")) {
		if_fail_fe(::llc::argsOptionValue(runtimeValues.EntryPointArgs, "requests", iterationText));
		::llc::u3_t parsedRequests = 0;
		if_true_fe(::llc::parseIntegerDecimal(iterationText, parsedRequests) != (::llc::err_t)iterationText.size() || !parsedRequests || parsedRequests > 0xFFFFFFFFULL);
		requests = (::llc::u2_t)parsedRequests;
	}
	cnst ::llc::vcst_t text = {input.begin(), input.size()};
	Json::CharReaderBuilder jsoncppBuilder;
	::std::printf("library,bytes,iterations,seconds,mib_per_second\n");
	if_fail_fe(benchmarkParse("llc_tree", iterations, text.size(), [&]() -> ::llc::err_t {
		::llc::SJSONReader document;
		if_fail_fe(::llc::jsonParse(document, text, ::llc::JSON_PARSE_STRICT));
		rtrn document.Tree.size() ? 0 : -1;
	}));
	if_fail_fe(benchmarkParse("jsoncpp", iterations, text.size(), [&]() -> ::llc::err_t {
		::std::unique_ptr<Json::CharReader> parser{jsoncppBuilder.newCharReader()};
		Json::Value document;
		Json::String errors;
		rtrn parser->parse(text.begin(), text.end(), &document, &errors) ? 0 : -1;
	}));
	if_fail_fe(benchmarkParse("nlohmann", iterations, text.size(), [&]() -> ::llc::err_t {
		cnst auto document = nlohmann::json::parse(text.begin(), text.end(), nullptr, false);
		rtrn document.is_discarded() ? -1 : 0;
	}));
	if_fail_fe(benchmarkParse("rapidjson", iterations, text.size(), [&]() -> ::llc::err_t {
		rapidjson::Document document;
		document.Parse(text.begin(), text.size());
		rtrn document.HasParseError() ? -1 : 0;
	}));
	::llc::SJSONParseOptions tokenOptions = ::llc::JSON_PARSE_STRICT;
	tokenOptions.Set(::llc::JSON_PARSE_OPTION_BUILD_TREE, false);
	tokenOptions.Set(::llc::JSON_PARSE_OPTION_BUILD_VIEWS, false);
	if_fail_fe(benchmarkParse("llc_tokens", iterations, text.size(), [&]() -> ::llc::err_t {
		::llc::SJSONReader document;
		if_fail_fe(::llc::jsonParse(document, text, tokenOptions));
		rtrn document.Token.size() ? 0 : -1;
	}));
	::llc::SJSONReader llcDocument;
	if_fail_fe(::llc::jsonParse(llcDocument, text, ::llc::JSON_PARSE_STRICT));
	::std::unique_ptr<Json::CharReader> jsoncppParser{jsoncppBuilder.newCharReader()};
	Json::Value jsoncppDocument;
	Json::String jsoncppErrors;
	if_true_fe(!jsoncppParser->parse(text.begin(), text.end(), &jsoncppDocument, &jsoncppErrors));
	cnst auto nlohmannDocument = nlohmann::json::parse(text.begin(), text.end(), nullptr, false);
	if_true_fe(nlohmannDocument.is_discarded());
	rapidjson::Document rapidDocument;
	rapidDocument.Parse(text.begin(), text.size());
	if_true_fe(rapidDocument.HasParseError());
	::llc::string llcOutput;
	if_fail_fe(::llc::jsonWrite(llcDocument.Tree[0], llcDocument.View, llcOutput));
	Json::StreamWriterBuilder jsoncppWriter;
	jsoncppWriter["indentation"] = "";
	cnst ::std::string jsoncppOutput = Json::writeString(jsoncppWriter, jsoncppDocument);
	cnst ::std::string nlohmannOutput = nlohmannDocument.dump();
	rapidjson::StringBuffer rapidBuffer;
	rapidjson::Writer<rapidjson::StringBuffer> rapidWriter(rapidBuffer);
	if_true_fe(!rapidDocument.Accept(rapidWriter));
	cnst auto equivalent = [&](cnst char * data, size_t size) {
		cnst auto parsed = nlohmann::json::parse(data, data + size, nullptr, false);
		rtrn !parsed.is_discarded() && parsed == nlohmannDocument;
	};
	if_true_fe(!equivalent(llcOutput.begin(), llcOutput.size()) || !equivalent(jsoncppOutput.data(), jsoncppOutput.size()) || !equivalent(rapidBuffer.GetString(), rapidBuffer.GetSize()));
	::llc::string block;
	if(rapidDocument.IsArray())
		if_fail_fe(block.append(text));
	if(!rapidDocument.IsArray()) {
		if_fail_fe(block.push_back('['));
		for(::llc::u2_t i = 0; i < 8; ++i) {
			if(i && ::llc::failed(block.push_back(','))) rtrn -1;
			if_fail_fe(block.append(text));
		}
		if_fail_fe(block.push_back(']'));
	}
	cnst ::llc::vcst_t blockText = {block.begin(), block.size()};
	::llc::SJSONReader llcBlock;
	if_fail_fe(::llc::jsonParse(llcBlock, blockText, ::llc::JSON_PARSE_STRICT));
	Json::Value jsoncppBlock;
	Json::String blockErrors;
	if_true_fe(!jsoncppParser->parse(blockText.begin(), blockText.end(), &jsoncppBlock, &blockErrors));
	cnst auto nlohmannBlock = nlohmann::json::parse(blockText.begin(), blockText.end(), nullptr, false);
	if_true_fe(nlohmannBlock.is_discarded());
	rapidjson::Document rapidBlock;
	rapidBlock.Parse(blockText.begin(), blockText.size());
	if_true_fe(rapidBlock.HasParseError());
	::llc::u2_t first = 2, count = 4;
	if(0 <= ::llc::argsOptionIndex(runtimeValues.EntryPointArgs, "range-start")) {
		if_fail_fe(::llc::argsOptionValue(runtimeValues.EntryPointArgs, "range-start", iterationText));
		if_true_fe(::llc::parseIntegerDecimal(iterationText, first) != (::llc::err_t)iterationText.size());
	}
	if(0 <= ::llc::argsOptionIndex(runtimeValues.EntryPointArgs, "range-count")) {
		if_fail_fe(::llc::argsOptionValue(runtimeValues.EntryPointArgs, "range-count", iterationText));
		if_true_fe(::llc::parseIntegerDecimal(iterationText, count) != (::llc::err_t)iterationText.size());
	}
	if_true_fe(!count || first >= rapidBlock.Size() || count > rapidBlock.Size() - first);
	cnst ::llc::u2_t last = first + count - 1;
	::llc::string llcResponse;
	::std::string jsoncppResponse, nlohmannResponse;
	rapidjson::StringBuffer rapidResponse;
	cnst auto llcRange = [&]() -> ::llc::err_t {
		llcResponse.clear();
		cnst ::llc::err_t beginIndex = ::llc::jsonArrayValueGet(llcBlock, 0, first);
		cnst ::llc::err_t endIndex = ::llc::jsonArrayValueGet(llcBlock, 0, last);
		if_true_fe(beginIndex < 0 || endIndex < 0);
		cnst ::llc::vcst_t beginView = llcBlock.View[beginIndex], endView = llcBlock.View[endIndex];
		if_fail_fe(llcResponse.push_back('['));
		if_fail_fe(llcResponse.append(beginView.begin(), (::llc::u2_t)(endView.end() - beginView.begin())));
		if_fail_fe(llcResponse.push_back(']'));
		rtrn 0;
	};
	cnst auto jsoncppRange = [&]() -> ::llc::err_t {
		Json::Value selected(Json::arrayValue);
		for(::llc::u2_t i = first; i <= last; ++i) selected.append(jsoncppBlock[i]);
		jsoncppResponse = Json::writeString(jsoncppWriter, selected);
		rtrn jsoncppResponse.empty() ? -1 : 0;
	};
	cnst auto nlohmannRange = [&]() -> ::llc::err_t {
		nlohmann::json selected = nlohmann::json::array();
		for(::llc::u2_t i = first; i <= last; ++i) selected.push_back(nlohmannBlock[i]);
		nlohmannResponse = selected.dump();
		rtrn nlohmannResponse.empty() ? -1 : 0;
	};
	cnst auto rapidRange = [&]() -> ::llc::err_t {
		rapidResponse.Clear();
		rapidjson::Writer<rapidjson::StringBuffer> writer(rapidResponse);
		if_true_fe(!writer.StartArray());
		for(::llc::u2_t i = first; i <= last; ++i) if_true_fe(!rapidBlock[i].Accept(writer));
		rtrn writer.EndArray() ? 0 : -1;
	};
	if_fail_fe(llcRange());
	if_fail_fe(jsoncppRange());
	if_fail_fe(nlohmannRange());
	if_fail_fe(rapidRange());
	cnst auto expected = nlohmann::json::parse(nlohmannResponse.begin(), nlohmannResponse.end(), nullptr, false);
	if_true_fe(expected.is_discarded());
	if(0 <= ::llc::argsOptionIndex(runtimeValues.EntryPointArgs, "expected")) {
		::llc::vcst_t expectedPath;
		::llc::string expectedText;
		if_fail_fe(::llc::argsOptionValue(runtimeValues.EntryPointArgs, "expected", expectedPath));
		if_fail_fe(::llc::fileToMemory(expectedPath, expectedText));
		if_true_fe(nlohmann::json::parse(expectedText.begin(), expectedText.end(), nullptr, false) != expected);
	}
	cnst auto matches = [&](cnst char * data, size_t size) { rtrn nlohmann::json::parse(data, data + size, nullptr, false) == expected; };
	if_true_fe(!matches(llcResponse.begin(), llcResponse.size()) || !matches(jsoncppResponse.data(), jsoncppResponse.size()) || !matches(rapidResponse.GetString(), rapidResponse.GetSize()));
	::std::printf("range_block_bytes,%u,range_response_bytes,%u\n", block.size(), llcResponse.size());
	::std::printf("request,iterations,seconds,requests_per_second,checksum_sum\n");
	if_fail_fe(benchmarkRequest("llc_range_response", requests, llcRange, [&]() { rtrn llcResponse.cu8(); }));
	if_fail_fe(benchmarkRequest("jsoncpp_range_response", requests, jsoncppRange, [&]() { rtrn ::llc::vcu0_t{(cnst ::llc::u0_t*)jsoncppResponse.data(), (::llc::u2_t)jsoncppResponse.size()}; }));
	if_fail_fe(benchmarkRequest("nlohmann_range_response", requests, nlohmannRange, [&]() { rtrn ::llc::vcu0_t{(cnst ::llc::u0_t*)nlohmannResponse.data(), (::llc::u2_t)nlohmannResponse.size()}; }));
	if_fail_fe(benchmarkRequest("rapidjson_range_response", requests, rapidRange, [&]() { rtrn ::llc::vcu0_t{(cnst ::llc::u0_t*)rapidResponse.GetString(), (::llc::u2_t)rapidResponse.GetSize()}; }));
	::std::printf("library,bytes,iterations,seconds,mib_per_second\n");
	if_fail_fe(benchmarkParse("llc_write", iterations, llcOutput.size(), [&]() -> ::llc::err_t {
		::llc::string output;
		if_fail_fe(::llc::jsonWrite(llcDocument.Tree[0], llcDocument.View, output));
		rtrn output.size() ? 0 : -1;
	}));
	if_fail_fe(benchmarkParse("jsoncpp_write", iterations, jsoncppOutput.size(), [&]() -> ::llc::err_t {
		cnst ::std::string output = Json::writeString(jsoncppWriter, jsoncppDocument);
		rtrn output.size() ? 0 : -1;
	}));
	if_fail_fe(benchmarkParse("nlohmann_write", iterations, nlohmannOutput.size(), [&]() -> ::llc::err_t {
		cnst ::std::string output = nlohmannDocument.dump();
		rtrn output.size() ? 0 : -1;
	}));
	if_fail_fe(benchmarkParse("rapidjson_write", iterations, rapidBuffer.GetSize(), [&]() -> ::llc::err_t {
		rapidjson::StringBuffer output;
		rapidjson::Writer<rapidjson::StringBuffer> writer(output);
		rtrn rapidDocument.Accept(writer) && output.GetSize() ? 0 : -1;
	}));
	rtrn 0;
}
