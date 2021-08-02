
#pragma once

#include <iostream>
#include "FullTextResult.h"

using namespace std;

template<typename DataRecord>
class FullTextResultSet {

public:

	FullTextResultSet();
	~FullTextResultSet();

	size_t len() const { return m_len; }
	size_t total_num_results() const { return m_total_num_results ; };
	void allocate(size_t len);

	uint64_t *value_pointer() { return m_value_pointer; }
	const uint64_t *value_pointer() const { return m_value_pointer; }

	float *score_pointer() { return m_score_pointer; }
	const float *score_pointer() const { return m_score_pointer; }

	DataRecord *record_pointer() { return m_record_pointer; }
	const DataRecord *record_pointer() const { return m_record_pointer; }

	void set_total_num_results(size_t total_num_results);

private:

	FullTextResultSet(const FullTextResultSet &res) = delete;

	uint64_t *m_value_pointer;
	float *m_score_pointer;
	DataRecord *m_record_pointer;

	bool m_has_allocated;
	size_t m_len;
	size_t m_total_num_results;

};

template<typename DataRecord>
FullTextResultSet<DataRecord>::FullTextResultSet()
: m_len(0), m_has_allocated(false), m_total_num_results(0)
{

}

template<typename DataRecord>
FullTextResultSet<DataRecord>::~FullTextResultSet() {
	if (m_has_allocated) {
		delete m_value_pointer;
		delete m_score_pointer;
		delete m_record_pointer;
	}
}

template<typename DataRecord>
void FullTextResultSet<DataRecord>::allocate(size_t len) {

	if (len > 0) {
		m_value_pointer = new uint64_t[len];
		m_score_pointer = new float[len];
		m_record_pointer = new DataRecord[len];
		m_has_allocated = true;
		m_len = len;
	}

}

template<typename DataRecord>
void FullTextResultSet<DataRecord>::set_total_num_results(size_t total_num_results) {
	m_total_num_results = total_num_results;
}

