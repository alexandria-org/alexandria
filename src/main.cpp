
// main.cpp
#include "parser/HtmlParser.h"
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

#define RUN_ON_LAMBDA 1
#define CC_PARSER_CHUNK 1024*1024*600
//#define CC_PARSER_CHUNK 0
#define CC_PARSER_ZLIB_IN 1024*1024*16
#define CC_PARSER_ZLIB_OUT 1024*1024*16
#define CC_TARGET_BUCKET "alexandria-cc-output"

class CCParser {

public:

	CCParser(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key);
	~CCParser();

	bool run(string &response);

private:

	const vector<string> m_tlds = {"se", "com", "nu", "net", "org", "gov", "edu", "info"};

	string m_bucket;
	string m_key;
	Aws::S3::S3Client m_s3_client;
	int m_cur_offset = 0;
	bool m_continue_inflate = false;
	string m_result;
	string m_links;
	HtmlParser m_html_parser;

	char *m_z_buffer_in;
	char *m_z_buffer_out;

	z_stream m_zstream; /* decompression stream */

	string next_range();
	int unzip_record(char *data, int size);
	int unzip_chunk(int bytes_in);
	void handle_chunk(const char *buffer, int len);

	void handle_record_chunk(char *data, int len);
	void parse_record(const string &record, const string &url);
	string get_warc_record(const string &record, const string &key, int &offset);

	void upload_result();
	void upload_links();

};

CCParser::CCParser(const Aws::S3::S3Client &s3_client, const string &bucket, const string &key) {
	m_bucket = bucket;
	m_key = key;
	m_s3_client = s3_client;

	m_z_buffer_in = new char[CC_PARSER_ZLIB_IN];
	m_z_buffer_out = new char[CC_PARSER_ZLIB_OUT];
}

CCParser::~CCParser() {
	delete m_z_buffer_in;
	delete m_z_buffer_out;
}

string CCParser::next_range() {
	string ret = "bytes=" + to_string(m_cur_offset) + "-" + to_string(m_cur_offset + CC_PARSER_CHUNK - 1);
	return ret;
}

bool CCParser::run(string &response) {

	Aws::S3::Model::GetObjectRequest request;
	request.SetBucket(m_bucket);
	request.SetKey(m_key);

	// Main loop
	auto total_start = std::chrono::high_resolution_clock::now();
	bool fatal_error = false;
	while (!fatal_error) {
		auto start = std::chrono::high_resolution_clock::now();
		//cout << "fetching range: " << next_range() << endl;

		if (CC_PARSER_CHUNK) {
			request.SetRange(next_range());
		}

		auto outcome = m_s3_client.GetObject(request);

		if (outcome.IsSuccess()) {

			m_cur_offset += CC_PARSER_CHUNK;

			auto &stream = outcome.GetResultWithOwnership().GetBody();
			stream.seekg(0, stream.end);
			int length = stream.tellg();
			stream.seekg(0, stream.beg);

			cout << "Got stream with " << length << " bytes" << endl;

			if (stream.bad()) {
				cout << "Badbit was set" << endl;
				return false;
			}
			if (stream.fail()) {
				cout << "Failbit was set" << endl;
				return false;
			}
			if (stream.eof()) {
				cout << "Eofbit was set" << endl;
			}

			
			int total_bytes_read = 0;
			while (stream.good()) {
				stream.read(m_z_buffer_in, CC_PARSER_ZLIB_IN);

				auto bytes_read = stream.gcount();
				total_bytes_read += bytes_read;

				if (bytes_read > 0) {
					if (unzip_chunk(bytes_read) < 0) {
						//cout << "Stopped because fatal error" << endl;
						fatal_error = true;
						break;
					}
				}
			}

			//cout << "total bytes read: " << total_bytes_read << endl;

			if (total_bytes_read == 0) {
				cout << "Stopped because gcount was 0" << endl;
				break;
			}


		} else {
			//AWS_LOGSTREAM_ERROR(TAG, "Failed with error: " << outcome.GetError());
			cout << "WE ARE HERE NOW AND GOT AN ERROR, probably the stream has ended" << endl;
			break;
		}
		auto elapsed = std::chrono::high_resolution_clock::now() - start;
		auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
		cout << "offset: " << m_cur_offset << " took: " << microseconds / 1000 << " milliseconds" << endl;
		/*if (m_cur_offset > 1024*1024*100) {
			break;
		}*/
		if (!CC_PARSER_CHUNK) {
			break;
		}
	}

	auto total_elapsed = std::chrono::high_resolution_clock::now() - total_start;
	auto total_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(total_elapsed).count();

	upload_result();
	upload_links();

	response = string("We read ") + to_string(m_cur_offset) + " bytes in total of " + to_string(total_microseconds / 1000) + "ms";

	return true;
}

int CCParser::unzip_record(char *data, int size) {

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
	if (ret == Z_OK || ret == Z_BUF_ERROR) {
		m_continue_inflate = true;
	} else {
		m_continue_inflate = false;
		(void)inflateEnd(&m_zstream);
	}

	/* clean up and return */
	return consumed_total;
}

