
#pragma once

#include <iostream>
#include <span>

using namespace std;

template<typename DataRecord>
class FullTextResultSet {

public:

	FullTextResultSet(size_t size);
	~FullTextResultSet();

	size_t size() const { return m_size; }

	const DataRecord *data_pointer() const { return m_data_pointer; }
	DataRecord *data_pointer() { return m_data_pointer; }
	span<DataRecord> *span_pointer() { return m_span; }

	size_t total_num_results() const { return m_total_num_results ; };
	void set_total_num_results(size_t total_num_results);

	void resize(size_t n) {
		delete m_span;
		m_span = new span<DataRecord>(m_data_pointer, n);
		m_size = n;
	}

private:

	FullTextResultSet(const FullTextResultSet &res) = delete;

	span<DataRecord> *m_span;
	DataRecord *m_data_pointer;

	size_t m_size;
	size_t m_total_num_results;

};

template<typename DataRecord>
FullTextResultSet<DataRecord>::FullTextResultSet(size_t size)
: m_size(size), m_total_num_results(0)
{

	m_data_pointer = new DataRecord[size];
	m_span = new span<DataRecord>(m_data_pointer, size);

}

template<typename DataRecord>
FullTextResultSet<DataRecord>::~FullTextResultSet() {
	delete m_span;
	delete m_data_pointer;
}

template<typename DataRecord>
void FullTextResultSet<DataRecord>::set_total_num_results(size_t total_num_results) {
	m_total_num_results = total_num_results;
}

