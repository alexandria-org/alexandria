
#pragma once

#include "parser/URL.h"

class Link {

public:
	Link();
	Link(const URL &source_url, const URL &target_url, int source_harmonic, int target_harmonic);
	~Link();

	uint32_t url_score() const;
	uint32_t domain_score() const;

	const URL &source_url() const { return m_source_url; }
	const URL &target_url() const { return m_target_url; }
	const uint64_t &target_host_hash() const { return m_target_host_hash; }
	const int &source_harmonic() const { return m_source_harmonic; }
	const int &target_harmonic() const { return m_target_harmonic; }

private:
	URL m_source_url;
	URL m_target_url;
	uint64_t m_target_host_hash;
	int m_source_harmonic;
	int m_target_harmonic;
};

