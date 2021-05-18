
#pragma once

#include <cstdint>

class FullTextResult {

public:

	FullTextResult(uint64_t key, uint32_t score);

	uint64_t m_value;
	uint32_t m_score;

};