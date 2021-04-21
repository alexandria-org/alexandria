
#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <cctype>

using namespace std;

#define HTML_PARSER_LONG_TEXT_LEN 1000
#define HTML_PARSER_SHORT_TEXT_LEN 300
#define HTML_PARSER_CLEANBUF_LEN 1024

class Link {
public:
	string m_url;
	string m_text;
	string m_rel;
};

class HtmlParser {

public:

	HtmlParser();
	HtmlParser(int max_text_len);
	~HtmlParser();

	void parse(const string &html);

	string title();
	string meta();
	string h1();
	string text();
	vector<Link> links(const string &html);

private:

	char m_clean_buff[HTML_PARSER_CLEANBUF_LEN];
	string m_title;
	string m_h1;
	string m_meta;
	string m_text;

	inline string lower_case(const string &str);
	inline string get_tag_content(const string &html, const string &tag_start, const string &tag_end);
	inline string get_meta_tag(const string &html);
	inline void clean_text(string &str);
	inline void strip_tags(string &html);
	inline string get_text_after_h1(const string &html);

	int m_max_text_len = 1000;

};
