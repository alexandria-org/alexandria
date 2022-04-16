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

#include "URL.h"
#include "algorithm/hash.h"
#include "parser/parser.h"
#include <curl/curl.h>
#include "text/text.h"

using namespace std;

URL::URL() {
	m_status = ::parser::OK;
}

URL::URL(const URL &url) :
	m_url_string(url.m_url_string),
	m_host(url.m_host),
	m_host_reverse(url.m_host_reverse),
	m_scheme(url.m_scheme),
	m_path(url.m_path),
	m_query(url.m_query),
	m_status(url.m_status),
	m_has_www(url.m_has_www)
{
}

URL::URL(const string &url) :
	m_url_string(url)
{
	m_status = parse();
}

URL::URL(const string &host, const string &path) :
	m_url_string("http://" + host + path), m_host(host), m_path(path)
{
	m_host_reverse = URL::host_reverse(m_host);
	m_status = ::parser::OK;
}

URL::~URL() {

}

void URL::set_url_string(const string &url) {
	m_url_string = url;
	m_status = parse();
}

string URL::str() const {
	return m_url_string;
}

string URL::key() const {
	/*
	 * We should probably change this to:
	 * return m_host + path_with_query();
	 * but we need to do it later..
	 */
	return m_host + m_path + m_query;
}

uint64_t URL::hash() const {
	return ::algorithm::hash(m_host + m_path + m_query);
}

uint64_t URL::host_hash() const {
	return ::algorithm::hash(m_host);
}

uint64_t URL::link_hash(const URL &target_url, const string &link_text) const {
	return ::algorithm::hash(host_top_domain() + target_url.str() + link_text);
}

uint64_t URL::domain_link_hash(const URL &target_url, const string &link_text) const {
	return ::algorithm::hash(host_top_domain() + target_url.host() + link_text);
}

bool URL::canonically_different(const URL &url) const {
	return key() != url.key();
}

bool URL::has_https() const {
	return m_scheme == "https";
}

bool URL::has_www() const {
	return m_has_www;
}

string URL::host() const {
	return m_host;
}

string URL::host_top_domain() const {
	/*
	 * This algorithm is OK since we only run on these tlds:
	 * {"se", "com", "nu", "net", "org", "gov", "edu", "info"}
	 * */
	vector<string> parts;
	boost::split(parts, m_host, boost::is_any_of("."));
	if (parts.size() > 2) {
		parts = {parts[parts.size() - 2], parts[parts.size() - 1]};
	}
	return boost::algorithm::join(parts, ".");
}

string URL::scheme() const {
	return m_scheme;
}

string URL::host_reverse() const {
	return m_host_reverse;
}

string URL::path() const {
	return m_path;
}

string URL::path_with_query() const {
	if (m_query.size() > 0) {
		return m_path + "?" + m_query;
	} else {
		return m_path;
	}
}

map<string, string> URL::query() const {
	map<string, string> ret;
	vector<string> parts;
	boost::split(parts, m_query, boost::is_any_of("&"));
	for (const string &part : parts) {
		vector<string> pair;
		boost::split(pair, part, boost::is_any_of("="));
		if (pair.size() > 1) {
			ret[pair[0]] = parser::urldecode(pair[1]);
		}
	}

	return ret;
}

float URL::harmonic(const common::sub_system *subsys) const {

	const auto iter = subsys->domain_index()->find(m_host_reverse);

	float harmonic;
	if (iter == subsys->domain_index()->end()) {
		const auto iter2 = subsys->domain_index()->find(host_reverse_top_domain(m_host));
		if (iter2 == subsys->domain_index()->end()) {
			harmonic = 0.0f;
		} else {
			const common::dictionary_row row = iter2->second;
			harmonic = row.get_float(1) / 2.0; // Half the power for sub domains.
		}
	} else {
		const common::dictionary_row row = iter->second;
		harmonic = row.get_float(1);
	}

	return harmonic;
}

string URL::host_reverse(const string &host) {
	vector<string> parts;
	boost::split(parts, host, boost::is_any_of("."));
	reverse(parts.begin(), parts.end());
	return boost::algorithm::join(parts, ".");
}

string URL::host_reverse_top_domain(const string &host) {
	/*
	 * This algorithm is OK since we only run on these tlds:
	 * {"se", "com", "nu", "net", "org", "gov", "edu", "info"}
	 * */
	vector<string> parts;
	boost::split(parts, host, boost::is_any_of("."));
	if (parts.size() > 2) {
		parts = {parts[parts.size() - 2], parts[parts.size() - 1]};
	}
	reverse(parts.begin(), parts.end());
	return boost::algorithm::join(parts, ".");
}

string URL::domain_without_tld() const {
	vector<string> parts;
	boost::split(parts, m_host, boost::is_any_of("."));
	if (parts.size() > 1) {
		return parts[parts.size() - 2];
	}
	return "";
}

uint32_t URL::size() const {
	return str().size();
}

void URL::set_scheme(const string &scheme) {
	m_scheme = scheme;
	rebuild_url_str();
}

void URL::set_www(bool has_www) {
	m_has_www = has_www;
	rebuild_url_str();
}

istream &operator >>(istream &ss, URL &url) {
	ss >> (url.m_url_string);
	url.m_status = url.parse();

	return ss;
}

ostream &operator <<(ostream& os, const URL& url) {
	os << url.m_url_string;
	return os;
}

int URL::parse() {
	CURLU *h = curl_url();
	if (!h) return ::parser::ERROR;

	CURLUcode uc = curl_url_set(h, CURLUPART_URL, m_url_string.c_str(), 0);
	if (uc) {
		curl_url_cleanup(h);
		return ::parser::ERROR;
	}

	char *chost;
	uc = curl_url_get(h, CURLUPART_HOST, &chost, 0);
	if (!uc) {
		m_host = chost;
		remove_www(m_host);
		curl_free(chost);
	}

	char *scheme;
	uc = curl_url_get(h, CURLUPART_SCHEME, &scheme, 0);
	if (!uc) {
		m_scheme = scheme;
		curl_free(scheme);
	}

	char *cpath;
	uc = curl_url_get(h, CURLUPART_PATH, &cpath, 0);
	if (!uc) {
		m_path = cpath;
		curl_free(cpath);
	}

	char *cquery;
	uc = curl_url_get(h, CURLUPART_QUERY, &cquery, 0);
	if (!uc) {
		m_query = cquery;
		curl_free(cquery);
	}

	curl_url_cleanup(h);

	m_host_reverse = URL::host_reverse(m_host);

	return ::parser::OK;
}

void URL::rebuild_url_str() {
	m_url_string = m_scheme + "://" + (m_has_www ? "www." : "") + m_host + path_with_query();
}

inline void URL::remove_www(string &path) {
	size_t pos = path.find("www.");
	if (pos == 0) {
		m_has_www = true;
		path.erase(0, 4);
	} else {
		m_has_www = false;
	}
	text::trim(path);
}
