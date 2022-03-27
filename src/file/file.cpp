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

#include "config.h"
#include "file.h"

using namespace std;

namespace file {

	string read_test_file(const string &file_name) {

		ifstream file(config::test_data_path + file_name);
		if (file.is_open()) {
			string ret;
			file.seekg(0, ios::end);
			ret.resize(file.tellg());
			file.seekg(0, ios::beg);
			file.read(&ret[0], ret.size());
			file.close();
			return ret;
		}
		return "";
	}

	void copy_file(const string &source, const string &dest) {
		ifstream infile(source, ios::binary);
		ofstream outfile(dest, ios::binary | ios::trunc);

		outfile << infile.rdbuf();
	}

	void delete_file(const string &file) {
		remove(file.c_str());
	}

}
