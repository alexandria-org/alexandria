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

#include "Text.h"

namespace Text {

	bool is_clean_char(const char *ch, size_t multibyte_len) {
		if (multibyte_len == 1) {
			return (ch[0] >= 'a' && ch[0] <= 'z') || (ch[0] >= '0' && ch[0] <= '9');
		} else if (multibyte_len == 2) {
			return (strncmp(ch, "å", 2) == 0) || (strncmp(ch, "ä", 2) == 0) || (strncmp(ch, "ö", 2) == 0);
		}
		return false;
	}

	bool is_clean_word(const string &s) {
		const char *str = s.c_str();
		size_t len = s.size();
		for (size_t i = 0; i < len; ) {
			size_t multibyte_len = 1;
			for (int j = i + 1; IS_MULTIBYTE_CODEPOINT(str[j]) && (j < len); j++, multibyte_len++) {
			}

			if (!is_clean_char(&str[i], multibyte_len)) {
				return false;
			}

			i += multibyte_len;
		}

		return true;
	}

	string clean_word(const string &s) {
		string result;
		const char *str = s.c_str();
		size_t len = s.size();
		for (size_t i = 0; i < len; ) {
			size_t multibyte_len = 1;
			for (int j = i + 1; IS_MULTIBYTE_CODEPOINT(str[j]) && (j < len); j++, multibyte_len++) {
			}

			if (is_clean_char(&str[i], multibyte_len)) {
				result.append(&str[i], multibyte_len);
			}

			i += multibyte_len;
		}

		return result;
	}

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	vector<string> get_words(const string &str, size_t limit) {

		const string word_boundary = " \t,|!";

		string str_lc = lower_case(str);

		vector<string> raw_words, words;
		boost::split(raw_words, str_lc, boost::is_any_of(word_boundary));

		for (string &word : raw_words) {
			trim(word);
			if (is_clean_word(word) && word.size() <= CC_MAX_WORD_LEN &&
					word.size() > 0) {
				words.push_back(word);
			}
			if (limit && words.size() == limit) break;
		}

		return words;
	}

	vector<string> get_words(const string &str) {

		return get_words(str, 0);
	}

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	vector<string> get_full_text_words(const string &str, size_t limit) {

		const string word_boundary = " \t,|!";

		string str_lc = lower_case(str);

		vector<string> raw_words, words;
		boost::split(raw_words, str_lc, boost::is_any_of(word_boundary));

		for (string &word : raw_words) {
			if (Unicode::is_valid(word)) {
				trim(word);
				if (word.size() <= CC_MAX_WORD_LEN && word.size() > 0) {
					words.push_back(word);
				}
				if (limit && words.size() == limit) break;
			}
			
		}

		return words;
	}

	vector<string> get_full_text_words(const string &str) {

		return get_full_text_words(str, 0);
	}

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
		These functions also expand on blend chars.
	*/
	vector<string> get_expanded_full_text_words(const string &str, size_t limit) {

		const string word_boundary = " \t,|!";
		const string blend_chars = ".-:";

		string str_lc = lower_case(str);

		vector<string> raw_words, words, blended;
		boost::split(raw_words, str_lc, boost::is_any_of(word_boundary));

		for (string &word : raw_words) {
			if (Unicode::is_valid(word)) {
				trim(word);
				if (word.size() <= CC_MAX_WORD_LEN && word.size() > 0) {
					words.push_back(word);

					if (limit && words.size() == limit) break;

					boost::split(blended, word, boost::is_any_of(blend_chars));
					if (blended.size() > 1) {
						for (string &blended_word : blended) {
							trim(blended_word);
							words.push_back(blended_word);
							if (limit && words.size() == limit) break;
						}
					}
				}
			}
			
		}

		return words;
	}

	vector<string> get_expanded_full_text_words(const string &str) {

		return get_expanded_full_text_words(str, 0);
	}

	/*
		Returns a vector of words lower case, punctuation trimmed and less or equal than CC_MAX_WORD_LEN length.
	*/
	vector<string> get_words_without_stopwords(const string &str, size_t limit) {

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

	vector<string> get_words_without_stopwords(const string &str) {

		return get_words_without_stopwords(str, 0);
	}

}
