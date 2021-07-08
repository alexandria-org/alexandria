
#pragma once

#include <cstdint>

class FullTextResult {

public:

	FullTextResult();
	FullTextResult(uint64_t key, uint32_t score);
	FullTextResult(const FullTextResult &res);

	uint64_t m_value;
	uint32_t m_score;

	FullTextResult& operator=(const FullTextResult& other);

	friend bool operator==(const FullTextResult &a, const FullTextResult &b);
	friend bool operator==(const FullTextResult &a, uint64_t b);
	friend bool operator==(uint64_t b, const FullTextResult &a);

	friend bool operator<(const FullTextResult &a, const FullTextResult &b);
	friend bool operator<(const FullTextResult &a, uint64_t b);
	friend bool operator<(uint64_t b, const FullTextResult &a);

	friend bool operator>(const FullTextResult &a, const FullTextResult &b);
	friend bool operator>(const FullTextResult &a, uint64_t b);
	friend bool operator>(uint64_t b, const FullTextResult &a);

};
