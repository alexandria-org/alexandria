
#include "entities.h"
#include "HtmlParser.h"

const vector<string> non_content_tags{"script", "noscript", "style", "embed", "label", "form", "input",
	"iframe", "head", "meta", "link", "object", "aside", "channel", "img"};

// trim from start (in place)
void ltrim(string &s) {
	s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch) {
		return !isspace(ch) && !ispunct(ch);
	}));
}

// trim from end (in place)
void rtrim(string &s) {
	s.erase(find_if(s.rbegin(), s.rend(), [](int ch) {
		return !isspace(ch) && !ispunct(ch);
	}).base(), s.end());
}

void trim(string &s) {
	ltrim(s);
	rtrim(s);
}

HtmlParser::HtmlParser() {
}

HtmlParser::HtmlParser(int max_text_len) {
	m_max_text_len = max_text_len;
}

HtmlParser::~HtmlParser() {

}

void HtmlParser::parse(const string &html) {

	m_title = get_tag_content(html, "<title>", "</title>");
	m_h1 = get_tag_content(html, "<h1>", "</h1>");
	m_meta = get_meta_tag(html);
	m_text = get_text_after_h1(html);

	clean_text(m_title);
	clean_text(m_h1);
	clean_text(m_meta);
	clean_text(m_text);

	/*m_title.resize(HTML_PARSER_SHORT_TEXT_LEN);
	m_h1.resize(HTML_PARSER_SHORT_TEXT_LEN);
	m_meta.resize(HTML_PARSER_LONG_TEXT_LEN);*/

	trim(m_title);
	trim(m_h1);
	trim(m_meta);
	trim(m_text);
}

string HtmlParser::title() {
	return m_title;
} 

string HtmlParser::meta() {
	return m_meta;
}

string HtmlParser::h1() {
	return m_h1;
}

string HtmlParser::text() {
	return m_text;
}

vector<Link> HtmlParser::links(const string &html) {
	vector<Link> links;
	return links;
}

inline string HtmlParser::lower_case(const string &str) {
	string ret = str;
	transform(ret.begin(), ret.end(), ret.begin(), [](unsigned char c){ return tolower(c); });
	return ret;
}

inline string HtmlParser::get_tag_content(const string &html, const string &tag_start, const string &tag_end) {
	const size_t pos_start = html.find(tag_start);
	const size_t pos_end = html.find(tag_end, pos_start);
	const size_t len = pos_end - pos_start;
	if (pos_start == string::npos || pos_end == string::npos) return "";
	return (string)html.substr(pos_start + tag_start.size(), len - tag_start.size());
}

inline string HtmlParser::get_meta_tag(const string &html) {
	const size_t pos_start = html.find("<meta");
	const size_t pos_end = html.find("description", pos_start);
	const size_t pos_end_tag = html.find(">", pos_end);
	if (pos_start == string::npos || pos_end == string::npos || pos_end_tag == string::npos) return "";

	const string s = "content=";
	const size_t content_start = html.rfind(s, pos_end_tag);
	if (content_start == string::npos) return "";

	return (string)html.substr(content_start + s.size(), pos_end_tag - content_start - s.size());
}

inline void HtmlParser::clean_text(string &str) {
	strip_tags(str);
	if (str.size() >= HTML_PARSER_CLEANBUF_LEN) return;
	decode_html_entities_utf8(m_clean_buff, str.c_str());
	str = m_clean_buff;
}

inline void HtmlParser::strip_tags(string &html) {
	const int len = html.size();
	bool copy = true;
	bool last_was_space = false;
	int i = 0, j = 0;
	for (; i < len; i++) {
		if (html[i] == '<') copy = false;
		if (isspace(html[i])) {
			html[j] = ' ';
			if (copy && !last_was_space) j++;
			last_was_space = true;
		} else {
			html[j] = html[i];
			if (copy) j++;
			last_was_space = false;
		}
		if (html[i] == '>') copy = true;
	}
	html.resize(j);
}

inline string HtmlParser::get_text_after_h1(const string &html) {
	string text(m_max_text_len, 0);
	const size_t pos_start = html.find("</h1>");

	if (pos_start == string::npos) return "";

	const int len = html.size();
	bool copy = true;
	bool last_was_space = false;
	int i = pos_start, j = 0;
	for (; i < len && j < m_max_text_len; i++) {
		if (html[i] == '<') copy = false;
		if (isspace(html[i])) {
			text[j] = ' ';
			if (copy && !last_was_space) j++;
			last_was_space = true;
		} else {
			text[j] = html[i];
			if (copy) j++;
			last_was_space = false;
		}
		if (html[i] == '>') copy = true;
	}
	text.resize(j);

	return text;
}

