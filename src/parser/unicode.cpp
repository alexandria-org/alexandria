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

#include "unicode.h"

using namespace std;

namespace parser {

	std::string unicode::encode(const std::string &str) {

		const char *cstr = str.c_str();
		size_t len = str.size();

		char *target = new char[str.size()];

		size_t last_unicode = len;
		size_t utf8_len = 0;
		for (size_t i = 0; i < len; i++) {
			bool copy = true;
			if (utf8_len == 0) {
				if (IS_UTF8_START_1(cstr[i])) {
					utf8_len = 1;
					last_unicode = i;
				} else if (IS_UTF8_START_2(cstr[i])) {
					utf8_len = 2;
					last_unicode = i;
				} else if (IS_UTF8_START_3(cstr[i])) {
					utf8_len = 3;
					last_unicode = i;
				} else if (IS_UNKNOWN_UTF8_START(cstr[i])) {
					copy = false;
				} else if ('\x00' <= cstr[i] && cstr[i] <= '\x1f') {
					copy = false;
				}
			} else if (IS_MULTIBYTE_CODEPOINT(cstr[i])) {
				utf8_len--;
			} else {
				// This unicode character has been terminated too soon.
				copy = false;
				for (size_t j = last_unicode; j <= i; j++) {
					target[j] = '?';
				}
				utf8_len = 0;
			}
			if (copy) {
				target[i] = cstr[i];
			} else {
				target[i] = '?';
			}
		}

		std::string ret(target, len);
		delete []target;
		if (utf8_len) {
			return ret.substr(0, last_unicode);
		} else {
			return ret;
		}
	}

	bool unicode::is_valid(const std::string &str) {
		
		const char *cstr = str.c_str();
		size_t len = str.size();

		size_t utf8_len = 0;
		for (size_t i = 0; i < len; i++) {
			if (utf8_len == 0) {
				if (IS_UTF8_START_1(cstr[i])) {
					utf8_len = 1;
				} else if (IS_UTF8_START_2(cstr[i])) {
					utf8_len = 2;
				} else if (IS_UTF8_START_3(cstr[i])) {
					utf8_len = 3;
				} else if (IS_UNKNOWN_UTF8_START(cstr[i])) {
					return false;
				}
			} else if (IS_MULTIBYTE_CODEPOINT(cstr[i])) {
				utf8_len--;
			} else {
				// This unicode character has been terminated too soon.
				return false;
			}
		}

		if (utf8_len) {
			return false;
		}

		return true;
	}

}
