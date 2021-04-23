
#include "entities.h"
#include "HtmlParser.h"
#include <curl/curl.h>

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

HtmlParser::~HtmlParser() {
}

void HtmlParser::parse(const string &html) {
	parse(html, "");
}

void HtmlParser::parse(const string &html, const string &url) {

	m_should_insert = false;

	parse_url(url, m_host, m_path);

	m_title.clear();
	m_h1.clear();
	m_meta.clear();
	m_text.clear();
	m_invisible_pos.clear();
	m_links.clear();

	find_scripts(html);
	find_styles(html);
	find_links(html);

	m_title = get_tag_content(html, "<title", "</title>");
	clean_text(m_title);
	if (m_title.size() == 0 || is_exotic_language(m_title)) return;

	m_should_insert = true;

	m_h1 = get_tag_content(html, "<h1", "</h1>");
	m_meta = get_meta_tag(html);
	m_text = get_text_after_h1(html);

	clean_text(m_h1);
	clean_text(m_meta);
	clean_text(m_text);

	/*m_title.resize(HTML_PARSER_SHORT_TEXT_LEN);
	m_h1.resize(HTML_PARSER_SHORT_TEXT_LEN);
	m_meta.resize(HTML_PARSER_LONG_TEXT_LEN);*/
}

void HtmlParser::find_scripts(const string &html) {
	size_t pos = 0;
	pair<size_t, size_t> tag(0, 0);
	while (pos != string::npos) {
		tag = find_tag(html, "<script", "</script>", tag.second);
		if (tag.second == string::npos) {
			break;
		}
		m_invisible_pos.push_back(tag);
	}
}

void HtmlParser::find_styles(const string &html) {
	size_t pos = 0;
	pair<size_t, size_t> tag(0, 0);
	while (pos != string::npos) {
		tag = find_tag(html, "<style", "</style>", tag.second);
		if (tag.second == string::npos) {
			break;
		}
		m_invisible_pos.push_back(tag);
	}
}

void HtmlParser::find_links(const string &html) {
	size_t pos = 0;
	pair<size_t, size_t> tag(0, 0);
	while (pos != string::npos) {
		tag = find_tag(html, "<a ", "</a>", tag.second);
		if (tag.second == string::npos) {
			break;
		}
		parse_link(html.substr(tag.first, tag.second - tag.first));
	}
}

int HtmlParser::parse_link(const string &link) {
	const string href_key = "href=\"";
	const size_t key_len = href_key.size();
	const size_t href_start = link.find(href_key);
	if (href_start == string::npos) return CC_ERROR;
	const size_t href_end = link.find("\"", href_start + key_len);
	if (href_end == string::npos) return CC_ERROR;
	string href = link.substr(href_start + key_len, href_end - href_start - key_len);
	if (href.find("http") != 0) return CC_ERROR;

	string host;
	string path;
	if (parse_url(href, host, path) != CC_OK) return CC_ERROR;

	if (host == m_host) return CC_ERROR;

	const size_t content_start = link.find(">", href_end) + 1;
	if (content_start == string::npos) return CC_ERROR;
	const size_t content_end = link.find("</a>", content_start);
	string content = link.substr(content_start, content_end - content_start);
	clean_text(content);

	if (content == "") return CC_ERROR;

	//cout << "External link: " << href << " with text: " << content << endl;

	m_links.push_back(Link(m_host, m_path, host, path, content));

	return CC_OK;
}

