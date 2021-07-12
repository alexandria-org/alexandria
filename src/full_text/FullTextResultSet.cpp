
#include "FullTextResultSet.h"

FullTextResultSet::FullTextResultSet()
: m_len(0), m_has_allocated(false)
{

}

FullTextResultSet::~FullTextResultSet() {
	if (m_has_allocated) {
		delete m_value_pointer;
		delete m_score_pointer;
	}
}

size_t FullTextResultSet::len() const {
	return m_len;
}

size_t FullTextResultSet::total_num_results() const {
	return m_total_num_results;
}

void FullTextResultSet::allocate(size_t len) {

	if (len > 0) {
		m_value_pointer = new uint64_t[len];
		m_score_pointer = new float[len];
		m_has_allocated = true;
		m_len = len;
	}

}

uint64_t *FullTextResultSet::value_pointer() {
	return m_value_pointer;
}

float *FullTextResultSet::score_pointer() {
	return m_score_pointer;
}

void FullTextResultSet::set_total_num_results(size_t total_num_results) {
	m_total_num_results = total_num_results;
}

