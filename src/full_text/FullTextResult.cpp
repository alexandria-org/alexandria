
#include "FullTextResult.h"

FullTextResult::FullTextResult()
: m_value(0), m_score(0)
{
}

FullTextResult::FullTextResult(uint64_t value, uint32_t score)
: m_value(value), m_score(score)
{
}

bool operator==(const FullTextResult &a, const FullTextResult &b) {
	return a.m_value == b.m_value;
}