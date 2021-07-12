
#pragma once

#include <iostream>
#include "FullTextResult.h"

using namespace std;

class FullTextResultSet {

public:

	FullTextResultSet();
	~FullTextResultSet();

	size_t len() const;
	size_t total_num_results() const;
	void allocate(size_t len);
	uint64_t *value_pointer();
	float *score_pointer();
	void set_total_num_results(size_t total_num_results);

private:

	uint64_t *m_value_pointer;
	float *m_score_pointer;
	bool m_has_allocated;
	size_t m_len;
	size_t m_total_num_results;

};
