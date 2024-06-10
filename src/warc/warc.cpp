
#include "warc.h"
#include "tlds.h"
#include "text/text.h"
#include "logger/logger.h"
#include "transfer/transfer.h"

using namespace std;

namespace warc {

	parser::parser() {
		m_z_buffer_in = new char[WARC_PARSER_ZLIB_IN];
		m_z_buffer_out = new char[WARC_PARSER_ZLIB_OUT];
	}

	parser::~parser() {
		delete [] m_z_buffer_in;
		delete [] m_z_buffer_out;
	}

	bool parser::parse_stream(istream &stream) {
		return parse_stream(stream, [this](const std::string &url, const ::parser::html_parser &html, const std::string &ip, const std::string &date) {
			handle_html(url, html, ip, date);
		});
	}

	bool parser::parse_stream(std::istream &stream, std::function<void(const std::string &url, const ::parser::html_parser &html, const std::string &ip,
				const std::string &date)> callback) {
		m_callback = callback;
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

	void parser::handle_html(const std::string &url, const ::parser::html_parser &html, const std::string &ip, const std::string &date) {

		m_result += (url
				+ '\t' + html.title()
				+ '\t' + html.h1()
				+ '\t' + html.meta()
				+ '\t' + html.text()
				+ '\t' + date
				+ '\t' + ip
				+ '\n');
		for (const auto &link : html.links()) {
			m_links += (link.host()
				+ '\t' + link.path()
				+ '\t' + link.target_host()
				+ '\t' + link.target_path()
				+ '\t' + link.text()
				+ '\t' + (link.nofollow() ? "1" : "0")
				+ '\n');
		}

		// internal links are too messy for us now.
		/*for (const auto &link : html.internal_links()) {
			// link is a std::pair<uint64_t, uint64_t>
			m_internal_links.append((char *)&link.first, sizeof(uint64_t));
			m_internal_links.append((char *)&link.second, sizeof(uint64_t));
		}*/

	}

	int parser::unzip_record(char *data, int size) {

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
					cout << "Z_MEM_ERROR" << endl;
					(void)inflateEnd(&m_zstream);
					return -1;
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

	int parser::unzip_chunk(int bytes_in) {

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
	void parser::handle_record_chunk(char *data, int len) {

		m_handled += len;
		m_num_handled++;

		if (len > 8 && strncmp(data, "WARC/1.0", 8) == 0) {
			// data is the start of a warc record
			string record(data, len);
			m_current_record.assign(data, len);
		} else {
			m_current_record.append(data, len);
		}

		if (m_current_record.find("\r\n\r\n") != string::npos) {

			const string warc_header = get_warc_header(m_current_record);
			const string content_len_str = ::parser::get_http_header(warc_header, "Content-Length: ");

			size_t content_len = stoull(content_len_str);
			size_t received_content = m_current_record.size() - (warc_header.size() + 8);

			if (content_len == received_content) {
				const string type = ::parser::get_http_header(warc_header, "WARC-Type: ");

				if (type == "response") {
					parse_record(warc_header, m_current_record);
				}
			}
		}

	}

	void parser::parse_record(const string &warc_header, const string &warc_record) {

		const string url = ::parser::get_http_header(warc_header, "WARC-Target-URI: ");
		const string tld = m_html_parser.url_tld(url);

		if (tlds.count(tld) == 0) return;

		const string ip = ::parser::get_http_header(warc_header, "WARC-IP-Address: ");
		const string date = ::parser::get_http_header(warc_header, "WARC-Date: ");

		const size_t warc_response_start = warc_record.find("\r\n\r\n");
		const size_t response_body_start = warc_record.find("\r\n\r\n", warc_response_start + 4);

		string http_header = warc_record.substr(warc_response_start + 4, response_body_start - warc_response_start - 4);
		text::lower_case(http_header);

		//const size_t http_code = http_response_code(http_header);
		//const string location = ::parser::get_http_header(warc_header, "location: ");

		string html = warc_record.substr(response_body_start + 4);
		m_html_parser.parse(html, url);

		if (m_html_parser.should_insert()) {
			m_callback(url, m_html_parser, ip, date);
		}
	}

	string parser::get_warc_header(const string &record) {
		const size_t pos = record.find("\r\n\r\n");
		return record.substr(0, pos);
	}

	size_t parser::http_response_code(const string &http_header) {
		const size_t return_on_invalid = 500;
		const size_t code_start = http_header.find(' ');
		const size_t code_end = http_header.find(' ', code_start);
		if (code_start == string::npos || code_end == string::npos) return return_on_invalid;

		size_t response_code = stoull(http_header.substr(code_start + 1, 3));

		if (response_code < 100 || response_code >= 600) return return_on_invalid;

		return response_code;
	}

	void multipart_download(const string &url, const std::function<void(const string &chunk)> &callback) {

		int error;
		size_t content_len = transfer::head_content_length(url, error);

		if (error == transfer::ERROR) {
			throw std::runtime_error("Could not make HEAD request to: " + url);
		}

		const size_t max_parts = 50;
		const size_t max_retries = 3;

		size_t part = 1;
		size_t read_bytes = 0;
		while (read_bytes < content_len && part < max_parts) {
			size_t retry = 0;
			while (retry < max_retries) {
				string buffer;
				transfer::url_to_string(url + "?partNumber=" + to_string(part), buffer, error);
				if (error == transfer::OK) {
					read_bytes += buffer.size();
					callback(buffer);
					break;
				} else {
					throw std::runtime_error("Got error response");
				}
				retry++;
			}
			if (retry == max_retries) {
				break;
			}
			part++;
		}
	}

	string get_result_path(const string &warc_path) {
		string path = warc_path;
		path.replace(path.find(".warc.gz"), 8, string(".gz"));
		return path;
	}

	string get_link_result_path(const string &warc_path) {
		string path = warc_path;
		path.replace(path.find(".warc.gz"), 8, string(".links.gz"));
		return path;
	}

	string get_internal_link_result_path(const string &warc_path) {
		string path = warc_path;
		path.replace(path.find(".warc.gz"), 8, string(".internal.gz"));
		return path;
	}

}

