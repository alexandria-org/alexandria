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

#include "config.h"
#include "text/text.h"
#include "full_text/full_text_index.h"
#include "full_text/full_text_shard.h"

namespace stats {

	std::hash<std::string> hasher;

	template<typename data_record>
	std::map<std::string, double> word_stats(const full_text::full_text_index<data_record> &index, const std::string &query, size_t index_size);

	template<typename data_record>
	std::map<std::string, double> get_word_counts(const std::vector<full_text::full_text_shard<data_record> *> &shards, const std::string &query) {

		std::vector<std::string> words = text::get_full_text_words(query);
		if (words.size() == 0) return {};

		std::map<std::string, double> result;
		std::vector<std::string> searched_words;
		for (const std::string &word : words) {

			// One word should only be searched once.
			if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
			searched_words.push_back(word);

			uint64_t word_hash = hasher(word);
			result[word] = shards[word_hash % config::ft_num_shards]->total_num_results(word_hash);
		}

		return result;
	}

	template<typename data_record>
	std::map<std::string, double> word_stats(const full_text::full_text_index<data_record> &index, const std::string &query, size_t index_size) {

		std::map<std::string, double> complete_result = get_word_counts<data_record>(index.shards(), query);

		for (const auto &iter : complete_result) {
			complete_result[iter.first] /= index_size;
		}

		return complete_result;
	}

}
