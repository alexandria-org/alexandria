
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

#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace boost::iostreams;
using namespace aws::lambda_runtime;

namespace io = boost::iostreams;

char const TAG[] = "LAMBDA_ALLOC";

#define RUN_ON_LAMBDA 0
#define CC_PARSER_CHUNK 1024*1024*50
//#define CC_PARSER_CHUNK 0
#define CC_PARSER_ZLIB_IN 1024*1024*16
#define CC_PARSER_ZLIB_OUT 1024*1024*16

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

	char *m_z_buffer_in;
	char *m_z_buffer_out;

	z_stream m_zstream; /* decompression stream */

	string download_file(const string &key);
	int unzip_record(char *data, int size);
	int unzip_chunk(int bytes_in);

	void handle_record_chunk(char *data, int len);
	void parse_record(const string &record, const string &url);
	string get_warc_record(const string &record, const string &key, int &offset);

};

CCIndexer::CCIndexer(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key) {
	m_bucket = bucket;

	m_key = key;
	m_key.replace(key.find(".warc.gz"), 8, ".gz");

	m_link_key = key;
	m_link_key.replace(key.find(".warc.gz"), 8, ".links.gz");
	m_s3_client = s3_client;

	m_z_buffer_in = new char[CC_PARSER_ZLIB_IN];
	m_z_buffer_out = new char[CC_PARSER_ZLIB_OUT];
}

CCIndexer::~CCIndexer() {
	delete m_z_buffer_in;
	delete m_z_buffer_out;
}

bool CCIndexer::run(string &response) {
	auto total_start = std::chrono::high_resolution_clock::now();

	string file1 = download_file(m_key);
	string file2 = download_file(m_link_key);

	cout << file1.substr(0, 10000) << endl;
	//cout << file2.substr(0, 100) << endl;

	return true;
}

string CCIndexer::download_file(const string &key) {

	Aws::S3::Model::GetObjectRequest request;
	request.SetBucket(m_bucket);
	request.SetKey(key);

	auto outcome = m_s3_client.GetObject(request);

	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();
		int total_bytes_read = 0;
		while (stream.good()) {
			stream.read(m_z_buffer_in, CC_PARSER_ZLIB_IN);

			auto bytes_read = stream.gcount();
			total_bytes_read += bytes_read;

			if (bytes_read > 0) {
				if (unzip_chunk(bytes_read) < 0) {
					return "";
				}
			}
		}
	}

	return string(m_z_buffer_out);
}

int CCIndexer::unzip_record(char *data, int size) {

	/*
	    data is:
	    #|------------------|-----|------------------------|--|----#-------|
	     |doc_a______________doc_b_doc_c_____|
	                         CC_PARSER_ZLIB_IN
	     |_________________________________________________________|
	                                                           size
	*/

	int data_size = size;
	int consumed = 0, consumed_total = 0;
	int avail_in_before_inflate;
	int ret;
	unsigned have;
	z_stream strm;

	m_zstream.zalloc = Z_NULL;
	m_zstream.zfree = Z_NULL;
	m_zstream.opaque = Z_NULL;

	m_zstream.avail_in = 0;
	m_zstream.next_in = Z_NULL;

	int err = inflateInit2(&m_zstream, 16);
	if (err != Z_OK) {
		cout << "zlib error" << endl;
	}

	/* decompress until deflate stream ends or end of file */
	do {

		m_zstream.next_in = (unsigned char *)(data + consumed_total);

		m_zstream.avail_in = min(CC_PARSER_ZLIB_IN, data_size);

		if (m_zstream.avail_in == 0)
			break;

		/* run inflate() on input until output buffer not full */
		do {

			m_zstream.avail_out = CC_PARSER_ZLIB_OUT;
			m_zstream.next_out = (unsigned char *)m_z_buffer_out;

			avail_in_before_inflate = m_zstream.avail_in;

			ret = inflate(&m_zstream, Z_NO_FLUSH);

			// consumed is the number of bytes read from input in this inflate
			consumed = (avail_in_before_inflate - m_zstream.avail_in);
			data_size -= consumed;
			consumed_total += consumed;
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) {
			case Z_BUF_ERROR:
				//cout << "Z_BUF_ERROR" << endl;
				// Not fatal, just keep going.
				break;
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;	 /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				cout << "Z_MEM_ERROR" << endl;
				(void)inflateEnd(&m_zstream);
				return -1;
			}

			have = CC_PARSER_ZLIB_OUT - m_zstream.avail_out;
			handle_record_chunk((char *)m_z_buffer_out, have);

		} while (m_zstream.avail_out == 0);

		if (data_size <= 0) {
			break;
		}

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	//cout << "ret: " << ret << endl;
	//cout << "Ending with code: " << ret << endl;
	(void)inflateEnd(&m_zstream);

	/* clean up and return */
	return consumed_total;
}

int CCIndexer::unzip_chunk(int bytes_in) {

	int consumed = 0;
	int consumed_total = 0;

	char *ptr = m_z_buffer_in;
	int len = bytes_in;

	while (len > 0) {
		consumed = unzip_record(ptr, len);
		//cout << "consumed: " << consumed << " len: " << len << endl;
		if (consumed == 0) {
			cout << "Nothing consumed, done..." << endl;
			break;
		}
		if (consumed < 0) {
			cout << "Encountered fatal error" << endl;
			return -1;
		}
		ptr += consumed;
		len -= consumed;
		consumed_total += consumed;
	}

	return 0;
}

void CCIndexer::handle_record_chunk(char *data, int len) {
	
}

void CCIndexer::parse_record(const string &record, const string &url) {

	
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

