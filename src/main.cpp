
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
#include <aws/lambda-runtime/runtime.h>
#include <iostream>
#include <memory>
#include "zlib.h"

#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace boost::iostreams;
using namespace aws::lambda_runtime;

namespace io = boost::iostreams;

char const TAG[] = "LAMBDA_ALLOC";

// 5mb chunk
#define CC_PARSER_CHUNK 1024*1024*5
#define CC_PARSER_ZLIB_IN 1024*16
#define CC_PARSER_ZLIB_OUT 1024*16

class CCParser {

public:

	CCParser(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key);
	~CCParser();

	string run();

private:

	string m_bucket;
	string m_key;
	Aws::S3::S3Client m_s3_client;
	int m_cur_offset = 0;
	bool m_continue_inflate = false;
	ofstream m_fout;

	z_stream m_zstream; /* decompression stream */
	char m_z_buffer_in[CC_PARSER_ZLIB_IN];
	char m_z_buffer_out[CC_PARSER_ZLIB_OUT];

	string next_range();
	void handle_record_chunk(char *data, int len);
	int unzip_record(char *data, int size);
	void unzip_chunk(int bytes_in);
	void handle_chunk(const char *buffer, int len);

};

CCParser::CCParser(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key) {
	m_bucket = bucket;
	m_key = key;
	m_s3_client = s3_client;

	m_fout.open("output");
}

CCParser::~CCParser() {
	m_fout.close();
}

string CCParser::next_range() {
	string ret = "bytes=" + to_string(m_cur_offset) + "-" + to_string(m_cur_offset + CC_PARSER_CHUNK - 1);
	return ret;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
#define CHUNK 1024
int inf(FILE *source, FILE *dest) {
	int consumed = 0, consumed_total = 0;
	int avail_in_start;
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];


	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2(&strm, 16);
	if (ret != Z_OK)
		return 0;
	
	/* decompress until deflate stream ends or end of file */
	do {

		avail_in_start = fread(in, 1, CHUNK, source);
		strm.avail_in = avail_in_start;
		if (ferror(source)) {
			(void)inflateEnd(&strm);
			return -1;
		}
		if (strm.avail_in == 0)
			break;
		strm.next_in = in;

		/* run inflate() on input until output buffer not full */
		do {

			strm.avail_out = CHUNK;
			strm.next_out = out;



			ret = inflate(&strm, Z_NO_FLUSH);
			consumed = (avail_in_start - strm.avail_in);
			avail_in_start -= consumed;
			consumed_total += consumed;
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;	 /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				return -1;
			}

			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
				(void)inflateEnd(&strm);
				return -1;
			}

		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	//cout << "ret: " << ret << endl;

	/* clean up and return */
	(void)inflateEnd(&strm);
	return consumed_total;
}

string CCParser::run() {

	Aws::S3::Model::GetObjectRequest request;
	request.SetBucket(m_bucket);
	request.SetKey(m_key);

	// Main loop
	auto total_start = std::chrono::high_resolution_clock::now();
	while (true) {
		auto start = std::chrono::high_resolution_clock::now();
		//cout << "fetching range: " << next_range() << endl;
		request.SetRange(next_range());

		m_cur_offset += CC_PARSER_CHUNK;

		auto outcome = m_s3_client.GetObject(request);

		if (outcome.IsSuccess()) {
			auto &stream = outcome.GetResultWithOwnership().GetBody();

			stream.read(m_z_buffer_in, CC_PARSER_ZLIB_IN);

			if (stream.gcount() == 0) {
				cout << "Stopped because gcount was 0" << endl;
				break;
			}

			unzip_chunk(stream.gcount());

		} else {
			//AWS_LOGSTREAM_ERROR(TAG, "Failed with error: " << outcome.GetError());
			cout << outcome.GetError().GetMessage() << endl;
			break;
		}
		auto elapsed = std::chrono::high_resolution_clock::now() - start;
		auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
		cout << "offset: " << m_cur_offset << " took: " << microseconds / 1000 << " milliseconds" << endl;
		/*if (m_cur_offset > 1024*1024*100) {
			break;
		}*/
	}

	auto total_elapsed = std::chrono::high_resolution_clock::now() - total_start;
	auto total_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(total_elapsed).count();

	return string("We read ") + to_string(m_cur_offset) + " bytes in total of " + to_string(total_microseconds / 1000) + "ms";
}

