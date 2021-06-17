
#include "FullTextResultSet.h"

FullTextResultSet::FullTextResultSet()
: m_len(0)
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

void FullTextResultSet::allocate(size_t len) {

	m_value_pointer = new uint64_t[len];
	m_score_pointer = new uint32_t[len];
	m_has_allocated = true;
	m_len = len;

}

uint64_t *FullTextResultSet::value_pointer() {
	return m_value_pointer;
}

uint32_t *FullTextResultSet::score_pointer() {
	return m_score_pointer;
}

