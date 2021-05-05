
#include "URL.h"
#include <curl/curl.h>

URL::URL() {
	m_status = CC_OK;
}

URL::URL(const string &url) :
	m_url_string(url)
{
	m_status = parse();
}

URL::~URL() {

}

void URL::set_url_string(const string &url) {
	m_url_string = url;
	m_status = parse();
}

string URL::host() const {
	return m_host;
}

string URL::host_reverse() const {
	return m_host_reverse;
}

string URL::path() const {
	return m_path;
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
	if (!h) return CC_ERROR;

	CURLUcode uc = curl_url_set(h, CURLUPART_URL, m_url_string.c_str(), 0);
	if (uc) {
		curl_url_cleanup(h);
		return CC_ERROR;
	}

	char *chost;
	uc = curl_url_get(h, CURLUPART_HOST, &chost, 0);
	if (!uc) {
		m_host = chost;
		remove_www(m_host);
		curl_free(chost);
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
		m_path += "?" + string(cquery);
		curl_free(cquery);
	}

	curl_url_cleanup(h);

	vector<string> parts;
	boost::split(parts, m_host, boost::is_any_of("."));
	reverse(parts.begin(), parts.end());
	m_host_reverse = boost::algorithm::join(parts, ".");

	return CC_OK;
}

inline void URL::remove_www(string &path) {
	size_t pos = path.find("www.");
	if (pos == 0) path.erase(0, 4);
	trim(path);
}