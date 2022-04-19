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

#include "tsv_file.h"
#include <exception>

using namespace std;

namespace file {

	tsv_file::tsv_file() {

	}

	tsv_file::tsv_file(const string &file_name) {
		set_file_name(file_name);
	}

	tsv_file::~tsv_file() {
		m_file.close();
	}

	string tsv_file::find(const string &key) {
		size_t pos = binary_find_position(m_file_size, 0, key);
		if (pos == string::npos) {
			return "";
		}

		m_file.seekg(pos, m_file.beg);
		

		string line;
		getline(m_file, line);

		return line;
	}

	size_t tsv_file::find_first_position(const string &key) {
		m_file.clear();
		m_file.seekg(0, m_file.beg);
		const size_t pos = binary_find_position(m_file_size, 0, key);
		if (pos == string::npos) return string::npos;
		// pos is the position of one item. but we need the first one.
		size_t jump = 1000;
		while (pos > jump) {
			m_file.seekg(pos - jump, m_file.beg);
			// read next line.
			string line;
			getline(m_file, line);
			getline(m_file, line);
			string jump_key = line.substr(0, line.find("\t"));
			if (jump_key < key) {
				// We jamp too far.
				break;
			}
			jump = jump << 1;
		}
		if (pos < jump) jump = pos;

		// The first occurance is between pos - jump and pos - (jump/2)
		// Linear search.
		m_file.seekg(pos - jump, m_file.beg);
		string line;
		if (pos > jump) {
			getline(m_file, line);
		}
		while (getline(m_file, line)) {
			string jump_key = line.substr(0, line.find("\t"));
			if (jump_key == key) {
				return (size_t)m_file.tellg() - (line.size() + 1u);
			}
		}
		return string::npos;
	}

	size_t tsv_file::find_last_position(const string &key) {
		m_file.clear();
		m_file.seekg(0, m_file.beg);
		const size_t pos = binary_find_position(m_file_size, 0, key);
		if (pos == string::npos) return string::npos;
		// pos is the position of one item. but we need the last one.
		size_t jump = 1000;
		while (pos + jump < m_file_size) {
			m_file.seekg(pos + jump, m_file.beg);
			// read next line.
			string line;
			getline(m_file, line);
			getline(m_file, line);
			string jump_key = line.substr(0, line.find("\t"));
			if (jump_key > key) {
				// We jamp too far.
				break;
			}
			jump = jump << 1;
		}
		jump = jump >> 1;
		if (pos + jump > m_file_size) {
			jump = 0;
		}

		// The first occurance is between pos - jump and pos - (jump/2)
		// Linear search.
		m_file.seekg(pos + jump, m_file.beg);
		size_t ret_pos = pos + jump;
		string line;
		getline(m_file, line);
		size_t last_line_length = line.size() + 1u;
		ret_pos += line.size() + 1u;
		while (getline(m_file, line)) {
			string jump_key = line.substr(0, line.find("\t"));
			if (jump_key > key) {
				return ret_pos - last_line_length;
			}
			ret_pos += line.size() + 1u;
			last_line_length = line.size() + 1u;
		}
		return ret_pos - last_line_length;
	}

	size_t tsv_file::find_next_position(const string &key) {
		m_file.clear();
		m_file.seekg(0, m_file.beg);
		const size_t pos = binary_find_position_any(m_file_size, 0, key);

		// pos is the position of one item. but we need the last one.
		size_t jump = 1000;
		while (pos + jump < m_file_size) {
			m_file.seekg(pos + jump, m_file.beg);
			// read next line.
			string line;
			getline(m_file, line);
			getline(m_file, line);
			string jump_key = line.substr(0, line.find("\t"));
			if (jump_key > key) {
				// We jamp too far.
				break;
			}
			jump = jump << 1;
		}
		jump = jump >> 1;
		if (pos + jump > m_file_size) {
			jump = 0;
		}

		// The first occurance is between pos - jump and pos - (jump/2)
		// Linear search.
		m_file.seekg(pos + jump, m_file.beg);
		size_t ret_pos = pos + jump;
		string line;
		getline(m_file, line);
		ret_pos += line.size() + 1u;
		while (getline(m_file, line)) {
			string jump_key = line.substr(0, line.find("\t"));
			if (jump_key > key) {
				return ret_pos;
			}
			ret_pos += line.size() + 1u;
		}
		return m_file_size;
	}

	map<string, string> tsv_file::find_all(const set<string> &keys) {
		m_file.clear();
		m_file.seekg(0, m_file.beg);
		size_t pos = 0;
		map<string, string> result;
		string line;
		for (const string &key : keys) {
			pos = binary_find_position(m_file_size, pos, key);
			if (pos != string::npos) {
				m_file.seekg(pos, m_file.beg);
				getline(m_file, line);
				result[key] = line;
			} else {
				// Key not found, ignore.
			}
		}

		return result;
	}

	size_t tsv_file::read_column_into(int column, set<string> &container) {
		m_file.clear();
		m_file.seekg(0, m_file.beg);

		if (!m_file.is_open()) {
			throw runtime_error("File is not open any more: " + m_file_name);
		}

		string line;
		size_t rows_read = 0;
		while (getline(m_file, line)) {
			stringstream ss(line);
			string col;
			ss >> col;
			container.insert(col);
			rows_read++;
		}

		return rows_read;
	}

