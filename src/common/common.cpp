
#include "common.h"

#include <sstream>
#include <iomanip>

string escape_json(const std::string &s) {
	ostringstream o;
	for (auto c = s.cbegin(); c != s.cend(); c++) {
		if (*c == '"' || *c == '\\' || ('\x00' <= *c && *c <= '\x1f')) {
			o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
		} else {
			o << *c;
		}
	}
	return o.str();
}

void init_aws_api() {
	Aws::SDKOptions options;
	options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Error;
	options.loggingOptions.logger_create_fn = get_logger_factory();

	Aws::InitAPI(options);
}

void deinit_aws_api() {
	Aws::SDKOptions options;
	Aws::ShutdownAPI(options);
}

Aws::Client::ClientConfiguration get_s3_config() {

	Aws::Client::ClientConfiguration config;
	config.region = "us-east-1";
	config.scheme = Aws::Http::Scheme::HTTP;
	config.verifySSL = false;
	config.requestTimeoutMs = 300000; // 5 minutes
	config.connectTimeoutMs = 300000; // 5 minutes

	return config;
}

Aws::S3::S3Client get_s3_client() {
	setenv("AWS_EC2_METADATA_DISABLED", "true", 1);
	auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>("asd");
	return Aws::S3::S3Client(credentialsProvider, get_s3_config());
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> get_logger_factory() {
	return [] {
		return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
			"console_logger", Aws::Utils::Logging::LogLevel::Error);
	};
}