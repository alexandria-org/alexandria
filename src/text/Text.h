
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
