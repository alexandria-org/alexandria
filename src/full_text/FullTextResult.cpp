
#include "FullTextResult.h"

FullTextResult::FullTextResult()
: m_value(0), m_score(0)
{
}

FullTextResult::FullTextResult(uint64_t value, uint32_t score)
: m_value(value), m_score(score)
{
}