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

#include "entities.h"
#include "html_parser.h"
#include "parser.h"
#include "config.h"
#include "text/text.h"
#include <curl/curl.h>

using namespace std;

namespace parser {

	const vector<string> non_content_tags{"script", "noscript", "style", "embed", "label", "form", "input",
		"iframe", "head", "meta", "link", "object", "aside", "channel", "img"};

	html_parser::html_parser()
	: m_long_text_len(1000), m_long_str_buf_len(m_long_text_len)
	{
		m_long_str_buf = new char[m_long_str_buf_len];
	}

	html_parser::html_parser(size_t long_text_len)
	: m_long_text_len(long_text_len), m_long_str_buf_len(long_text_len)
	{
		m_long_str_buf = new char[m_long_str_buf_len];
	}

	html_parser::~html_parser() {
		delete [] m_long_str_buf;
	}

	void html_parser::parse(const string &html) {
		parse(html, "");
	}

	void html_parser::parse(const string &html, const string &url) {

		m_should_insert = false;
		m_should_insert = false;

		parse_url(url, m_host, m_path, "");

		m_title.clear();
		m_h1.clear();
		m_meta.clear();
		m_text.clear();
		m_invisible_pos.clear();
		m_links.clear();
		m_internal_links.clear();

		parse_encoding(html);
		if (m_encoding == ENC_UNKNOWN) {
			m_should_insert = false;
			return;
		}

		find_scripts(html);
		find_styles(html);
		sort_invisible();
		find_links(html, url);

		m_title = get_tag_content(html, "<title", "</title>");
		m_h1 = get_tag_content(html, "<h1", "</h1>");
		m_meta = get_meta_tag(html);
		m_text = get_text_content(html);

		text::trim_punct(m_meta);

		if (m_encoding == ENC_ISO_8859_1) {
			iso_to_utf8(m_title);
			iso_to_utf8(m_h1);
			iso_to_utf8(m_meta);
			iso_to_utf8(m_text);
		}

		clean_text(m_title);
		if (m_title.size() == 0 || is_exotic_language(m_title) || m_title.size() > HTML_PARSER_MAX_TITLE_LEN) return;
		m_should_insert = true;

		clean_text(m_h1);
		clean_text(m_meta);
		clean_text(m_text);

		if (m_h1.size() > HTML_PARSER_MAX_H1_LEN) {
			m_should_insert = false;
			return;
		}
	}

