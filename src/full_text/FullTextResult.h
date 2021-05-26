
#pragma once

#include <cstdint>

class FullTextResult {

public:

	FullTextResult();
	FullTextResult(uint64_t key, uint32_t score);

	uint64_t m_value;
	uint32_t m_score;

	friend bool operator==(const FullTextResult &a, const FullTextResult &b);

};
