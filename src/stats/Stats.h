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
#include "text/Text.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextShard.h"

namespace Stats {

	hash<string> hasher;

	template<typename DataRecord>
	map<string, double> word_stats(vector<FullTextIndex<DataRecord> *> index_array, const string &query, size_t index_size);

	template<typename DataRecord>
	map<string, double> get_word_counts(const vector<FullTextShard<DataRecord> *> &shards, const string &query) {

		vector<string> words = Text::get_full_text_words(query);
		if (words.size() == 0) return {};

		map<string, double> result;
		vector<string> searched_words;
		for (const string &word : words) {

			// One word should only be searched once.
			if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
			searched_words.push_back(word);

			uint64_t word_hash = hasher(word);
			result[word] = shards[word_hash % Config::ft_num_shards]->total_num_results(word_hash);
			cout << "found word count: " << result[word] << " for " << word << " at shard " << word_hash % Config::ft_num_shards << endl;
		}

		return result;
	}

	template<typename DataRecord>
	map<string, double> word_stats(vector<FullTextIndex<DataRecord> *> index_array, const string &query, size_t index_size) {

		vector<future<map<string, double>>> futures;

		size_t idx = 0;
		for (FullTextIndex<DataRecord> *index : index_array) {
			future<map<string, double>> future = async(get_word_counts<DataRecord>, index->shards(), query);
			futures.push_back(move(future));
			idx++;
		}

		map<string, double> complete_result;
		for (auto &future : futures) {
			map<string, double> result = future.get();
			for (const auto &iter : result) {
				complete_result[iter.first] += (double)iter.second;
			}
		}

		for (const auto &iter : complete_result) {
			complete_result[iter.first] /= index_size;
		}

		return complete_result;
	}

}
