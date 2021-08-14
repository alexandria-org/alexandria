
#pragma once

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
			result[word] = shards[word_hash % FT_NUM_SHARDS]->total_num_results(word_hash);
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
