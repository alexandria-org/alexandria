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
	span<DataRecord> *span_pointer() { return &m_span; }

	size_t total_num_results() const { return m_total_num_results ; };
	void set_total_num_results(size_t total_num_results);

	void resize(size_t n) {
		m_span = span<DataRecord>(m_data_pointer, n);
		m_size = n;
	}

private:

	FullTextResultSet(const FullTextResultSet &res) = delete;

	span<DataRecord> m_span;
	DataRecord *m_data_pointer;

	size_t m_size;
	size_t m_total_num_results;

};

template<typename DataRecord>
FullTextResultSet<DataRecord>::FullTextResultSet(size_t size)
: m_size(size), m_total_num_results(0)
{
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

