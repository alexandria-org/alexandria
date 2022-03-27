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

namespace full_text {

	template<typename data_record>
	class full_text_result_set {

		public:

			full_text_result_set(size_t size);
			~full_text_result_set();

			size_t size() const { return m_size; }
			size_t max_size() const { return m_max_size; }

			const data_record *data_pointer() const { return m_data_pointer; }
			const data_record *section_pointer(size_t section) const { return &m_data_pointer[section * Config::ft_max_results_per_section]; }
			data_record *data_pointer() { return m_data_pointer; }
			data_record *section_pointer(size_t section) { return &m_data_pointer[section * Config::ft_max_results_per_section]; }
			std::span<data_record> *span_pointer() { return &m_span; }

			size_t total_num_results() const { return m_total_num_results ; };
			void set_total_num_results(size_t total_num_results);

			void resize(size_t n) {
				m_span = std::span<data_record>(m_data_pointer, n);
				m_size = n;
			}

			void prepare_sections(const std::string &filename, size_t offset, size_t len);
			void read_to_section(size_t section);
			bool has_next_section();
			size_t num_sections();
			void close_sections();
			void copy_vector(const std::vector<data_record> &vec);

		private:

			full_text_result_set(const full_text_result_set &res) = delete;

			std::span<data_record> m_span;
			data_record *m_data_pointer;

			size_t m_size; // The length in first section.
			const size_t m_max_size; // The maximum number of elements the result set can hold.
			size_t m_total_size; // The lengths of all elements in all sections.
			size_t m_total_num_results; // The total indexed length, only used to display total number of results.
			size_t m_section_len;
			size_t m_records_read;
			int m_file_descriptor;
			bool m_error = false;

	};

	template<typename data_record>
	full_text_result_set<data_record>::full_text_result_set(size_t size)
	: m_size(size), m_max_size(size), m_total_num_results(0)
	{
		m_file_descriptor = -1;
		m_data_pointer = new data_record[size];
		m_span = std::span<data_record>(m_data_pointer, size);
	}

	template<typename data_record>
	full_text_result_set<data_record>::~full_text_result_set() {
		delete []m_data_pointer;
	}

	template<typename data_record>
	void full_text_result_set<data_record>::set_total_num_results(size_t total_num_results) {
		m_total_num_results = total_num_results;
	}

	template<typename data_record>
	void full_text_result_set<data_record>::prepare_sections(const std::string &filename, size_t offset, size_t len) {

		assert(m_file_descriptor < 0);

		m_size = len / sizeof(data_record);
		m_total_size = m_size;
		if (m_size > Config::ft_max_results_per_section) m_size = Config::ft_max_results_per_section;

		m_file_descriptor = open(filename.c_str(), O_RDONLY);
		posix_fadvise(m_file_descriptor, offset, m_total_size * sizeof(data_record), POSIX_FADV_SEQUENTIAL);
		lseek(m_file_descriptor, offset, SEEK_SET);
		m_records_read = 0;
		resize(m_size);
	}

	/*
		Reads data up to and includint the section. So if the argument section equals zero the first section is read.
	*/
	template<typename data_record>
	void full_text_result_set<data_record>::read_to_section(size_t section) {
		size_t read_start = m_records_read;
		size_t read_end = (section + 1) * Config::ft_max_results_per_section;
		if (read_end > m_total_size) read_end = m_total_size;

		if (read_start > read_end) return;

		size_t records_to_read = read_end - read_start;

		int bytes_read = ::read(m_file_descriptor, (void *)&m_data_pointer[m_records_read], (size_t)records_to_read * sizeof(data_record));
		if (bytes_read < 0) {
			m_error = true;
		} else {
			m_error = false;
		}
		m_records_read += records_to_read;
	}

	template<typename data_record>
	bool full_text_result_set<data_record>::has_next_section() {
		if (m_file_descriptor < 0) return false;
		return m_total_size > m_records_read;
	}

	template<typename data_record>
	size_t full_text_result_set<data_record>::num_sections() {
		// Ceiling integer division of m_total_size/Config::ft_max_results_per_section;
		return (m_total_size + Config::ft_max_results_per_section - 1) / Config::ft_max_results_per_section;
	}

	template<typename data_record>
	void full_text_result_set<data_record>::close_sections() {
		if (m_file_descriptor >= 0) {
			close(m_file_descriptor);
			m_file_descriptor = -1;
		}
	}

	template<typename data_record>
	void full_text_result_set<data_record>::copy_vector(const std::vector<data_record> &vec) {
		memcpy(&m_data_pointer[0], vec.data(), vec.size() * sizeof(data_record));
		resize(vec.size());
	}

}
