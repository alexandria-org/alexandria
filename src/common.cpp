
#include "common.h"

Aws::Client::ClientConfiguration get_s3_config() {

	Aws::Client::ClientConfiguration config;
	config.region = "us-east-1";
	config.scheme = Aws::Http::Scheme::HTTP;
	config.verifySSL = false;

	return config;
}

Aws::S3::S3Client get_s3_client() {
	setenv("AWS_EC2_METADATA_DISABLED", "true", 1);
	auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>("asd");
	return Aws::S3::S3Client(credentialsProvider, get_s3_config());
}
