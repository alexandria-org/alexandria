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

#include "DictionaryRow.h"

DictionaryRow::DictionaryRow() {
}

DictionaryRow::DictionaryRow(const DictionaryRow &row) {
	m_columns = row.m_columns;
}

DictionaryRow::DictionaryRow(const string &row) {
	stringstream stream(row);
	read_stream(stream);
}

DictionaryRow::DictionaryRow(stringstream &stream) {
	read_stream(stream);
}

DictionaryRow::~DictionaryRow() {

}

int DictionaryRow::get_int(int column) const {
	return (int)m_columns[column];
}

float DictionaryRow::get_float(int column) const {
	return (float)m_columns[column];
}

double DictionaryRow::get_double(int column) const {
	return m_columns[column];
}

void DictionaryRow::read_stream(stringstream &stream) {
	string col;
	int i = 0;
	while (getline(stream, col, '\t')) {
		try {
			m_columns.push_back(stod(col));
		} catch(const invalid_argument &error) {

		} catch(const out_of_range &error) {
		}
		i++;
	}
}