int HtmlParser::parse_url(const string &url, string &host, string &path) {
	CURLU *h = curl_url();
	if (!h) return CC_ERROR;

	CURLUcode uc = curl_url_set(h, CURLUPART_URL, url.c_str(), 0);
	if (uc) {
		curl_url_cleanup(h);
		return CC_ERROR;
	}

	char *chost;
	uc = curl_url_get(h, CURLUPART_HOST, &chost, 0);
	if (!uc) {
		host = chost;
		curl_free(chost);
	}

	char *cpath;
	uc = curl_url_get(h, CURLUPART_PATH, &cpath, 0);
	if (!uc) {
		if (strnlen(cpath, HTML_PARSER_CLEANBUF_LEN) < HTML_PARSER_CLEANBUF_LEN) {
			decode_html_entities_utf8(m_clean_buff, cpath);
			path = m_clean_buff;
		} else {
			path = cpath;
		}
		curl_free(cpath);
	}

	char *cquery;
	uc = curl_url_get(h, CURLUPART_QUERY, &cquery, 0);
	if (!uc) {
		if (strnlen(cquery, HTML_PARSER_CLEANBUF_LEN) < HTML_PARSER_CLEANBUF_LEN) {
			decode_html_entities_utf8(m_clean_buff, cquery);
			path += "?" + string(m_clean_buff);
		} else {
			path += "?" + string(cquery);
		}
		curl_free(cquery);
	}

	curl_url_cleanup(h);

	return CC_OK;
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

vector<Link> HtmlParser::links() {
	return m_links;
}

bool HtmlParser::should_insert() {
	return m_should_insert;
}

string HtmlParser::url_tld(const string &url) {

	string response;
	string host;
	vector<string> parts;
	CURLU *h = curl_url();
	if (!h) return "";

	CURLUcode uc = curl_url_set(h, CURLUPART_URL, url.c_str(), 0);
	if (uc) {
		curl_url_cleanup(h);
		return "";
	}

	char *chost;
	uc = curl_url_get(h, CURLUPART_HOST, &chost, 0);
	if (!uc) {
		host = chost;
		boost::split(parts, host, boost::is_any_of("."));
		curl_free(chost);

		if (parts.size()) {
			response = parts.back();
		}
	}

	curl_url_cleanup(h);

	return response;
}

inline string HtmlParser::lower_case(const string &str) {
	string ret = str;
	transform(ret.begin(), ret.end(), ret.begin(), [](unsigned char c){ return tolower(c); });
	return ret;
}

inline pair<size_t, size_t> HtmlParser::find_tag(const string &html, const string &tag_start, const string &tag_end,
	size_t pos) {
	size_t pos_start = html.find(tag_start, pos);
	if (pos_start == string::npos) return pair<size_t, size_t>(string::npos, string::npos);

	const size_t pos_end = html.find(tag_end, pos_start);
	const size_t len = pos_end - pos_start;
	if (pos_end == string::npos) return pair<size_t, size_t>(string::npos, string::npos);
	return pair<size_t, size_t>(pos_start, pos_end + tag_end.size());
}

inline string HtmlParser::get_tag_content(const string &html, const string &tag_start, const string &tag_end) {
	size_t pos_start = html.find(tag_start);
	if (pos_start == string::npos) return "";
	pos_start = html.find(">", pos_start);

	const size_t pos_end = html.find(tag_end, pos_start);
	const size_t len = pos_end - pos_start;
	if (pos_end == string::npos) return "";
	return (string)html.substr(pos_start + 1, len - 1);
}

inline string HtmlParser::get_meta_tag(const string &html) {
	const size_t pos_start = html.find("<meta");
	const size_t pos_end = html.find("description\"", pos_start);
	const size_t pos_end_tag = html.find(">", pos_end);
	if (pos_start == string::npos || pos_end == string::npos || pos_end_tag == string::npos) return "";

	const string s = "content=";
	const size_t content_start = html.rfind(s, pos_end_tag);
	if (content_start == string::npos) return "";

	return (string)html.substr(content_start + s.size(), pos_end_tag - content_start - s.size() - 1);
}

inline void HtmlParser::clean_text(string &str) {
	strip_tags(str);
	if (str.size() >= HTML_PARSER_CLEANBUF_LEN) return;
	decode_html_entities_utf8(m_clean_buff, str.c_str());
	str = m_clean_buff;
	strip_whitespace(str);
	trim(str);
}

inline void HtmlParser::strip_tags(string &html) {
	const int len = html.size();
	bool copy = true;
	bool last_was_space = false;
	int i = 0, j = 0;
	const char *html_s = html.c_str();
	for (; i < len; i++) {
		if (html_s[i] == '<') copy = false;
		if (isspace(html_s[i])) {
			html[j] = ' ';
			if (copy && !last_was_space) j++;
			last_was_space = true;
		} else {
			html[j] = html_s[i];
			if (copy) j++;
			last_was_space = false;
		}
		if (html_s[i] == '>') copy = true;
	}
	html.resize(j);
}

inline void HtmlParser::strip_whitespace(string &html) {
	const int len = html.size();
	bool last_was_space = false;
	int i = 0, j = 0;
	const char *html_s = html.c_str();
	for (; i < len; i++) {
		if (isspace(html_s[i])) {
			html[j] = ' ';
			if (!last_was_space) j++;
			last_was_space = true;
		} else {
			html[j] = html_s[i];
			j++;
			last_was_space = false;
		}
	}
	html.resize(j);
}

inline string HtmlParser::get_text_after_h1(const string &html) {
	const size_t pos_start = html.find("</h1>");

	if (pos_start == string::npos) return "";

	const int len = html.size();
	bool copy = true;
	bool ignore = false;
	bool last_was_space = false;
	int i = pos_start, j = 0;

	auto interval = m_invisible_pos.begin();
	const auto invisible_end = m_invisible_pos.end();
	while (interval != m_invisible_pos.end() && interval->first < pos_start) {
		interval++;
	}

	const char *html_s = html.c_str();

	for (; i < len && j < HTML_PARSER_LONG_TEXT_LEN; i++) {
		if (html_s[i] == '<') {
			if (interval != invisible_end && interval->first == i) {
				// Skip the whole invisible tag.
				i = interval->second;
				interval++;
				continue;
			}
			copy = false;
		}
		if (isspace(html_s[i])) {
			m_long_str_buf[j] = ' ';
			if (copy && !last_was_space) j++;
			last_was_space = true;
		} else {
			m_long_str_buf[j] = html_s[i];
			if (copy) j++;
			last_was_space = false;
		}
		if (!ignore && html_s[i] == '>') copy = true;
	}

	string text(m_long_str_buf, j);

	return text;
}

bool HtmlParser::is_exotic_language_debug(const string &str) const {
	const size_t len = str.size();
	const char *cstr = str.c_str();
	int num_exotic = 0;
	int num_normal = 0;
	int num_seminormal = 0;
	for (int i = 0; i < len;) {
		int multibyte_len = 1;
		int cumsum = 0;
		for (int j = i + 1; (j < len) && IS_MULTIBYTE_CODEPOINT(cstr[j]); j++, multibyte_len++) {
			cumsum += (unsigned char)cstr[j];
		}

		if (multibyte_len > 2) {
			num_exotic++;
		} else if (multibyte_len == 2){
			num_seminormal++;
		} else {
			num_normal++;
		}

		i += multibyte_len;
	}

	int total = (num_seminormal + num_exotic + num_normal);

	cout << str << " exotic: " << num_exotic << " seminormal: " << num_seminormal << " normal: " << num_normal << endl;

	if (num_exotic > 5) return true;
	if (total <= 3) return false;
	if ((float)(num_seminormal + num_exotic) / ((float)total) > 0.5) return true;

	return false;
}

bool HtmlParser::is_exotic_language(const string &str) const {
	const size_t len = str.size();
	const char *cstr = str.c_str();
	int num_exotic = 0;
	int num_normal = 0;
	int num_seminormal = 0;
	for (int i = 0; i < len;) {
		int multibyte_len = 1;
		int cumsum = 0;
		for (int j = i + 1; (j < len) && IS_MULTIBYTE_CODEPOINT(cstr[j]); j++, multibyte_len++) {
			cumsum += (unsigned char)cstr[j];
		}

		if (multibyte_len > 2) {
			num_exotic++;
		} else if (multibyte_len == 2){
			num_seminormal++;
		} else {
			num_normal++;
		}

		i += multibyte_len;
	}

	int total = (num_seminormal + num_exotic + num_normal);

	if (num_exotic > 5) return true;
	if (total <= 3) return false;
	if ((float)(num_seminormal + num_exotic) / ((float)total) > 0.5) return true;

	return false;
}

