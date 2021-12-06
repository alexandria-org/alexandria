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

#include "Parser.h"

using namespace std;

namespace Parser {

	bool is_percent_encoding(const char *cstr) {
		const char first = tolower(cstr[1]);
		const char second = tolower(cstr[2]);
		const bool first_valid = (first >= '0' && first <= '9') || (first >= 'a' && first <= 'f');
		const bool second_valid = (second >= '0' && second <= '9') || (second >= 'a' && second <= 'f');
		return cstr[0] == '%' && first_valid && second_valid;
	}

	string urldecode(const string &str) {
		const size_t len = str.size();
		const char *cstr = str.c_str();
		char *ret = new char[len + 1];
		size_t j = 0;
		for (size_t i = 0; i < len; i++) {
			if (i < len - 2 && is_percent_encoding(&cstr[i])) {
				ret[j++] = (char)stoi(string(&cstr[i + 1], 2), NULL, 16);
				i += 2;
			} else if (i < len - 1 && cstr[i] == '%' && cstr[i + 1] == '%') {
				ret[j++] = '%';
				i++;
			} else {
				ret[j++] = cstr[i];
			}
		}
		ret[j] = '\0';

		string ret_str(ret);

		delete ret;

		return ret_str;
	}

	string get_http_header(const string &record, const string &key) {
		const size_t pos = record.find(key);
		const size_t pos_end = record.find("\n", pos);
		if (pos == string::npos) {
			return "";
		}

		if (pos_end == string::npos) {
			return record.substr(pos + key.size());
		}

		return record.substr(pos + key.size(), pos_end - pos - key.size() - 1);
	}
}
