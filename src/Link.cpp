
#include "Link.h"

Link::Link(const string &host, const string &path, const string &target_host, const string &target_path,
	const string &text) :
	m_host(host),
	m_path(path),
	m_target_host(target_host),
	m_target_path(target_path),
	m_text(text)
{
	
}

Link::~Link() {}

string Link::host() const {
	return m_host;
}

string Link::path() const {
	return m_path;
}

string Link::target_host() const {
	return m_target_host;
}

string Link::target_path() const {
	return m_target_path;
}

string Link::text() const {
	return m_text;
}

