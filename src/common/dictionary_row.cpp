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

#include "dictionary_row.h"

namespace common {

	dictionary_row::dictionary_row() {
	}

	dictionary_row::dictionary_row(const dictionary_row &row) {
		m_columns = row.m_columns;
	}

	dictionary_row::dictionary_row(const std::string &row) {
		std::stringstream stream(row);
		read_stream(stream);
	}

	dictionary_row::dictionary_row(std::stringstream &stream) {
		read_stream(stream);
	}

	dictionary_row::~dictionary_row() {

	}

	int dictionary_row::get_int(int column) const {
		return (int)m_columns[column];
	}

	float dictionary_row::get_float(int column) const {
		return (float)m_columns[column];
	}

	double dictionary_row::get_double(int column) const {
		return m_columns[column];
	}

	void dictionary_row::read_stream(std::stringstream &stream) {
		std::string col;
		int i = 0;
		while (std::getline(stream, col, '\t')) {
			try {
				m_columns.push_back(stod(col));
			} catch(const std::invalid_argument &error) {

			} catch(const std::out_of_range &error) {
			}
			i++;
		}
	}

}