int CCParser::unzip_chunk(int bytes_in) {

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

void CCParser::handle_chunk(const char *buffer, int len) {
	cout << "len: " << len << endl;
	cout << string(buffer, len) << endl;
	//printf("%.*s", len, buffer);
}

void CCParser::handle_record_chunk(char *data, int len) {

	string record(data, len);

	int type_offset;
	int uri_offset;
	const string type = get_warc_record(record, "WARC-Type: ", type_offset);

	if (type == "response") {
		const string url = get_warc_record(record, "WARC-Target-URI: ", uri_offset);
		const string tld = m_html_parser.url_tld(url);
		if (find(m_tlds.begin(), m_tlds.end(), tld) != m_tlds.end()) {

			parse_record(record, url);

		}
	}

}

void CCParser::parse_record(const string &record, const string &url) {

	m_html_parser.parse(record, url);

	if (m_html_parser.should_insert()) {
		m_result += (url
			+ '\t' + m_html_parser.title()
			+ '\t' + m_html_parser.h1()
			+ '\t' + m_html_parser.meta()
			+ '\t' + m_html_parser.text()
			+ '\n');
		for (const auto &link : m_html_parser.links()) {
			m_links += (link.host()
				+ '\t' + link.path()
				+ '\t' + link.target_host()
				+ '\t' + link.target_path()
				+ '\t' + link.text()
				+ '\n');
		}
	}
	//cout << m_links << endl;
	//if (m_html_parser.links().size() > 100) {
		/*if (url == "http://store.chessgames.com/chess-books/chess-notation-type/an---algebraic/author/s/alexander-cherniaev-anatoly-karpov-joe-gallagher-joel-r.-steed-miguel-a.-sanchez-richard-obrien/hardware-requirements/windows.html") {
			cout << record << endl;
		}
		cout << url << endl;
		cout << "link array size: " << m_html_parser.links().size() << endl;*/
		//cout << "result size is: " << m_result.size() << endl;
		//cout << "links  size is: " << m_links.size() << endl;
		/*
		if (url == "http://store.chessgames.com/chess-books/chess-notation-type/an---algebraic/author/s/alexander-cherniaev-anatoly-karpov-joe-gallagher-joel-r.-steed-miguel-a.-sanchez-richard-obrien/hardware-requirements/windows.html") {
			exit(0);
		}*/
	//}
	//exit(0);
}

string CCParser::get_warc_record(const string &record, const string &key, int &offset) {
	const int pos = record.find(key);
	const int pos_end = record.find("\n", pos);
	if (pos == string::npos || pos_end == string::npos) {
		return "";
	}

	return record.substr(pos + key.size(), pos_end - pos - key.size() - 1);
}

void CCParser::upload_result() {

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket(CC_TARGET_BUCKET);
	string key = m_key;
	key.replace(key.find(".warc.gz"), 8, string(".gz"));
	request.SetKey(key);

	stringstream ss;
	ss << m_result;

	filtering_istream in;
	in.push(gzip_compressor());
	in.push(ss);

	std::shared_ptr<Aws::StringStream> request_body = Aws::MakeShared<Aws::StringStream>("");
	*request_body << in.rdbuf();
	request.SetBody(request_body);

	Aws::S3::Model::PutObjectOutcome outcome = m_s3_client.PutObject(request);

	if (outcome.IsSuccess()) {

	    std::cout << "Added object '" << m_key << "' to bucket '" << CC_TARGET_BUCKET << "'.";
	} else {
		std::cout << "Error: PutObject: " <<
			outcome.GetError().GetMessage() << std::endl;
	}
}

void CCParser::upload_links() {

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket(CC_TARGET_BUCKET);
	string key = m_key;
	key.replace(key.find(".warc.gz"), 8, string(".links.gz"));
	request.SetKey(key);

	stringstream ss;
	ss << m_links;

	filtering_istream in;
	in.push(gzip_compressor());
	in.push(ss);

	std::shared_ptr<Aws::StringStream> request_body = Aws::MakeShared<Aws::StringStream>("");
	*request_body << in.rdbuf();
	request.SetBody(request_body);

	Aws::S3::Model::PutObjectOutcome outcome = m_s3_client.PutObject(request);

	if (outcome.IsSuccess()) {

	    std::cout << "Added object '" << m_key << "' to bucket '" << CC_TARGET_BUCKET << "'.";
	} else {
	    std::cout << "Error: PutObject: " <<
	        outcome.GetError().GetMessage() << std::endl;

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
	CCParser parser(client, bucket, key);
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
		//CCParser parser(Aws::S3::S3Client(getS3Config()), "commoncrawl", "crawl-data/CC-MAIN-2021-10/segments/1614178361510.12/warc/CC-MAIN-20210228145113-20210228175113-00135.warc.gz");
		CCParser parser(Aws::S3::S3Client(getS3Config()), "commoncrawl", "crawl-data/CC-MAIN-2021-17/segments/1618038056325.1/warc/CC-MAIN-20210416100222-20210416130222-00002.warc.gz");
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

