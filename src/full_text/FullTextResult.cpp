
#include "FullTextResult.h"
#include "system/Logger.h"

FullTextResult::FullTextResult()
: m_value(0), m_score(0)
{
}

FullTextResult::FullTextResult(uint64_t value, uint32_t score)
: m_value(value), m_score(score)
{
}

/*FullTextResult::FullTextResult(const FullTextResult &res)
: m_value(res.m_value), m_score(res.m_score)
{
	m_value = res.m_value;
	m_score = res.m_score;
}

//FullTextResult& operator=(const FullTextResult& other) 
*/

bool operator==(const FullTextResult &a, const FullTextResult &b) {
	return a.m_value == b.m_value;
}
