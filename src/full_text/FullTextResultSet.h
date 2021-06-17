
#pragma once

#include <iostream>
#include "FullTextResult.h"

using namespace std;

class FullTextResultSet {

public:

	FullTextResultSet();
	~FullTextResultSet();

	size_t len() const;
	void allocate(size_t len);
	uint64_t *value_pointer();
	uint32_t *score_pointer();

private:

	uint64_t *m_value_pointer;
	uint32_t *m_score_pointer;
	bool m_has_allocated;
	size_t m_len;

};
