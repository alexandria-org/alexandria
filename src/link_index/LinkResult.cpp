
#include "LinkResult.h"
#include "system/Logger.h"

LinkResult::LinkResult()
: m_source(0), m_target(0), m_source_domain(0), m_target_domain(0), m_score(0)
{
}

LinkResult::LinkResult(uint64_t link_hash, uint64_t source, uint64_t target, uint64_t source_domain,
	uint64_t target_domain, float score)
: m_link_hash(link_hash), m_source(source), m_target(target), m_source_domain(source_domain),
	m_target_domain(target_domain), m_score(score)
{
}

bool operator==(const LinkResult &a, const LinkResult &b) {
	return a.m_link_hash == b.m_link_hash;
}
