
#include "LinkResultSet.h"

LinkResultSet::LinkResultSet()
: m_len(0), m_has_allocated(false)
{

}

LinkResultSet::~LinkResultSet() {
	if (m_has_allocated) {
		delete m_link_hash_pointer;
		delete m_source_pointer;
		delete m_target_pointer;
		delete m_source_domain_pointer;
		delete m_target_domain_pointer;
		delete m_score_pointer;
	}
}

size_t LinkResultSet::len() const {
	return m_len;
}

size_t LinkResultSet::total_num_results() const {
	return m_total_num_results;
}

void LinkResultSet::allocate(size_t len) {

	if (len > 0) {
		m_link_hash_pointer = new uint64_t[len];
		m_source_pointer = new uint64_t[len];
		m_target_pointer = new uint64_t[len];
		m_source_domain_pointer = new uint64_t[len];
		m_target_domain_pointer = new uint64_t[len];
		m_score_pointer = new float[len];
		m_has_allocated = true;
		m_len = len;
	}

}

uint64_t *LinkResultSet::link_hash_pointer() {
	return m_link_hash_pointer;
}

uint64_t *LinkResultSet::source_pointer() {
	return m_source_pointer;
}

uint64_t *LinkResultSet::target_pointer() {
	return m_target_pointer;
}

uint64_t *LinkResultSet::source_domain_pointer() {
	return m_source_domain_pointer;
}

uint64_t *LinkResultSet::target_domain_pointer() {
	return m_target_domain_pointer;
}

float *LinkResultSet::score_pointer() {
	return m_score_pointer;
}

void LinkResultSet::set_total_num_results(size_t total_num_results) {
	m_total_num_results = total_num_results;
}

