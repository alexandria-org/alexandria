
#include "ApiLink.h"

ApiLink::ApiLink(const string &host, const string &path, const string &target_host, const string &target_path,
	const string &text) :
	m_host(host),
	m_path(path),
	m_target_host(target_host),
	m_target_path(target_path),
	m_text(text)
{
	
}

ApiLink::~ApiLink() {}

string ApiLink::host() const {
	return m_host;
}

string ApiLink::path() const {
	return m_path;
}

string ApiLink::target_host() const {
	return m_target_host;
}

string ApiLink::target_path() const {
	return m_target_path;
}

string ApiLink::text() const {
	return m_text;
}

