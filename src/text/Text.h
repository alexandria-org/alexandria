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

#define CC_MAX_WORD_LEN 100

#include <vector>
#include <iostream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include "Stopwords.h"
#include "parser/Unicode.h"

using namespace std;

namespace Text {

	inline void ltrim(string &s) {
		s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch) {
			return !isspace(ch) && !ispunct(ch);
		}));
	}

	// trim from end (in place)
	inline void rtrim(string &s) {
		s.erase(find_if(s.rbegin(), s.rend(), [](int ch) {
			return !isspace(ch) && !ispunct(ch);
		}).base(), s.end());
	}

	// trim return
	inline string trim(string &s) {
		ltrim(s);
		rtrim(s);
		return s;
	}

	inline string lower_case(const string &str) {
		string ret = str;
		transform(ret.begin(), ret.end(), ret.begin(), [](unsigned char c){ return tolower(c); });
		return ret;
	}

	bool is_clean_char(const char *ch, size_t multibyte_len);
	bool is_clean_word(const string &s);
	string clean_word(const string &s);

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	vector<string> get_words(const string &str, size_t limit);
	vector<string> get_words(const string &str);

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	vector<string> get_full_text_words(const string &str, size_t limit);
	vector<string> get_full_text_words(const string &str);

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
		These functions also expand on blend chars.
	*/
	vector<string> get_expanded_full_text_words(const string &str, size_t limit);
	vector<string> get_expanded_full_text_words(const string &str);

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	vector<string> get_words_without_stopwords(const string &str, size_t limit);
	vector<string> get_words_without_stopwords(const string &str);

}
