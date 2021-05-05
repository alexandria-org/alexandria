
#pragma once

#include <vector>
#include <iostream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <sstream>

#include "Stopwords.h"

using namespace std;

#define CC_MAX_WORD_LEN 20
#define IS_MULTIBYTE(ch) ((ch >> 7) && !((ch >> 6) & 0x1))

class TextBase {

public:

	// trim from start (in place)
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

	// trim (in place)
	inline void trim(string &s) {
		ltrim(s);
		rtrim(s);
	}

	inline bool is_clean_char(const char *ch, size_t multibyte_len) {
		if (multibyte_len == 1) {
			return (ch[0] >= 'a' && ch[0] <= 'z') || (ch[0] >= '0' && ch[0] <= '9');
		} else if (multibyte_len == 2) {
			return (strncmp(ch, "å", 2) == 0) || (strncmp(ch, "ä", 2) == 0) || (strncmp(ch, "ö", 2) == 0);
		}
		return false;
	}

	inline string clean_word(const string &s) {
		string result;
		const char *str = s.c_str();
		size_t len = s.size();
		for (size_t i = 0; i < len; ) {
			size_t multibyte_len = 1;
			for (int j = i + 1; IS_MULTIBYTE(str[j]) && (j < len); j++, multibyte_len++) {
			}

			if (is_clean_char(&str[i], multibyte_len)) {
				result.append(&str[i], multibyte_len);
			}

			i += multibyte_len;
		}

		return result;
	}

	inline bool is_clean_word(const string &s) {
		const char *str = s.c_str();
		size_t len = s.size();
		for (size_t i = 0; i < len; ) {
			size_t multibyte_len = 1;
			for (int j = i + 1; IS_MULTIBYTE(str[j]) && (j < len); j++, multibyte_len++) {
			}

			if (!is_clean_char(&str[i], multibyte_len)) {
				return false;
			}

			i += multibyte_len;
		}

		return true;
	}

	inline string lower_case(const string &str) const {
		string ret = str;
		transform(ret.begin(), ret.end(), ret.begin(), [](unsigned char c){ return tolower(c); });
		return ret;
	}

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	inline vector<string> get_words(const string &str, size_t limit) {

		const string word_boundary = " \t,|!,";

		string str_lc = lower_case(str);

		vector<string> raw_words, words;
		boost::split(raw_words, str_lc, boost::is_any_of(word_boundary));

		for (string &word : raw_words) {
			trim(word);
			if (is_clean_word(word) && !Stopwords::is_stop_word(word) && word.size() <= CC_MAX_WORD_LEN &&
					word.size() > 0) {
				words.push_back(word);
			}
			if (limit && words.size() == limit) break;
		}

		return words;
	}

	inline vector<string> get_words(const string &str) {

		return get_words(str, 0);

	}

};