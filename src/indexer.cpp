
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

#include "CCIndex.h"

using namespace std;
using namespace aws::lambda_runtime;

namespace io = boost::iostreams;

char const TAG[] = "LAMBDA_ALLOC";

#define RUN_ON_LAMBDA 0

class CCIndexer {

public:

	CCIndexer(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key);
	~CCIndexer();

	bool run(string &response);

private:

	string m_bucket;
	string m_key;
	string m_link_key;
	Aws::S3::S3Client m_s3_client;

	filtering_ostream m_zstream; /* decompression stream */

	CCIndex m_index1;
	CCIndex m_index2;

	void download_file(const string &key, CCIndex &index);

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

bool CCIndexer::run(string &response) {

	download_file(m_key, m_index1);
	download_file(m_link_key, m_index2);

	return true;
}

void CCIndexer::download_file(const string &key, CCIndex &index) {

	Aws::S3::Model::GetObjectRequest request;
	request.SetBucket(m_bucket);
	request.SetKey(key);

	auto outcome = m_s3_client.GetObject(request);

	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();
		index.read_stream(stream);

	}

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
	CCIndexer parser(client, bucket, key);
	string response;

	for (int retry = 1; retry <= 1; retry++) {
		if (parser.run(response)) {
			break;
		}
		cout << "Retry " << retry << endl;
	}

	return invocation_response::success("We did it! Response: " + response, "application/json");
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

int main(int argc, const char **argv) {

	Aws::SDKOptions options;
	options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
	options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
	Aws::InitAPI(options);

	if (RUN_ON_LAMBDA) {
		run_lambda_handler();
	} else {
		CCIndexer parser(Aws::S3::S3Client(getS3Config()), "commoncrawl-output", "crawl-data/CC-MAIN-2021-10/segments/1614178347293.1/warc/CC-MAIN-20210224165708-20210224195708-00008.warc.gz");
		string response;
		for (int retry = 1; retry <= 1; retry++) {
			if (parser.run(response)) {
				break;
			}
			cout << "Retry " << retry << endl;
		}
		cout << response << endl;
	}

	Aws::ShutdownAPI(options);

	return 0;
}

