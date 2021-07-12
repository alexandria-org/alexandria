
#include "FullTextResult.h"
#include "system/Logger.h"

FullTextResult::FullTextResult()
: m_value(0), m_score(0)
{
}

FullTextResult::FullTextResult(uint64_t value, float score)
: m_value(value), m_score(score)
{
}

FullTextResult::FullTextResult(const FullTextResult &res)
: m_value(res.m_value), m_score(res.m_score)
{
	//LogInfo("Copy on FullTextResult called");
}

FullTextResult& FullTextResult::operator=(const FullTextResult& other) {
	m_value = other.m_value;
	m_score = other.m_score;
	//LogInfo("Assignemt on FullTextResult called");

	return *this;
}

bool operator==(const FullTextResult &a, const FullTextResult &b) {
	return a.m_value == b.m_value;
}

bool operator==(const FullTextResult &a, uint64_t b) {
	return a.m_value == b;
}

bool operator==(uint64_t b, const FullTextResult &a) {
	return a.m_value == b;
}

bool operator<(const FullTextResult &a, const FullTextResult &b) {
	return a.m_value < b.m_value;
}

bool operator<(const FullTextResult &a, uint64_t b) {
	return a.m_value < b;
}

bool operator<(uint64_t b, const FullTextResult &a) {
	return a.m_value < b;
}

bool operator>(const FullTextResult &a, const FullTextResult &b) {
	return a.m_value > b.m_value;
}

bool operator>(const FullTextResult &a, uint64_t b) {
	return a.m_value > b;
}

bool operator>(uint64_t b, const FullTextResult &a) {
	return a.m_value > b;
}
