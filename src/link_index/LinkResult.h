
#pragma once

#include <cstdint>

class LinkResult {

public:

	LinkResult();
	LinkResult(uint64_t link_hash, uint64_t source, uint64_t target, uint64_t source_domain, uint64_t target_domain,
		float score);

	uint64_t m_link_hash;
	uint64_t m_source;
	uint64_t m_target;
	uint64_t m_source_domain;
	uint64_t m_target_domain;
	float m_score;

	friend bool operator==(const LinkResult &a, const LinkResult &b);

};
