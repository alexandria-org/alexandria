
#pragma once

#include "parser/URL.h"

class Link {

public:
	Link();
	Link(const URL &source_url, const URL &target_url, float source_harmonic, float target_harmonic);
	~Link();

	float url_score() const;
	float domain_score() const;

	const URL &source_url() const { return m_source_url; }
	const URL &target_url() const { return m_target_url; }
	const uint64_t &target_host_hash() const { return m_target_host_hash; }
	const float &source_harmonic() const { return m_source_harmonic; }
	const float &target_harmonic() const { return m_target_harmonic; }

private:
	URL m_source_url;
	URL m_target_url;
	uint64_t m_target_host_hash;
	float m_source_harmonic;
	float m_target_harmonic;
};

