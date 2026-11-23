#include "llc_runtime.h"
#include "llc_file.h"
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

sttc ::llc::err_t benchmark_json_entry_point(::llc::SRuntimeValues & runtimeValues) {
	::llc::string input;
	::llc::vcst_t inputPath;
	if(0 <= ::llc::argsOptionIndex(runtimeValues.EntryPointArgs, "input")) {
		if_fail_fe(::llc::argsOptionValue(runtimeValues.EntryPointArgs, "input", inputPath));
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
	rtrn benchmarkParse("rapidjson_write", iterations, rapidBuffer.GetSize(), [&]() -> ::llc::err_t {
		rapidjson::StringBuffer output;
		rapidjson::Writer<rapidjson::StringBuffer> writer(output);
		rtrn rapidDocument.Accept(writer) && output.GetSize() ? 0 : -1;
	});
}
