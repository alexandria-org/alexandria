
#pragma once

#include <iostream>
#include "parser/html_parser.h"
#include "parser/parser.h"
#include "zlib.h"

#define WARC_PARSER_ZLIB_IN 1024*1024*16
#define WARC_PARSER_ZLIB_OUT 1024*1024*16

namespace warc {

	using std::string;

	class parser {

		public:

			parser();
			~parser();

			bool parse_stream(std::istream &stream);
			bool parse_stream(std::istream &stream, std::function<void(const std::string &url, const ::parser::html_parser &html, const std::string &ip,
						const std::string &date)>);
			const string &result() const { return m_result; };
			const string &link_result() const { return m_links; };
			const string &internal_link_result() const { return m_internal_links; };
			void handle_html(const std::string &url, const ::parser::html_parser &html, const std::string &ip, const std::string &date);

		private:

			int m_cur_offset = 0;
			bool m_continue_inflate = false;
			std::string m_result;
			std::string m_links;
			std::string m_internal_links;
			::parser::html_parser m_html_parser;
			std::function<void(const std::string &url, const ::parser::html_parser &html, const std::string &ip, const std::string &date)>
				m_callback;

			char *m_z_buffer_in;
			char *m_z_buffer_out;

			z_stream m_zstream; /* decompression stream */

			size_t m_handled = 0;
			size_t m_num_handled = 0;
			string m_current_record;

			int unzip_record(char *data, int size);
			int unzip_chunk(int bytes_in);

			void handle_record_chunk(char *data, int len);
			void parse_record(const std::string &warc_header, const std::string &warc_record);
			std::string get_warc_header(const std::string &record);
			size_t http_response_code(const string &http_header);

	};

	void multipart_download(const string &url, const std::function<void(const string &chunk)> &callback);

	string get_result_path(const string &warc_path);
	string get_link_result_path(const string &warc_path);
	string get_internal_link_result_path(const string &warc_path);
}
