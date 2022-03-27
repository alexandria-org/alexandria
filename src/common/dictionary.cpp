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

#include "dictionary.h"
#include "logger/logger.h"
#include "file/tsv_file.h"
#include "dictionary_row.h"

using namespace std;

namespace common {

	dictionary::dictionary() {

	}

	dictionary::dictionary(file::tsv_file &tsv_file) {
		load_tsv(tsv_file);
	}

	dictionary::~dictionary() {

	}

	void dictionary::load_tsv(file::tsv_file &tsv_file) {
		while (!tsv_file.eof()) {
			string line = tsv_file.get_line();
			stringstream ss(line);
			string col;
			getline(ss, col, '\t');

			if (col.size()) {
				size_t key = hash<string>{}(col);

				if (m_rows.find(key) != m_rows.end()) {
					handle_collision(key, col);
				}

				m_rows[key] = dictionary_row(ss);
			}
		}
	}

	unordered_map<size_t, dictionary_row>::const_iterator dictionary::find(const string &key) const {
		return m_rows.find(hash<string>{}(key));
	}

	unordered_map<size_t, dictionary_row>::const_iterator dictionary::begin() const {
		return m_rows.begin();
	}

	unordered_map<size_t, dictionary_row>::const_iterator dictionary::end() const {
		return m_rows.end();
	}

	bool dictionary::has_key(const string &key) const {
		return find(key) != end();
	}

	void dictionary::handle_collision(size_t key, const string &col) {
		LOG_ERROR("Collision: " + to_string(key) + " " + col);
	}
}
