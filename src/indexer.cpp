
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
using namespace Aws::Utils::Json;

namespace io = boost::iostreams;

char const TAG[] = "LAMBDA_ALLOC";

#define RUN_ON_LAMBDA 0

class CCIndexer {

public:

	CCIndexer(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key, int id);
	~CCIndexer();

	string run();

private:

	int m_id;
	string m_bucket;
	string m_key;
	string m_link_key;
	Aws::S3::S3Client m_s3_client;

	BasicUrlData m_url_data;
	BasicLinkData m_link_data;

	void download_file(const string &key, BasicData &index);

};

CCIndexer::CCIndexer(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key, int id) {
	m_id = id;
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
	//download_file(m_key, m_link_data);

	m_url_data.build_index(m_id);
	//m_link_data.build_index();


	return "Indexing complete";
}

void CCIndexer::download_file(const string &key, BasicData &data) {

	Aws::S3::Model::GetObjectRequest request;
	cout << "Downloading " << m_bucket << " key: " << key << endl;
	request.SetBucket(m_bucket);
	request.SetKey(key);

	auto outcome = m_s3_client.GetObject(request);

	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();
		data.read_stream(stream);

	}

}

/*
SNS Records looks like this:
{
    "Records": [
        {
            "EventSource": "aws:sns",
            "EventVersion": "1.0",
            "EventSubscriptionArn": "arn:aws:sns:us-east-1:259015057813:lambda-cc-indexer:de574280-62ce-43d9-8a15-4f4dcdd9e34f",
            "Sns": {
                "Type": "Notification",
                "MessageId": "4b12ffab-c986-51e0-8e4f-a6ae79b3b3ce",
                "TopicArn": "arn:aws:sns:us-east-1:259015057813:lambda-cc-indexer",
                "Subject": "Testar",
                "Message": "HelloJose",
                "Timestamp": "2021-04-30T07:12:53.110Z",
                "SignatureVersion": "1",
                "Signature": "UuSvZr0RoRf5wqFX/kf1/6ACZ4zSyAtITZbyIOSJkI/ZpA4hQs/OCSl+V5KmIGRUR9Uyc7YpDDDUbIheTkIo46/KqyFWeknDMdsjz8PWuaXPZ853bfNpS5a8Yhvyw37nVIx3Fdwbi3daN3u2OCmkQzIejfVAY33YMgdnW/wpWfuYR7XHsy61085kwwa7fUzcfbVPCofdBJDAyKaHXFSqVfX3kFkxH+zTO0zcsZzfBm2o6HnM3mx040GoAO+L3aqRxmDMZ148niHq7j1VfKrfxn3HZJ94fd3Bbd9ObYGVHoreG3n4/nvvbhU2jRPFtcpTSicFyB0EIMLk2U9RiXqhUQ==",
                "SigningCertUrl": "https://sns.us-east-1.amazonaws.com/SimpleNotificationService-010a507c1833636cd94bdb98bd93083a.pem",
                "UnsubscribeUrl": "https://sns.us-east-1.amazonaws.com/?Action=Unsubscribe&SubscriptionArn=arn:aws:sns:us-east-1:259015057813:lambda-cc-indexer:de574280-62ce-43d9-8a15-4f4dcdd9e34f",
                "MessageAttributes": {}
            }
        }
    ]
}

*/

invocation_response handle_invoke(Aws::S3::S3Client const& client, Aws::Utils::Json::JsonView &payload) {
	if (!payload.ValueExists("s3bucket") || !payload.ValueExists("s3key") ||
		!payload.GetObject("s3bucket").IsString() ||
		!payload.GetObject("s3key").IsString(), !payload.GetObject("id").IsIntegerType()) {
		return invocation_response::failure("Missing input value s3bucket or s3key", "InvalidJSON");
	}

	auto bucket = payload.GetString("s3bucket");
	auto key = payload.GetString("s3key");
	auto id = payload.GetInteger("id");

	CCIndexer indexer(client, bucket, key, id);
	string response = indexer.run();

	return invocation_response::success("We did it! Response: " + response, "application/json");
}

invocation_response handle_sns_record(Aws::S3::S3Client const& client, Aws::Utils::Json::JsonView &record) {

	if (!record.ValueExists("Sns")) {
		return invocation_response::failure("Malformed SNS record", "InvalidJSON"); 
	}

	auto sns = record.GetObject("Sns");

	if (!sns.ValueExists("Message") || !sns.GetObject("Message").IsString()) {
		return invocation_response::failure("Malformed SNS message", "InvalidJSON"); 
	}

	JsonValue json(sns.GetString("Message"));

	auto v = json.View();
	return handle_invoke(client, v);
}

static invocation_response handle_lambda_invoke(invocation_request const& req, Aws::S3::S3Client const& client) {

	JsonValue json(req.payload);
	if (!json.WasParseSuccessful()) {
		return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
	}

	auto v = json.View();

	if (v.ValueExists("Records") && v.GetObject("Records").IsListType()) {
		return handle_sns_record(client, v.GetArray("Records").GetItem(0));
	}
	return handle_invoke(client, v);
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
			"crawl-data/CC-MAIN-2021-10/segments/1614178351134.11/warc/CC-MAIN-20210225124124-20210225154124-00003.warc.gz",
			123);
		string response = indexer.run();
		cout << response << endl;
	}

	Aws::ShutdownAPI(options);

	return 0;
}

