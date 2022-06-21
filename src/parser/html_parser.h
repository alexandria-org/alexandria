/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string.h>
#include <boost/algorithm/string.hpp>

#include "html_link.h"
#include "parser/unicode.h"

#define HTML_PARSER_LONG_TEXT_LEN 1000
#define HTML_PARSER_MAX_H1_LEN 400
#define HTML_PARSER_MAX_TITLE_LEN 400

#define HTML_PARSER_CLEANBUF_LEN 1024
#define HTML_PARSER_ENCODING_BUFFER_LEN 8192

#define ENC_UTF_8 1
#define ENC_ISO_8859_1 2
#define ENC_UNKNOWN -1

namespace parser {

	class html_parser {

	public:

		html_parser();
		html_parser(size_t long_text_len);
		~html_parser();

		void parse(const std::string &html, const std::string &url);
		void parse(const std::string &html);

		std::string title() const;
		std::string meta() const;
		std::string h1() const;
		std::string text() const;
		std::vector<html_link> links() const;
		std::vector<std::pair<uint64_t, uint64_t>> internal_links() const;
		bool should_insert() const;

		// Return top level domain
		std::string url_tld(const std::string &url);
		bool is_exotic_language_debug(const std::string &str) const;
		bool is_exotic_language(const std::string &str) const;

	private:

		std::vector<html_link> m_links;
		std::vector<std::pair<uint64_t, uint64_t>> m_internal_links;
		std::vector<std::pair<size_t, size_t>> m_invisible_pos;

		const size_t m_long_text_len = 1000;
		char m_clean_buff[HTML_PARSER_CLEANBUF_LEN];
		const size_t m_long_str_buf_len;
		char *m_long_str_buf;
		unsigned char m_encoding_buffer[HTML_PARSER_ENCODING_BUFFER_LEN];
		bool m_should_insert;
		int m_encoding = ENC_UNKNOWN;

		std::string m_title;
		std::string m_h1;
		std::string m_meta;
		std::string m_text;

		std::string m_host;
		std::string m_path;

		void find_scripts(const std::string &html);
		void find_styles(const std::string &html);
		void find_links(const std::string &html, const std::string &base_url);

		int parse_link(const std::string &link, const std::string &base_url);
		int parse_url(const std::string &url, std::string &host, std::string &path, const std::string &base_url);
		inline void remove_www(std::string &path);
		void parse_encoding(const std::string &html);
		void iso_to_utf8(std::string &text);

		inline std::pair<size_t, size_t> find_tag(const std::string &html, const std::string &tag_start, const std::string &tag_end,
			size_t pos);
		std::string get_tag_content(const std::string &html, const std::string &tag_start, const std::string &tag_end);
		std::string get_meta_tag(const std::string &html);
		void clean_text(std::string &str);
		void strip_whitespace(std::string &html);
		void strip_tags(std::string &html);
		std::string get_text_content(const std::string &html);
		void sort_invisible();
		inline bool is_invisible(size_t pos);

	};

}
