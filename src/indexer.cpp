
#include <iterator>
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#include <iostream>
#include <memory>
#include <unistd.h>

#include "CCIndexer.h"
#include "SubSystem.h"


using namespace std;
using namespace Aws::Utils::Json;

namespace io = boost::iostreams;

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> get_logger_factory() {
	return [] {
		return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
			"console_logger", Aws::Utils::Logging::LogLevel::Error);
	};
}

int main(int argc, const char **argv) {

	Aws::SDKOptions options;
	options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Error;
	options.loggingOptions.logger_create_fn = get_logger_factory();

	Aws::InitAPI(options);

	Aws::S3::S3Client s3_client = get_s3_client();

	SubSystem *sub_system = new SubSystem(s3_client);

	string warc_paths_url = "crawl-data/CC-MAIN-2021-10/warc.paths.gz";
	TsvFileS3 warc_paths_file(s3_client, "commoncrawl", warc_paths_url);

	vector<string> warc_paths;
	warc_paths_file.read_column_into(0, warc_paths);

	int id = 1;
	const int num_shards = 8;
	for (const string &warc_path : warc_paths) {
		string bucket = "alexandria-cc-output";

		CCIndexer indexer(sub_system, bucket, warc_path, id % num_shards, id);
		string response = indexer.run();
		
		cout << response << endl;
	}

	delete sub_system;

	Aws::ShutdownAPI(options);

	return 0;
}

