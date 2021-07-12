
#include "Link.h"

Link::Link() {

}

Link::Link(const URL &source_url, const URL &target_url, int source_harmonic, int target_harmonic)
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

uint32_t Link::url_score() const {
	if (m_source_harmonic <= m_target_harmonic) return 1000;
	if (m_source_harmonic - m_target_harmonic < 10000000) (m_source_harmonic - m_target_harmonic) / 10;
	return 1000000;
}

uint32_t Link::domain_score() const {
	if (m_source_harmonic <= m_target_harmonic) return 1000;
	if (m_source_harmonic - m_target_harmonic < 10000000) (m_source_harmonic - m_target_harmonic) / 10;
	return 1000000;
}

