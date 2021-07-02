
#pragma once

#include <iostream>
#include "LinkResult.h"

using namespace std;

class LinkResultSet {

public:

	LinkResultSet();
	~LinkResultSet();

	size_t len() const;
	size_t total_num_results() const;
	void allocate(size_t len);
	uint64_t *link_hash_pointer();
	uint64_t *source_pointer();
	uint64_t *target_pointer();
	uint64_t *source_domain_pointer();
	uint64_t *target_domain_pointer();
	uint32_t *score_pointer();
	void set_total_num_results(size_t total_num_results);

private:

	uint64_t *m_link_hash_pointer;
	uint64_t *m_source_pointer;
	uint64_t *m_target_pointer;
	uint64_t *m_source_domain_pointer;
	uint64_t *m_target_domain_pointer;
	uint32_t *m_score_pointer;
	bool m_has_allocated;
	size_t m_len;
	size_t m_total_num_results;

};
