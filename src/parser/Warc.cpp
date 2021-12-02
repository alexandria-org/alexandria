
#include "Warc.h"

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

	void Parser::handle_record_chunk(char *data, int len) {

		string record(data, len);

		int type_offset;
		int uri_offset;
		const string type = get_warc_record(record, "WARC-Type: ", type_offset);

		if (type == "response") {
			const string url = get_warc_record(record, "WARC-Target-URI: ", uri_offset);
			const string tld = m_html_parser.url_tld(url);
			parse_record(record, url);
		}

	}

	void Parser::parse_record(const string &record, const string &url) {

		const size_t warc_response_start = record.find("\r\n\r\n");
		const size_t response_body_start = record.find("\r\n\r\n", warc_response_start + 4);

		string html = record.substr(response_body_start + 4);
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

	string Parser::get_warc_record(const string &record, const string &key, int &offset) {
		const size_t pos = record.find(key);
		const size_t pos_end = record.find("\n", pos);
		if (pos == string::npos || pos_end == string::npos) {
			return "";
		}

		return record.substr(pos + key.size(), pos_end - pos - key.size() - 1);
	}

}

