
#include "HtmlLink.h"

HtmlLink::HtmlLink(const string &host, const string &path, const string &target_host, const string &target_path,
	const string &text) :
	m_host(host),
	m_path(path),
	m_target_host(target_host),
	m_target_path(target_path),
	m_text(text)
{
	
}

HtmlLink::~HtmlLink() {}

string HtmlLink::host() const {
	return m_host;
}

string HtmlLink::path() const {
	return m_path;
}

string HtmlLink::target_host() const {
	return m_target_host;
}

string HtmlLink::target_path() const {
	return m_target_path;
}

string HtmlLink::text() const {
	return m_text;
}

