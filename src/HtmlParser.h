
#pragma once

#include "common.h"
#include "Link.h"
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string.h>
#include <boost/algorithm/string.hpp>

using namespace std;

#define HTML_PARSER_LONG_TEXT_LEN 1000
#define HTML_PARSER_SHORT_TEXT_LEN 300
#define HTML_PARSER_CLEANBUF_LEN 1024
#define IS_MULTIBYTE_CODEPOINT(ch) ((ch >> 7) && !((ch >> 6) & 0x1))

class HtmlParser {

public:

	HtmlParser();
	~HtmlParser();

	void parse(const string &html, const string &url);
	void parse(const string &html);

	string title();
	string meta();
	string h1();
	string text();
	vector<Link> links();
	bool should_insert();

	// Return top level domain
	string url_tld(const string &url);
	bool is_exotic_language_debug(const string &str) const;
	bool is_exotic_language(const string &str) const;

private:

	vector<Link> m_links;
	vector<pair<int, int>> m_invisible_pos;

	char m_clean_buff[HTML_PARSER_CLEANBUF_LEN];
	char m_long_str_buf[HTML_PARSER_LONG_TEXT_LEN];
	bool m_should_insert;
	string m_title;
	string m_h1;
	string m_meta;
	string m_text;

	string m_host;
	string m_path;

	void find_scripts(const string &html);
	void find_styles(const string &html);
	void find_links(const string &html);

	int parse_link(const string &link);
	int parse_url(const string &url, string &host, string &path);

	inline string lower_case(const string &str);
	inline pair<size_t, size_t> find_tag(const string &html, const string &tag_start, const string &tag_end,
		size_t pos);
	inline string get_tag_content(const string &html, const string &tag_start, const string &tag_end);
	inline string get_meta_tag(const string &html);
	inline void clean_text(string &str);
	inline void strip_whitespace(string &html);
	inline void strip_tags(string &html);
	inline string get_text_after_h1(const string &html);

};
