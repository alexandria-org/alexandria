/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// main.cpp
#include "config.h"
#include "parser/Warc.h"
#include "system/Logger.h"
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/lambda-runtime/runtime.h>
#include <iostream>

#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace boost::iostreams;
using namespace aws::lambda_runtime;

namespace io = boost::iostreams;

char const TAG[] = "LAMBDA_ALLOC";

#define CC_PARSER_CHUNK 0


string next_range(size_t offset) {
	string ret = "bytes=" + to_string(offset) + "-" + to_string(offset + CC_PARSER_CHUNK - 1);
	return ret;
}

void upload_result(Aws::S3::S3Client const& s3_client, const string &result, const string &target_key) {

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket(Config::cc_target_output);
	string key = target_key;
	key.replace(key.find(".warc.gz"), 8, string(".gz"));
	request.SetKey(key);

	stringstream ss;
	ss << result;

	filtering_istream in;
	in.push(gzip_compressor());
	in.push(ss);

	shared_ptr<Aws::StringStream> request_body = Aws::MakeShared<Aws::StringStream>("");
	*request_body << in.rdbuf();
	request.SetBody(request_body);

	Aws::S3::Model::PutObjectOutcome outcome = s3_client.PutObject(request);

	if (outcome.IsSuccess()) {
	    cout << "Added object '" << key << "' to bucket '" << Config::cc_target_output << "'.";
	} else {
		cout << "Error: PutObject: " << outcome.GetError().GetMessage() << endl;
	}
}

void upload_links(Aws::S3::S3Client const& s3_client, const string &result, const string &target_key) {

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket(Config::cc_target_output);
	string key = target_key;
	key.replace(key.find(".warc.gz"), 8, string(".links.gz"));
	request.SetKey(key);

	stringstream ss;
	ss << result;

	filtering_istream in;
	in.push(gzip_compressor());
	in.push(ss);

	shared_ptr<Aws::StringStream> request_body = Aws::MakeShared<Aws::StringStream>("");
	*request_body << in.rdbuf();
	request.SetBody(request_body);

	Aws::S3::Model::PutObjectOutcome outcome = s3_client.PutObject(request);

	if (outcome.IsSuccess()) {
	    cout << "Added object '" << key << "' to bucket '" << Config::cc_target_output << "'.";
	} else {
	    cout << "Error: PutObject: " << outcome.GetError().GetMessage() << endl;

	}
}

void upload_internal_links(Aws::S3::S3Client const& s3_client, const string &result, const string &target_key) {

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket(Config::cc_target_output);
	string key = target_key;
	key.replace(key.find(".warc.gz"), 8, string(".internal.gz"));
	request.SetKey(key);

	stringstream ss;
	ss << result;

	filtering_istream in;
	in.push(gzip_compressor());
	in.push(ss);

	shared_ptr<Aws::StringStream> request_body = Aws::MakeShared<Aws::StringStream>("");
	*request_body << in.rdbuf();
	request.SetBody(request_body);

	Aws::S3::Model::PutObjectOutcome outcome = s3_client.PutObject(request);

	if (outcome.IsSuccess()) {
	    cout << "Added object '" << key << "' to bucket '" << Config::cc_target_output << "'.";
	} else {
	    cout << "Error: PutObject: " << outcome.GetError().GetMessage() << endl;

	}
}

