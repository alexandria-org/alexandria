
#pragma once

#include <iostream>
#include "HtmlParser.h"
#include "zlib.h"

#define WARC_PARSER_ZLIB_IN 1024*1024*16
#define WARC_PARSER_ZLIB_OUT 1024*1024*16

namespace Warc {

	class Parser {

		public:

			Parser();
			~Parser();

			bool parse_stream(std::istream &stream);
			//const string &result() const { return m};

		private:

			//const vector<string> m_tlds = {"se", "com", "nu", "net", "org", "gov", "edu", "info"};

			int m_cur_offset = 0;
			bool m_continue_inflate = false;
			std::string m_result;
			std::string m_links;
			HtmlParser m_html_parser;

			char *m_z_buffer_in;
			char *m_z_buffer_out;

			z_stream m_zstream; /* decompression stream */

			int unzip_record(char *data, int size);
			int unzip_chunk(int bytes_in);

			void handle_record_chunk(char *data, int len);
			void parse_record(const std::string &record, const std::string &url);
			std::string get_warc_record(const std::string &record, const std::string &key, int &offset);

	};

	void cparse(std::istream &infile);

}
