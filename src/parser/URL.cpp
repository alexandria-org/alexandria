
#include "URL.h"
#include <curl/curl.h>

string URL::host_reverse(const string &host) {
	vector<string> parts;
	boost::split(parts, host, boost::is_any_of("."));
	reverse(parts.begin(), parts.end());
	return boost::algorithm::join(parts, ".");
}

URL::URL() {
	m_status = CC_OK;
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
	m_status = CC_OK;
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

uint64_t URL::hash() const {
	return m_hasher(m_host + m_path);
}

uint64_t URL::host_hash() const {
	return m_hasher(m_host);
}

uint64_t URL::link_hash(const URL &target_url) const {
	return m_hasher(str() + " to " + target_url.str());
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

int URL::harmonic(const SubSystem *sub_system) const {

	const auto iter = sub_system->domain_index()->find(host_reverse());

	int harmonic;
	if (iter == sub_system->domain_index()->end()) {
		harmonic = 0;
	} else {
		const DictionaryRow row = iter->second;
		harmonic = row.get_int(1);
	}

	return harmonic;
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

	m_host_reverse = URL::host_reverse(m_host);

	return CC_OK;
}

inline void URL::remove_www(string &path) {
	size_t pos = path.find("www.");
	if (pos == 0) path.erase(0, 4);
	trim(path);
}
