
#include "Warc.h"
#include "text/Text.h"

using namespace std;

namespace Warc {

		
	Parser::Parser() {
		m_z_buffer_in = new char[WARC_PARSER_ZLIB_IN];
		m_z_buffer_out = new char[WARC_PARSER_ZLIB_OUT];
	}

	Parser::~Parser() {
		delete m_z_buffer_in;
		delete m_z_buffer_out;
	}

	bool Parser::parse_stream(istream &stream) {

		size_t total_bytes_read = 0;
		while (stream.good()) {
			stream.read(m_z_buffer_in, WARC_PARSER_ZLIB_IN);

			auto bytes_read = stream.gcount();
			total_bytes_read += bytes_read;

			if (bytes_read > 0) {
				if (unzip_chunk(bytes_read) < 0) {
					cout << "Stopped because fatal error" << endl;
					break;
				}
			}
		}

		return true;
	}

	int Parser::unzip_record(char *data, int size) {

		/*
			data is:
			#|------------------|-----|------------------------|--|----#-------|
			 |doc_a______________doc_b_doc_c_____|
								 WARC_PARSER_ZLIB_IN
			 |_________________________________________________________|
																   size
		*/

		int data_size = size;
		int consumed = 0, consumed_total = 0;
		int avail_in_before_inflate;
		int ret = Z_OK;
		unsigned have;

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

			m_zstream.avail_in = min(WARC_PARSER_ZLIB_IN, data_size);

			if (m_zstream.avail_in == 0)
				break;

			/* run inflate() on input until output buffer not full */
			do {

				m_zstream.avail_out = WARC_PARSER_ZLIB_OUT;
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

				have = WARC_PARSER_ZLIB_OUT - m_zstream.avail_out;
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

	int Parser::unzip_chunk(int bytes_in) {

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

	/*
	 * Handles unzipped data. The data pointer is either pointing to a new warc record or it is the continuation of a previous warc record.
	 * */
	void Parser::handle_record_chunk(char *data, int len) {

		m_handled += len;
		m_num_handled++;

		if (len > 8 && strncmp(data, "WARC/1.0", 8) == 0) {
			// data is the start of a warc record
			string record(data, len);
			m_current_record.assign(data, len);
		} else {
			m_current_record.append(data, len);
		}

		const string warc_header = get_warc_header(m_current_record);
		const string content_len_str = get_header(warc_header, "Content-Length: ");

		size_t content_len = stoull(content_len_str);
		size_t received_content = m_current_record.size() - (warc_header.size() + 8);

		if (content_len == received_content) {
			const string type = get_header(warc_header, "WARC-Type: ");

			if (type == "response") {
				parse_record(warc_header, m_current_record);
			}
		}

	}

	void Parser::parse_record(const string &warc_header, const string &warc_record) {

		const string url = get_header(warc_header, "WARC-Target-URI: ");
		const string tld = m_html_parser.url_tld(url);
		const string ip = get_header(warc_header, "WARC-IP-Address: ");
		const string date = get_header(warc_header, "WARC-Date: ");

		const size_t warc_response_start = warc_record.find("\r\n\r\n");
		const size_t response_body_start = warc_record.find("\r\n\r\n", warc_response_start + 4);

		string http_header = warc_record.substr(warc_response_start + 4, response_body_start - warc_response_start - 4);
		Text::lower_case(http_header);

		const size_t http_code = http_response_code(http_header);
		const string location = get_header(warc_header, "location: ");

		cout << "code: " << http_code << endl;
		if (m_num_handled > 10) {
			//exit(0);
		}

		string html = warc_record.substr(response_body_start + 4);
		m_html_parser.parse(html, url);

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
	}

	string Parser::get_warc_header(const string &record) {
		const size_t pos = record.find("\r\n\r\n");
		return record.substr(0, pos);
	}

	string Parser::get_header(const string &record, const string &key) {
		const size_t pos = record.find(key);
		const size_t pos_end = record.find("\n", pos);
		if (pos == string::npos) {
			return "";
		}

		if (pos_end == string::npos) {
			return record.substr(pos + key.size());
		}

		return record.substr(pos + key.size(), pos_end - pos - key.size() - 1);
	}

	size_t Parser::http_response_code(const string &http_header) {
		const size_t return_on_invalid = 500;
		const size_t code_start = http_header.find(' ');
		const size_t code_end = http_header.find(' ', code_start);
		if (code_start == string::npos || code_end == string::npos) return return_on_invalid;

		size_t response_code = stoull(http_header.substr(code_start + 1, 3));

		if (response_code < 100 || response_code >= 600) return return_on_invalid;

		return response_code;
	}

}

