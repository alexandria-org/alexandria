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

#include "GzTsvFile.h"
#include <exception>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

namespace File {

	GzTsvFile::GzTsvFile() {

	}

	GzTsvFile::GzTsvFile(const string &file_name) {
		m_file_name = file_name;

		ifstream infile(m_file_name);

		if (infile.is_open()) {
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			m_data = string(istreambuf_iterator<char>(decompress_stream), {});
		}
	}

	GzTsvFile::~GzTsvFile() {
	}

	size_t GzTsvFile::read_column_into(size_t column, vector<string> &container) {
		stringstream ss(m_data);

		string line;
		size_t rows_read = 0;
		while (getline(ss, line)) {
			vector<string> cols;
			boost::algorithm::split(cols, line, boost::is_any_of("\t"));
			if (cols.size() > column) {
				container.push_back(cols[column]);
			} else {
				container.push_back("");
			}
			rows_read++;
		}

		return rows_read;
	}

}