	size_t tsv_file::read_column_into(int column, set<string> &container, size_t limit) {
		m_file.clear();
		m_file.seekg(0, m_file.beg);

		if (!m_file.is_open()) {
			throw runtime_error("File is not open any more: " + m_file_name);
		}

		string line;
		size_t rows_read = 0;
		while (getline(m_file, line)) {
			stringstream ss(line);
			string col;
			ss >> col;
			container.insert(col);
			rows_read++;
			if (rows_read >= limit) break;
		}

		return rows_read;
	}

	size_t tsv_file::read_column_into(int column, set<string> &container, size_t limit, size_t offset) {
		m_file.clear();
		m_file.seekg(0, m_file.beg);

		if (!m_file.is_open()) {
			throw runtime_error("File is not open any more: " + m_file_name);
		}

		string line;
		size_t rows_read = 0;
		while (getline(m_file, line)) {
			stringstream ss(line);
			string col;
			ss >> col;
			if (rows_read >= offset) {
				container.insert(col);
			}
			rows_read++;
			if (rows_read >= limit) break;
		}

		return rows_read;
	}

	size_t tsv_file::size() const {
		return m_file_size;
	}

	bool tsv_file::eof() const {
		return m_file.eof();
	}

	bool tsv_file::is_open() const {
		return m_file.is_open();
	}

	string tsv_file::get_line() {
		string line;
		getline(m_file, line);
		return line;
	}

	size_t tsv_file::read_column_into(int column, vector<string> &container) {
		m_file.clear();
		m_file.seekg(0, m_file.beg);

		string line;
		size_t rows_read = 0;
		while (getline(m_file, line)) {
			stringstream ss(line);
			string col;
			ss >> col;
			container.push_back(col);
			rows_read++;
		}

		return rows_read;
	}

	size_t tsv_file::read_column_into(int column, vector<string> &container, size_t limit) {
		m_file.clear();
		m_file.seekg(0, m_file.beg);

		string line;
		size_t rows_read = 0;
		while (getline(m_file, line)) {
			stringstream ss(line);
			string col;
			ss >> col;
			container.push_back(col);
			rows_read++;
			if (rows_read >= limit) break;
		}

		return rows_read;
	}

	size_t tsv_file::read_column_into(int column, vector<string> &container, size_t limit, size_t offset) {
		m_file.clear();
		m_file.seekg(0, m_file.beg);

		string line;
		size_t rows_read = 0;
		while (getline(m_file, line)) {
			stringstream ss(line);
			string col;
			ss >> col;
			if (rows_read >= offset) {
				container.push_back(col);
			}
			rows_read++;
			if (rows_read >= limit) break;
		}

		return rows_read;
	}

	size_t tsv_file::binary_find_position(size_t file_size, size_t offset, const string &key) {

		string line;

		if (file_size - offset < 750) {
			// Make linear search.
			m_file.seekg(offset, m_file.beg);
			size_t bytes_read = 0;
			while (getline(m_file, line) && bytes_read <= file_size - offset) {
				bytes_read += (line.size() + 1u);
				if (line.starts_with(key + "\t")) {
					return (size_t)m_file.tellg() - (line.size() + 1u);
				}
			}

			return string::npos;
		}

		size_t pivot_len_1 = (file_size - offset) / 2;
		size_t pivot = offset + pivot_len_1;

		// Get key at pivot.
		m_file.seekg(pivot, m_file.beg);

		getline(m_file, line);
		getline(m_file, line);
		string pivot_key = line.substr(0, line.find("\t"));

		if (key < pivot_key) {
			return binary_find_position(offset + pivot_len_1, offset, key);
		} else if (key > pivot_key) {
			return binary_find_position(file_size, pivot, key);
		}

		return (size_t)m_file.tellg() - (line.size() + 1u);
	}

	size_t tsv_file::binary_find_position_any(size_t file_size, size_t offset, const string &key) {

		string line;

		if (file_size - offset < 750) {
			// Make linear search.
			m_file.seekg(offset, m_file.beg);
			size_t bytes_read = 0;
			while (getline(m_file, line) && bytes_read <= file_size - offset) {
				bytes_read += (line.size() + 1u);
				const string this_key = line.substr(0, line.find("\t"));
				if (this_key >= key) {
					return (size_t)m_file.tellg() - (line.size() + 1u);
				}
			}

			return m_file_size;
		}

		size_t pivot_len_1 = (file_size - offset) / 2;
		size_t pivot = offset + pivot_len_1;

		// Get key at pivot.
		m_file.seekg(pivot, m_file.beg);

		getline(m_file, line);
		getline(m_file, line);
		string pivot_key = line.substr(0, line.find("\t"));

		if (key < pivot_key) {
			return binary_find_position(offset + pivot_len_1, offset, key);
		} else if (key > pivot_key) {
			return binary_find_position(file_size, pivot, key);
		}

		return (size_t)m_file.tellg() - (line.size() + 1u);
	}

	void tsv_file::set_file_name(const string &file_name) {

		m_file_name = file_name;
		m_original_file_name = file_name;

		m_file.open(m_file_name);

		if (!m_file.is_open()) {
			throw runtime_error("Could not open file: " + m_file_name + " error: " + strerror(errno));
		}

		m_file.seekg(0, m_file.end);
		m_file_size = m_file.tellg();
		m_file.seekg(0, m_file.beg);
	}

}
