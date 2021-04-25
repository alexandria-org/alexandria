
#pragma once

#include <vector>
#include <iostream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <sstream>

using namespace std;

#define CC_MAX_WORD_LEN 20

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

	inline string lower_case(const string &str) {
		string ret = str;
		transform(ret.begin(), ret.end(), ret.begin(), [](unsigned char c){ return tolower(c); });
		return ret;
	}

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	inline vector<string> get_words(const string &str) {

		const string word_boundary = " \t,|!.,";

		string str_lc = lower_case(str);

		vector<string> raw_words, words;
		boost::split(raw_words, str_lc, boost::is_any_of(word_boundary));

		for (string &word : raw_words) {
			trim(word);
			if (word.size() <= CC_MAX_WORD_LEN && word.size() > 0) {
				words.push_back(word);
			}
		}

		return words;
	}

};