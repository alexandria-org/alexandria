
#include "Link.h"

Link::Link() {

}

Link::Link(const URL &source_url, const URL &target_url, float source_harmonic, float target_harmonic)
:
	m_source_url(source_url),
	m_target_url(target_url),
	m_target_host_hash(target_url.host_hash()),
	m_source_harmonic(source_harmonic),
	m_target_harmonic(target_harmonic)
{
}

Link::~Link() {

}

float Link::url_score() const {
	return max(m_source_harmonic - m_target_harmonic, m_source_harmonic / 100.0f) * 10.0;
}

float Link::domain_score() const {
	return max(m_source_harmonic - m_target_harmonic, m_source_harmonic / 100.0f);
}

