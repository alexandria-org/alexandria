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
#include "system/ThreadPool.h"
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

void run_downloader(const string &warc_path) {

	cout << "downloading: " << warc_path << endl;

	Warc::Parser pp;
	Warc::multipart_download("http://commoncrawl.s3.amazonaws.com/" + warc_path, [&pp](const string &chunk) {
		cout << "parsing chunk of size: " << chunk.size() << endl;
		stringstream ss(chunk);
		pp.parse_stream(ss);
	});

	cout << "uploading: " << warc_path << endl;
	int error;
	error = Transfer::upload_gz_file("/" + Warc::get_result_path(warc_path), pp.result());
	error = Transfer::upload_gz_file("/" + Warc::get_link_result_path(warc_path), pp.link_result());

	if (error) {
		cout << "error" << endl;
	}
}

int main(int argc, const char **argv) {

	vector<string> warc_paths = {
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00000.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00001.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00002.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00003.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00004.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00005.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00006.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00007.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00008.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00009.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00010.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00011.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00012.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00013.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00014.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00015.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00016.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00017.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00018.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00019.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00020.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00021.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00022.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00023.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00024.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00025.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00026.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00027.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00028.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00029.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00030.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00031.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00032.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00033.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00034.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00035.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00036.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00037.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00038.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00039.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00040.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00041.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00042.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00043.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00044.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00045.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00046.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00047.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00048.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00049.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00050.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00051.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00052.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00053.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00054.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00055.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00056.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00057.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00058.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00059.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00060.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00061.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00062.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00063.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00064.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00065.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00066.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00067.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00068.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00069.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00070.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00071.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00072.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00073.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00074.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00075.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00076.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00077.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00078.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00079.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00080.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00081.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00082.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00083.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00084.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00085.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00086.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00087.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00088.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00089.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00090.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00091.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00092.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00093.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00094.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00095.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00096.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00097.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00098.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00099.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00100.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00101.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00102.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00103.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00104.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00105.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00106.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00107.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00108.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00109.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00110.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00111.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00112.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00113.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00114.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00115.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00116.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00117.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00118.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00119.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00120.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00121.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00122.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00123.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00124.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00125.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00126.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00127.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00128.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00129.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00130.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00131.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00132.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00133.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00134.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00135.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00136.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00137.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00138.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00139.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00140.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00141.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00142.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00143.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00144.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00145.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00146.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00147.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00148.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00149.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00150.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00151.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00152.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00153.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00154.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00155.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00156.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00157.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00158.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00159.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00160.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00161.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00162.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00163.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00164.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00165.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00166.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00167.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00168.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00169.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00170.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00171.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00172.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00173.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00174.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00175.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00176.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00177.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00178.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00179.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00180.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00181.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00182.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00183.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00184.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00185.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00186.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00187.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00188.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00189.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00190.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00191.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00192.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00193.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00194.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00195.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00196.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00197.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00198.warc.gz",
		"crawl-data/CC-MAIN-2021-43/segments/1634323583083.92/warc/CC-MAIN-20211015192439-20211015222439-00199.warc.gz"
	};

	Logger::start_logger_thread();

	if (getenv("ALEXANDRIA_CONFIG") != NULL) {
		Config::read_config(getenv("ALEXANDRIA_CONFIG"));
	} else {
		Config::read_config("/etc/alexandria.conf");
	}

	const size_t num_threads = 48;
	ThreadPool pool(num_threads);
	vector<future<void>> results;

	for (const string &warc_path : warc_paths) {
		results.emplace_back(pool.enqueue([warc_path, num_threads] {
			sleep(rand() % (num_threads * 2));
			run_downloader(warc_path);
		}));
	}


	for(auto &&result: results) {
		result.get();
	}

	Logger::join_logger_thread();


	return 0;
}