	void html_parser::find_scripts(const string &html) {
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

	void html_parser::find_styles(const string &html) {
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

	void html_parser::find_links(const string &html, const string &base_url) {
		size_t pos = 0;
		pair<size_t, size_t> tag(0, 0);
		while (pos != string::npos) {
			tag = find_tag(html, "<a ", "</a>", tag.second);
			if (tag.second == string::npos) {
				break;
			}

			parse_link(html.substr(tag.first, tag.second - tag.first), base_url);
		}
	}

	int html_parser::parse_link(const string &link, const string &base_url) {
		const string href_key = "href=\"";
		const size_t key_len = href_key.size();
		const size_t href_start = link.find(href_key);
		if (href_start == string::npos) return ::parser::ERROR;
		const size_t href_end = link.find("\"", href_start + key_len);
		if (href_end == string::npos) return ::parser::ERROR;
		string href = link.substr(href_start + key_len, href_end - href_start - key_len);

		const string rel_key = "rel=\"";
		const size_t rel_key_len = rel_key.size();
		const size_t rel_start = link.find(rel_key);
		bool nofollow = false;
		if (rel_start != string::npos) {
			// "rel=" present in string
			const size_t rel_end = link.find("\"", rel_start + key_len);
			const string rel = link.substr(rel_start + rel_key_len, rel_end - rel_start - rel_key_len);
			if (rel.find("nofollow") != string::npos) nofollow = true;
		}

		string host;
		string path;
		if (parse_url(href, host, path, base_url) != ::parser::OK) return ::parser::ERROR;

		if (host == m_host) {
			// Ignore internal links for now.
			if (!nofollow) {
				m_internal_links.emplace_back(std::make_pair(URL(m_host, m_path).hash(), URL(host, path).hash()));
			}
			return ::parser::OK;
		}

		const size_t content_start = link.find(">", href_end) + 1;
		if (content_start == string::npos) return ::parser::ERROR;
		const size_t content_end = link.find("</a>", content_start);
		string content = link.substr(content_start, content_end - content_start);

		if (m_encoding == ENC_ISO_8859_1) {
			iso_to_utf8(content);
		}
		clean_text(content);

		if (content == "") return ::parser::ERROR;

		m_links.push_back(html_link(m_host, m_path, host, path, nofollow, content));

		return ::parser::OK;
	}

	int html_parser::parse_url(const string &url, string &host, string &path, const string &base_url) {
		CURLU *h = curl_url();
		if (!h) return ::parser::ERROR;

		if (base_url.size()) {
			curl_url_set(h, CURLUPART_URL, base_url.c_str(), 0);
		}

		CURLUcode uc = curl_url_set(h, CURLUPART_URL, url.c_str(), 0);
		if (uc) {
			curl_url_cleanup(h);
			return ::parser::ERROR;
		}

		char *chost;
		uc = curl_url_get(h, CURLUPART_HOST, &chost, 0);
		if (!uc) {
			host = chost;
			remove_www(host);
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

		return ::parser::OK;
	}

	void html_parser::remove_www(string &path) {
		size_t pos = path.find("www.");
		if (pos == 0) path.erase(0, 4);
		text::trim(path);
	}

	void html_parser::parse_encoding(const string &html) {
		m_encoding = ENC_UTF_8;
		const size_t pos_start = html.find("charset=");
		if (pos_start == string::npos || pos_start > 1024) return;

		string encoding = html.substr(pos_start, 40);
		encoding = text::lower_case(encoding);

		const size_t utf8_start = encoding.find("utf-8");
		const size_t iso88591_start = encoding.find("iso-8859-1");
		if (utf8_start != string::npos) m_encoding = ENC_UTF_8;
		else if (iso88591_start != string::npos) m_encoding = ENC_ISO_8859_1;
		else m_encoding = ENC_UNKNOWN;
	}

	void html_parser::iso_to_utf8(string &str) {
		string str_out;
		for (std::string::iterator it = str.begin(); it != str.end(); ++it)
		{
			uint8_t ch = *it;
			if (ch < 0x80) {
				str_out.push_back(ch);
			}
			else {
				str_out.push_back(0xc0 | ch >> 6);
				str_out.push_back(0x80 | (ch & 0x3f));
			}
		}
		str = str_out;
	}

	string html_parser::title() const {
		return m_title;
	} 

	string html_parser::meta() const {
		return m_meta;
	}

	string html_parser::h1() const {
		return m_h1;
	}

	string html_parser::text() const {
		return m_text;
	}

	vector<html_link> html_parser::links() const {
		return m_links;
	}

	vector<std::pair<uint64_t, uint64_t>> html_parser::internal_links() const {
		return m_internal_links;
	}

	bool html_parser::should_insert() const {
		return m_should_insert;
	}

	string html_parser::url_tld(const string &url) {

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

	inline pair<size_t, size_t> html_parser::find_tag(const string &html, const string &tag_start, const string &tag_end,
		size_t pos) {
		size_t pos_start = html.find(tag_start, pos);
		if (pos_start == string::npos) return pair<size_t, size_t>(string::npos, string::npos);

		const size_t pos_end = html.find(tag_end, pos_start);
		if (pos_end == string::npos) return pair<size_t, size_t>(string::npos, string::npos);
		return pair<size_t, size_t>(pos_start, pos_end + tag_end.size());
	}

	string html_parser::get_tag_content(const string &html, const string &tag_start, const string &tag_end) {
		size_t pos_start = html.find(tag_start);
		if (pos_start == string::npos || is_invisible(pos_start)) return "";
		pos_start = html.find(">", pos_start);

		const size_t pos_end = html.find(tag_end, pos_start);
		const size_t len = pos_end - pos_start;
		if (pos_end == string::npos) return "";
		return (string)html.substr(pos_start + 1, len - 1);
	}

	string html_parser::get_meta_tag(const string &html) {
		size_t pos_start = 0;
		while ((pos_start = html.find("<meta", pos_start + 1)) != string::npos)  {
			const size_t pos_end = html.find(">", pos_start);
			const size_t pos_description = html.find("description\"", pos_start);
			if (pos_description < pos_end) {
				const size_t pos_end_tag = html.find(">", pos_description);
				const size_t pos_start_tag = html.rfind("<", pos_description);

				const string s = "content=";
				const size_t content_start = html.find(s, pos_start_tag);
				if (content_start != string::npos && content_start <= pos_end_tag) {
					return (string)html.substr(content_start + s.size(), pos_end_tag - content_start - s.size() - 1);
				}
			}
		}
		return "";
	}

	void html_parser::clean_text(string &str) {
		strip_tags(str);
		if (str.size() >= HTML_PARSER_CLEANBUF_LEN) return;
		decode_html_entities_utf8(m_clean_buff, str.c_str());
		str = m_clean_buff;
		strip_whitespace(str);
		text::trim(str);
	}

	void html_parser::strip_tags(string &html) {
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

	void html_parser::strip_whitespace(string &html) {
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

	/*
	 * This function returns the text content of the html by first trying to fetch content after the first <h1>...</h1> tag. If no h1 tag is present
	 * it tries to fetch content from the start of the <body>
	 * */
	string html_parser::get_text_content(const string &html) {
		size_t pos_start = html.find("</h1>");

		// Start from body if no h1 is present
		if (pos_start == string::npos || is_invisible(pos_start)) {
			pos_start = html.find("<body");
		}
		if (pos_start == string::npos || is_invisible(pos_start)) {
			return "";
		}

		const size_t len = html.size();
		bool copy = true;
		bool ignore = false;
		bool last_was_space = false;
		size_t i = pos_start, j = 0;

		auto interval = m_invisible_pos.begin();
		const auto invisible_end = m_invisible_pos.end();
		while (interval != m_invisible_pos.end() && interval->first < pos_start) {
			interval++;
		}

		const char *html_s = html.c_str();

		for (; i < len && j < m_long_str_buf_len; i++) {
			if (html_s[i] == '<') {
				if (interval != invisible_end && interval->first == i) {
					// Skip the whole invisible tag.
					i = interval->second - 1;
					interval++;
					continue;
				}
				// Insert a space, because we don't want to concatenate words.
				m_long_str_buf[j] = ' ';
				if (copy && !last_was_space) j++;
				last_was_space = true;

				copy = false;
			}
			if (isspace(html_s[i])) {
				if (j < m_long_str_buf_len) m_long_str_buf[j] = ' ';
				if (copy && !last_was_space) j++;
				last_was_space = true;
			} else {
				if (j < m_long_str_buf_len) m_long_str_buf[j] = html_s[i];
				if (copy) j++;
				last_was_space = false;
			}
			if (!ignore && html_s[i] == '>') copy = true;
		}

		string text(m_long_str_buf, j);

		return text;
	}

	bool html_parser::is_exotic_language_debug(const string &str) const {
		const size_t len = str.size();
		const char *cstr = str.c_str();
		int num_exotic = 0;
		int num_normal = 0;
		int num_seminormal = 0;
		for (size_t i = 0; i < len;) {
			int multibyte_len = 1;
			int cumsum = 0;
			for (size_t j = i + 1; (j < len) && IS_MULTIBYTE_CODEPOINT(cstr[j]); j++, multibyte_len++) {
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

	bool html_parser::is_exotic_language(const string &str) const {
		const size_t len = str.size();
		const char *cstr = str.c_str();
		int num_exotic = 0;
		int num_normal = 0;
		int num_seminormal = 0;
		for (size_t i = 0; i < len;) {
			int multibyte_len = 1;
			int cumsum = 0;
			for (size_t j = i + 1; (j < len) && IS_MULTIBYTE_CODEPOINT(cstr[j]); j++, multibyte_len++) {
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

	void html_parser::sort_invisible() {
		sort(m_invisible_pos.begin(), m_invisible_pos.end(), [](const pair<int, int>& lhs, const pair<int, int>& rhs) {
			return lhs.first < rhs.first;
		});
	}

	inline bool html_parser::is_invisible(size_t pos) {
		for (const auto &interval : m_invisible_pos) {
			if (interval.first <= pos && pos < interval.second) return true;
		}
		return false;
	}

}
