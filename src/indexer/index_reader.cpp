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

#include "index_reader.h"
#include <string.h>

using namespace std;

namespace indexer {

	index_reader_file::index_reader_file(const std::string &filename) {
		m_reader = make_unique<ifstream>();
		m_reader->open(filename, ios::binary);
	}

	index_reader_file::index_reader_file(index_reader_file &&other) {
		m_reader = move(other.m_reader);
	}

	bool index_reader_file::seek(size_t position) {
		if (!m_reader->is_open()) return false;
		m_reader->seekg(position, ios::beg);
		return true;
	}

	void index_reader_file::read(char *buffer, size_t length) {
		m_reader->read(buffer, length);
	}

	size_t index_reader_file::size() {
		m_reader->seekg(0, ios::end);
		return m_reader->tellg();
	}

	index_reader_ram::index_reader_ram(char *buffer, size_t length) {
		m_buffer = buffer;
		m_len = length;
	}

	index_reader_ram::index_reader_ram(index_reader_ram &&other) {
		m_buffer = other.m_buffer;
		m_len = other.m_len;

		other.m_buffer = nullptr;
		other.m_len = 0;
	}


	bool index_reader_ram::seek(size_t position) {
		if (position < m_len) {
			m_pos = position;
			return true;
		}
		return false;
	}

	void index_reader_ram::read(char *buffer, size_t length) {
		if (m_pos + length <= m_len) {
			memcpy(buffer, &m_buffer[m_pos], length);
			m_pos += length;
		}
	}

}
