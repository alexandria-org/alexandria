/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "config.h"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <span>
#include <cassert>

using namespace std;

template<typename DataRecord>
class FullTextResultSet {

public:

	FullTextResultSet(size_t size);
	~FullTextResultSet();

	size_t size() const { return m_size; }
	size_t max_size() const { return m_max_size; }

	const DataRecord *data_pointer() const { return m_data_pointer; }
	DataRecord *data_pointer() { return m_data_pointer; }
	span<DataRecord> *span_pointer() { return &m_span; }

	size_t total_num_results() const { return m_total_num_results ; };
	void set_total_num_results(size_t total_num_results);

	void resize(size_t n) {
		m_span = span<DataRecord>(m_data_pointer, n);
		m_size = n;
	}

	void prepare_segments(const string &filename, size_t offset, size_t len);
	void read_next_segment();
	bool has_next_segment();
	void close_segments();

private:

	FullTextResultSet(const FullTextResultSet &res) = delete;

	span<DataRecord> m_span;
	DataRecord *m_data_pointer;

	size_t m_size; // The length in first segment.
	const size_t m_max_size; // The maximum number of elements the result set can hold.
	size_t m_total_size; // The lengths of all elements in all segments.
	size_t m_total_num_results; // The total indexed length, only used to display total number of results.
	size_t m_segment_len;
	size_t m_records_read;
	int m_file_descriptor;

};

template<typename DataRecord>
FullTextResultSet<DataRecord>::FullTextResultSet(size_t size)
: m_size(size), m_max_size(size), m_total_num_results(0)
{
	m_file_descriptor = -1;
	m_data_pointer = new DataRecord[size];
	m_span = span<DataRecord>(m_data_pointer, size);
}

template<typename DataRecord>
FullTextResultSet<DataRecord>::~FullTextResultSet() {
	delete []m_data_pointer;
}

template<typename DataRecord>
void FullTextResultSet<DataRecord>::set_total_num_results(size_t total_num_results) {
	m_total_num_results = total_num_results;
}

template<typename DataRecord>
void FullTextResultSet<DataRecord>::prepare_segments(const string &filename, size_t offset, size_t len) {

	assert(m_file_descriptor < 0);

	m_size = len / sizeof(DataRecord);
	m_total_size = m_size;
	if (m_size > Config::ft_max_results_per_section) m_size = Config::ft_max_results_per_section;

	m_file_descriptor = open(filename.c_str(), O_RDONLY);
	posix_fadvise(m_file_descriptor, offset, m_size * sizeof(DataRecord), POSIX_FADV_SEQUENTIAL);
	lseek(m_file_descriptor, offset, SEEK_SET);
	m_records_read = 0;
}

template<typename DataRecord>
void FullTextResultSet<DataRecord>::read_next_segment() {
	if (m_records_read >= m_total_size) {
		resize(0);
		return;
	}
	size_t records_to_read = m_total_size - m_records_read;
	if (records_to_read > Config::ft_max_results_per_section) {
		records_to_read = Config::ft_max_results_per_section;
	}
	::read(m_file_descriptor, (void *)&m_data_pointer[0], (size_t)records_to_read * sizeof(DataRecord));
	m_records_read += records_to_read;
	resize(records_to_read);
}

template<typename DataRecord>
bool FullTextResultSet<DataRecord>::has_next_segment() {
	if (m_file_descriptor < 0) return false;
	return m_total_size > m_records_read;
}

template<typename DataRecord>
void FullTextResultSet<DataRecord>::close_segments() {
	if (m_file_descriptor >= 0) {
		close(m_file_descriptor);
		m_file_descriptor = -1;
	}
}

