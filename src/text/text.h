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
#include <map>
#include <iostream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include "stopwords.h"
#include "parser/unicode.h"
#include "algorithm/hash.h"

namespace text {

	inline void ltrim(std::string &s) {
		s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch) {
			return !isspace(ch) && !ispunct(ch);
		}));
	}

	// trim from end (in place)
	inline void rtrim(std::string &s) {
		s.erase(find_if(s.rbegin(), s.rend(), [](int ch) {
			return !isspace(ch) && !ispunct(ch);
		}).base(), s.end());
	}

	// trim return
	inline std::string trim(std::string &s) {
		ltrim(s);
		rtrim(s);
		return s;
	}

	inline std::string trim(const std::string &s) {
		std::string copy = s;
		ltrim(copy);
		rtrim(copy);
		return copy;
	}

	inline void ltrim_punct(std::string &s) {
		s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch) {
			return !ispunct(ch);
		}));
	}

	// trim from end (in place)
	inline void rtrim_punct(std::string &s) {
		s.erase(find_if(s.rbegin(), s.rend(), [](int ch) {
			return !ispunct(ch);
		}).base(), s.end());
	}

	// trim return
	inline std::string trim_punct(std::string &s) {
		ltrim_punct(s);
		rtrim_punct(s);
		return s;
	}

	inline std::string trim_punct(const std::string &s) {
		std::string copy = s;
		ltrim(copy);
		rtrim(copy);
		return copy;
	}

	inline std::string lower_case(const std::string &str) {
		std::string ret = str;
		transform(ret.begin(), ret.end(), ret.begin(), [](unsigned char c){ return tolower(c); });
		return ret;
	}

	bool is_clean_char(const char *ch, size_t multibyte_len);
	bool is_clean_word(const std::string &s);
	std::string clean_word(const std::string &s);

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	std::vector<std::string> get_words(const std::string &str, size_t limit);
	std::vector<std::string> get_words(const std::string &str);

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	std::vector<std::string> get_full_text_words(const std::string &str, size_t limit);
	std::vector<std::string> get_full_text_words(const std::string &str);

	std::vector<uint64_t> get_tokens(const std::string &str, std::function<uint64_t(std::string)> str2token);
	std::vector<uint64_t> get_tokens(const std::string &str);

	std::vector<std::string> get_snippets(const std::string &str);

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
		These functions also expand on blend chars.
	*/
	std::vector<std::string> get_expanded_full_text_words(const std::string &str, size_t limit);
	std::vector<std::string> get_expanded_full_text_words(const std::string &str);

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	std::vector<std::string> get_words_without_stopwords(const std::string &str, size_t limit);
	std::vector<std::string> get_words_without_stopwords(const std::string &str);

	void words_to_ngram_hash(const std::vector<std::string> &words, size_t n_grams, const std::function<void(uint64_t)> &ins);

	std::map<std::string, size_t> get_word_counts(const std::string &text);
	std::map<std::string, float> get_word_frequency(const std::string &text);

}