void CCParser::handle_record_chunk(char *data, int len) {
	m_fout << string(data, len);
	//cout << string(data, len);
}

int CCParser::unzip_record(char *data, int size) {

	int data_offset = 0;
	int data_size = size;
	int consumed = 0, consumed_total = 0;
	int avail_in_start;
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char out[CC_PARSER_ZLIB_OUT];

	if (!m_continue_inflate) {
		m_zstream.zalloc = Z_NULL;
		m_zstream.zfree = Z_NULL;
		m_zstream.opaque = Z_NULL;

		m_zstream.avail_in = 0;
		m_zstream.next_in = Z_NULL;

		int err = inflateInit2(&m_zstream, 16);
		if (err != Z_OK) {
			cout << "zlib error" << endl;
		}
	} else {
		// just continue on the last one.
	}

	/* decompress until deflate stream ends or end of file */
	do {

		m_zstream.next_in = (unsigned char *)(data + data_offset);
		avail_in_start = min(CHUNK, data_size);
		m_zstream.avail_in = avail_in_start;

		if (m_zstream.avail_in == 0)
			break;

		/* run inflate() on input until output buffer not full */
		do {

			m_zstream.avail_out = CHUNK;
			m_zstream.next_out = out;

			ret = inflate(&m_zstream, Z_NO_FLUSH);
			consumed = (avail_in_start - m_zstream.avail_in);
			data_size -= consumed;
			data_offset += consumed;
			avail_in_start -= consumed;
			consumed_total += consumed;
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;	 /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				cout << "Z_MEM_ERROR" << endl;
				(void)inflateEnd(&m_zstream);
				return -1;
			}

			have = CHUNK - m_zstream.avail_out;
			handle_record_chunk((char *)out, have);

		} while (m_zstream.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	//cout << "ret: " << ret << endl;
	if (ret == 0) {
		m_continue_inflate = true;
	} else {
		m_continue_inflate = false;
		(void)inflateEnd(&m_zstream);
	}

	/* clean up and return */
	return consumed_total;
}

void CCParser::unzip_chunk(int bytes_in) {

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
			break;
		}
		ptr += consumed;
		len -= consumed;
		consumed_total += consumed;
	}
}

void CCParser::handle_chunk(const char *buffer, int len) {
	cout << "len: " << len << endl;
	cout << string(buffer, len) << endl;
	//printf("%.*s", len, buffer);
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

	CCParser parser(client, bucket, key);
	string response = parser.run();

	return invocation_response::success("We did it! Response: " + response, "application/base64");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory() {
	return [] {
		return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
			"console_logger", Aws::Utils::Logging::LogLevel::Info);
	};
}

void run_lambda_handler() {

	Aws::Client::ClientConfiguration config;
	config.region = Aws::Environment::GetEnv("AWS_REGION");
	config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";

	auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
	Aws::S3::S3Client client(credentialsProvider, config);
	auto handler_fn = [&client](aws::lambda_runtime::invocation_request const& req) {
		return my_handler(req, client);
	};
	run_handler(handler_fn);
}

int main(int argc, const char **argv) {
	/*FILE *fp_in = fopen("asd.gz", "rb");
	FILE *fp_out = fopen("out.warc", "wb");
	int consumed = 0;
	int consumed_total = 0;
	while (true) {
		consumed = inf(fp_in, fp_out);
		cout << "consumed: " << consumed << endl;
		break;
		if (consumed == 0) {
			cout << "Nothing consumed, done..." << endl;
			break;
		}
		if (consumed < 0) {
			cout << "Encountered fatal error" << endl;
			break;
		}
		consumed_total += consumed;
		fseek(fp_in, consumed_total, SEEK_SET);
	}
	fclose(fp_in);
	fclose(fp_out);
	return 0;*/
	Aws::SDKOptions options;
	options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
	options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
	Aws::InitAPI(options);

	//run_lambda_handler();

	Aws::Client::ClientConfiguration clientConfig;
	clientConfig.region = "us-east-1";
	CCParser parser(Aws::S3::S3Client(clientConfig), "commoncrawl", "crawl-data/CC-MAIN-2021-10/segments/1614178361510.12/warc/CC-MAIN-20210228145113-20210228175113-00135.warc.gz");
	cout << parser.run();

	Aws::ShutdownAPI(options);

	return 0;
}
