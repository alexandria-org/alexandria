
#include "URL.h"
#include <curl/curl.h>
#include "text/Text.h"

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
	//return m_hasher(m_host + m_path);
	const size_t host_bits = 20;
	const uint64_t hash = m_hasher(m_host + m_path);
	const uint64_t host_part = (host_hash() >> (64 - host_bits)) << (64 - host_bits);
	return (hash >> host_bits) | host_part;
}

uint64_t URL::host_hash() const {
	return m_hasher(m_host);
}

uint64_t URL::link_hash(const URL &target_url, const string &link_text) const {
	const size_t host_bits = 20;
	const uint64_t hash = m_hasher(host() + target_url.str() + link_text);
	const uint64_t host_part = (target_url.host_hash() >> (64 - host_bits)) << (64 - host_bits);
	return (hash >> host_bits) | host_part;
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

map<string, string> URL::query() const {
	map<string, string> ret;
	vector<string> parts;
	boost::split(parts, m_query, boost::is_any_of("&"));
	for (const string &part : parts) {
		vector<string> pair;
		boost::split(pair, part, boost::is_any_of("="));
		if (pair.size() > 1) {
			ret[pair[0]] = unescape(pair[1]);
		}
	}

	return ret;
}

float URL::harmonic(const SubSystem *sub_system) const {

	const auto iter = sub_system->domain_index()->find(m_host_reverse);

	float harmonic;
	if (iter == sub_system->domain_index()->end()) {
		const auto iter2 = sub_system->domain_index()->find(host_reverse_top_domain(m_host));
		if (iter2 == sub_system->domain_index()->end()) {
			harmonic = 0.0f;
		} else {
			const DictionaryRow row = iter2->second;
			harmonic = row.get_float(1) / 2.0; // Half the power for sub domains.
		}
	} else {
		const DictionaryRow row = iter->second;
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

string URL::unescape(const string &str) const {
	const size_t len = str.size();
	const char *cstr = str.c_str();
	char *ret = new char[len + 1];
	size_t j = 0;
	for (size_t i = 0; i < len; i++) {
		if (cstr[i] == '%' && i < len - 2) {
			ret[j++] = (char)stoi(string(&cstr[i + 1], 2), NULL, 16);
			i += 2;
		} else {
			ret[j++] = cstr[i];
		}
	}
	ret[j] = '\0';

	string ret_str(ret);

	delete ret;

	return ret_str;
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
		m_query = cquery;
		m_path += "?" + m_query;
		curl_free(cquery);
	}

	curl_url_cleanup(h);

	m_host_reverse = URL::host_reverse(m_host);

	return CC_OK;
}

inline void URL::remove_www(string &path) {
	size_t pos = path.find("www.");
	if (pos == 0) path.erase(0, 4);
	Text::trim(path);
}
