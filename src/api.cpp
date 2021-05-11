
// main.cpp
#include "HtmlParser.h"
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
#include <unistd.h>

#include "CCApi.h"

#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace boost::iostreams;
using namespace aws::lambda_runtime;
using namespace Aws::Utils::Json;

namespace io = boost::iostreams;

string error_response(const string &error_message) {
	JsonValue response_json;

	response_json.WithObject("statusCode", JsonValue().AsInteger(200));
	response_json.WithObject("headers", JsonValue().WithObject("Access-Control-Allow-Origin", JsonValue().AsString("*")));
	response_json.WithObject("body", JsonValue().AsString(error_message));
	response_json.WithObject("isBase64Encoded", JsonValue().AsBool(false));

	return response_json.View().WriteReadable();
}

static invocation_response my_handler(invocation_request const& req, Aws::S3::S3Client const& client) {
	
	JsonValue json(req.payload);
	if (!json.WasParseSuccessful()) {
		return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
	}

	auto v = json.View();

	if (!v.ValueExists("queryStringParameters")) {
		return invocation_response::success(error_response("Missing query 1"), "application/json");
	}

	if (!v.GetObject("queryStringParameters").ValueExists("query")) {
		return invocation_response::success(error_response("Missing query 2"), "application/json");
	}

	auto query = v.GetObject("queryStringParameters").GetString("query");

	CCApi api(client);
	ApiResponse response = api.query(query);
	string response_body = response.json();

	JsonValue response_json;

	cout << "response_body: " << response_body << endl;

	response_json.WithObject("statusCode", JsonValue().AsInteger(200));
	response_json.WithObject("headers", JsonValue().WithObject("Access-Control-Allow-Origin", JsonValue().AsString("*")));
	response_json.WithObject("body", JsonValue().AsString(response_body));
	response_json.WithObject("isBase64Encoded", JsonValue().AsBool(true));

	cout << "invocation_response: " << response_json.View().WriteReadable() << endl;

	return invocation_response::success(response_json.View().WriteReadable(), "application/json");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory() {
	return [] {
		return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
			"console_logger", Aws::Utils::Logging::LogLevel::Error);
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
	options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Error;
	options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
	Aws::InitAPI(options);

	/*CCApi api(get_s3_client());
	string response_json = api.query("sushi uppsala");
	cout << response_json << endl;*/

	run_lambda_handler();

	Aws::ShutdownAPI(options);

	return 0;
}

