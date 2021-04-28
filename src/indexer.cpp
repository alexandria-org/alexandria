
// main.cpp
#include <iterator>
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
#include <memory>
#include "zlib.h"
#include <unistd.h>

#include "BasicUrlData.h"
#include "BasicLinkData.h"

using namespace std;
using namespace aws::lambda_runtime;

namespace io = boost::iostreams;

char const TAG[] = "LAMBDA_ALLOC";

#define RUN_ON_LAMBDA 0

class CCIndexer {

public:

	CCIndexer(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key);
	~CCIndexer();

	string run();

private:

	string m_bucket;
	string m_key;
	string m_link_key;
	Aws::S3::S3Client m_s3_client;

	filtering_ostream m_zstream; /* decompression stream */

	BasicUrlData m_url_data;
	BasicLinkData m_link_data;

	void download_file(const string &key, BasicData &index);

};

CCIndexer::CCIndexer(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key) {
	m_bucket = bucket;

	m_key = key;
	m_key.replace(key.find(".warc.gz"), 8, ".gz");

	m_link_key = key;
	m_link_key.replace(key.find(".warc.gz"), 8, ".links.gz");
	m_s3_client = s3_client;

}

CCIndexer::~CCIndexer() {
}

string CCIndexer::run() {

	download_file(m_key, m_url_data);
	download_file(m_key, m_link_data);

	m_url_data.build_index();
	m_link_data.build_index();


	return "Indexing complete";
}

void CCIndexer::download_file(const string &key, BasicData &data) {

	Aws::S3::Model::GetObjectRequest request;
	request.SetBucket(m_bucket);
	request.SetKey(key);

	auto outcome = m_s3_client.GetObject(request);

	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();
		data.read_stream(stream);

	}

}

static invocation_response handle_lambda_invoke(invocation_request const& req, Aws::S3::S3Client const& client) {

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

	CCIndexer indexer(client, bucket, key);
	string response = indexer.run();

	return invocation_response::success("We did it! Response: " + response, "application/json");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> get_logger_factory() {
	return [] {
		return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
			"console_logger", Aws::Utils::Logging::LogLevel::Warn);
	};
}

Aws::Client::ClientConfiguration get_s3_config() {

	Aws::Client::ClientConfiguration config;
	config.region = "us-east-1";
	config.scheme = Aws::Http::Scheme::HTTP;
	config.verifySSL = false;

	return config;
}

void run_lambda_handler() {

	Aws::S3::S3Client client(get_s3_config());
	auto handler_fn = [&client](aws::lambda_runtime::invocation_request const& req) {
		return handle_lambda_invoke(req, client);
	};
	run_handler(handler_fn);
}

int main(int argc, const char **argv) {

	Aws::SDKOptions options;
	options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
	options.loggingOptions.logger_create_fn = get_logger_factory();
	Aws::InitAPI(options);

	if (RUN_ON_LAMBDA) {
		run_lambda_handler();
	} else {
		CCIndexer indexer(Aws::S3::S3Client(get_s3_config()), "alexandria-cc-output",
			"crawl-data/CC-MAIN-2021-10/segments/1614178351134.11/warc/CC-MAIN-20210225124124-20210225154124-00003.warc.gz");
		string response = indexer.run();
		cout << response << endl;
	}

	Aws::ShutdownAPI(options);

	return 0;
}

