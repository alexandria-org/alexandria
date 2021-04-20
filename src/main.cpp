
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
#define CC_PARSER_CHUNK 4024
#define CC_PARSER_ZLIB_IN 4024
#define CC_PARSER_ZLIB_OUT 20240

class CCParser {

public:

	CCParser(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key);
	~CCParser();

	void run();

private:

	string m_bucket;
	string m_key;
	Aws::S3::S3Client m_s3_client;
	int m_cur_offset = 0;

	z_stream m_zstream; /* decompression stream */
	char *m_z_buffer_in;
	char *m_z_buffer_out;

	string next_range();
	void unzip_chunk(int bytes_in);
	void handle_chunk(const char *buffer, int len);

};

CCParser::CCParser(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key) {
	m_bucket = bucket;
	m_key = key;
	m_s3_client = s3_client;

	m_z_buffer_in = new char[CC_PARSER_ZLIB_IN];
	m_z_buffer_out = new char[CC_PARSER_ZLIB_OUT];

	m_zstream.zalloc = (alloc_func)0;
	m_zstream.zfree = (free_func)0;
	m_zstream.opaque = (voidpf)0;

	m_zstream.next_in = Z_NULL;
	m_zstream.avail_in = 0;

	m_zstream.next_out = Z_NULL;
	m_zstream.avail_out = 0;

	int err = inflateInit2(&m_zstream, 16+MAX_WBITS);
	if (err != Z_OK) {
		cout << "zlib error" << endl;
	}
}

CCParser::~CCParser() {
	delete m_z_buffer_in;
	delete m_z_buffer_out;
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
int inf(FILE *source, FILE *dest)
{
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
                ret = Z_DATA_ERROR;     /* and fall through */
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

    /* clean up and return */
    (void)inflateEnd(&strm);
    return consumed_total;
}

void CCParser::run() {


	Aws::S3::Model::GetObjectRequest request;
	request.SetBucket(m_bucket);
	request.SetKey(m_key);

	// Main loop
	while (true) {
		auto start = std::chrono::high_resolution_clock::now();
		cout << "fetching range: " << next_range() << endl;
		request.SetRange(next_range());

		m_cur_offset += CC_PARSER_CHUNK;

		auto outcome = m_s3_client.GetObject(request);

		if (outcome.IsSuccess()) {
			auto &stream = outcome.GetResultWithOwnership().GetBody();

			stream.read(m_z_buffer_in, CC_PARSER_ZLIB_IN);

			cout << "gcount: " << stream.gcount() << endl;

			unzip_chunk(stream.gcount());

		} else {
			//AWS_LOGSTREAM_ERROR(TAG, "Failed with error: " << outcome.GetError());
			cout << outcome.GetError().GetMessage() << endl;
			break;
		}
		//elapsed = std::chrono::high_resolution_clock::now() - start;
		//microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

		//cout << "second took: " << microseconds / 1000 << " milliseconds" << endl;
		break;

	}
}

void CCParser::unzip_chunk(int bytes_in) {

	int ret, bytes_out;

	m_zstream.next_in = (unsigned char *)m_z_buffer_in;
	m_zstream.avail_in = bytes_in;
	cout << bytes_in << endl;
	do {
		m_zstream.avail_out = CC_PARSER_ZLIB_OUT;
		cout << "avail_out: " << m_zstream.avail_out << endl;
		m_zstream.next_out = (unsigned char *)m_z_buffer_out;
		ret = inflate(&m_zstream, Z_SYNC_FLUSH);

		switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;
			case Z_DATA_ERROR:
				cout << "Z_DATA_ERROR" << endl;
				break;
			case Z_MEM_ERROR:
				cout << "Z_MEM_ERROR" << endl;
				break;
			case Z_STREAM_END:
				cout << "Z_STREAM_END" << endl;
				break;
		}

		bytes_out = CC_PARSER_ZLIB_OUT - m_zstream.avail_out;

		handle_chunk(m_z_buffer_out, bytes_out);

		cout << "avail_out: " << m_zstream.avail_out << endl;

	} while (m_zstream.avail_out == 0);

}

void CCParser::handle_chunk(const char *buffer, int len) {
	cout << "len: " << len << endl;
	cout << string(buffer, len) << endl;
	//printf("%.*s", len, buffer);
}

static invocation_response my_handler(invocation_request const& req, Aws::S3::S3Client const& client)
{
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
	parser.run();

	return invocation_response::success("We did it!", "application/base64");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory()
{
	return [] {
		return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
			"console_logger", Aws::Utils::Logging::LogLevel::Trace);
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
	FILE *fp_in = fopen("CC-MAIN-20210228145113-20210228175113-00135.warc.gz", "rb");
	FILE *fp_out = fopen("out.warc", "wb");
	int consumed = 0;
	int consumed_total = 0;
	while (true) {
		consumed = inf(fp_in, fp_out);
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
	return 0;
	Aws::SDKOptions options;
	Aws::InitAPI(options);

	//run_lambda_handler();
	/*if (argc == 1) {
	} else {
	*/
	Aws::Client::ClientConfiguration clientConfig;
	clientConfig.region = "us-east-1";
	CCParser parser(Aws::S3::S3Client(clientConfig), "commoncrawl", "crawl-data/CC-MAIN-2021-10/segments/1614178361510.12/warc/CC-MAIN-20210228145113-20210228175113-00135.warc.gz");
	parser.run();
	//}

	Aws::ShutdownAPI(options);

	return 0;
}