bool parse_warc(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key) {

	Aws::S3::Model::GetObjectRequest request;
	request.SetBucket(bucket);
	request.SetKey(key);

	// Main loop
	auto total_start = std::chrono::high_resolution_clock::now();
	bool fatal_error = false;
	size_t offset = 0;
	Warc::Parser parser;
	while (!fatal_error) {
		auto start = std::chrono::high_resolution_clock::now();

		if (CC_PARSER_CHUNK) {
			cout << "setting offset: " << next_range(offset) << endl;
			request.SetRange(next_range(offset));
		}

		auto outcome = s3_client.GetObject(request);

		if (outcome.IsSuccess()) {

			offset += CC_PARSER_CHUNK;

			auto &stream = outcome.GetResultWithOwnership().GetBody();

			stream.seekg(0, stream.end);
			int length = stream.tellg();
			stream.seekg(0, stream.beg);

			cout << "Got stream with " << length << " bytes" << endl;

			if (stream.bad()) {
				cout << "Badbit was set" << endl;
				return false;
			}
			if (stream.fail()) {
				cout << "Failbit was set" << endl;
				return false;
			}
			if (stream.eof()) {
				cout << "Eofbit was set" << endl;
			}

			parser.parse_stream(stream);

		} else {
			//AWS_LOGSTREAM_ERROR(TAG, "Failed with error: " << outcome.GetError());
			cout << "WE ARE HERE NOW AND GOT AN ERROR, probably the stream has ended" << endl;
			break;
		}
		auto elapsed = std::chrono::high_resolution_clock::now() - start;
		auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
		cout << "offset: " << offset << " took: " << microseconds / 1000 << " milliseconds" << endl;
		if (!CC_PARSER_CHUNK) {
			break;
		}
	}

	auto total_elapsed = std::chrono::high_resolution_clock::now() - total_start;
	auto total_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(total_elapsed).count();

	/*upload_result(s3_client, parser.result(), key);
	upload_links(s3_client, parser.link_result(), key);
	upload_internal_links(s3_client, parser.internal_link_result(), key);*/

	cout << string("We read ") + to_string(offset) + " bytes in total of " + to_string(total_microseconds / 1000) + "ms";

	return true;
}

static invocation_response my_handler(invocation_request const& req, Aws::S3::S3Client const& client) {
	using namespace Aws::Utils::Json;
	JsonValue json(req.payload);
	if (!json.WasParseSuccessful()) {
		return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
	}

	auto v = json.View();

	if (!v.ValueExists("s3bucket") || !v.ValueExists("s3key") || !v.GetObject("s3bucket").IsString() ||
		!v.GetObject("s3key").IsString()) {
		return invocation_response::failure("Missing input value s3bucket or s3key", "InvalidJSON");
	}

	auto bucket = v.GetString("s3bucket");
	auto key = v.GetString("s3key");

	AWS_LOGSTREAM_INFO(TAG, "Attempting to download file from s3://" << bucket << "/" << key);

	// Internal execution about 70 seconds...
	for (int retry = 1; retry <= 1; retry++) {
		if (parse_warc(client, bucket, key)) {
			break;
		}
		cout << "Retry " << retry << endl;
	}

	return invocation_response::success("We did it!", "application/json");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory() {
	return [] {
		return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
			"console_logger", Aws::Utils::Logging::LogLevel::Warn);
	};
}

Aws::Client::ClientConfiguration getS3Config() {

	Aws::Client::ClientConfiguration config;
	config.region = "us-east-1";
	config.scheme = Aws::Http::Scheme::HTTP;

	return config;
}

Aws::Client::ClientConfiguration getS3ConfigWithBundle() {

	cout << "USING S3 REGION: " << Aws::Environment::GetEnv("AWS_REGION") << endl;

	Aws::Client::ClientConfiguration config;
	config.region = Aws::Environment::GetEnv("AWS_REGION");
	config.scheme = Aws::Http::Scheme::HTTP;
	config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";
	config.verifySSL = false;

	return config;
}

void run_lambda_handler() {

	Aws::S3::S3Client client(getS3Config());
	auto handler_fn = [&client](aws::lambda_runtime::invocation_request const& req) {
		return my_handler(req, client);
	};
	run_handler(handler_fn);
}

void usage() {
	cout << "cc_parser [commoncrawl warc path]" << endl;
	exit(0);
}

int main(int argc, const char **argv) {

	if (argc != 2) {
		usage();
	}

	Logger::start_logger_thread();

	const string warc_path(argv[1]);
	const string data = Warc::multipart_download(warc_path);
	stringstream data_stream(data);

	Warc::Parser pp;
	pp.parse_stream(data_stream);

	int error;

	error = Transfer::upload_gz_file("/" + Warc::get_result_path(warc_path), pp.result());
	error = Transfer::upload_gz_file("/" + Warc::get_link_result_path(warc_path), pp.link_result());
	error = Transfer::upload_gz_file("/" + Warc::get_internal_link_result_path(warc_path), pp.internal_link_result());

	if (error) {
		cout << "error" << endl;
	}

	Logger::join_logger_thread();


		/*if (parse_warc(s3_client, "commoncrawl",
			"crawl-data/CC-MAIN-2021-17/segments/1618038056325.1/warc/CC-MAIN-20210416100222-20210416130222-00002.warc.gz")) {
		}*/

	return 0;
}

