
#include "FullText.h"
#include "search_engine/SearchEngine.h"

namespace FullText {

	hash<string> hasher;

	map<uint64_t, float> tsv_data_to_scores(const string &tsv_data, const SubSystem *sub_system) {

		const vector<size_t> cols = {1, 2, 3, 4};
		const vector<float> scores = {10.0, 3.0, 2.0, 1};

		vector<string> col_values;
		boost::algorithm::split(col_values, tsv_data, boost::is_any_of("\t"));

		URL url(col_values[0]);
		float harmonic = url.harmonic(sub_system);

		const string site_colon = "site:" + url.host() + " site:www." + url.host() + " " + url.host() + " " + url.domain_without_tld();

		size_t score_index = 0;
		map<uint64_t, float> word_map;

		add_words_to_word_map(Text::get_full_text_words(site_colon), 20*harmonic, word_map);

		for (size_t col_index : cols) {
			add_words_to_word_map(Text::get_expanded_full_text_words(col_values[col_index]), scores[score_index]*harmonic, word_map);
			score_index++;
		}

		return word_map;
	}

	void add_words_to_word_map(const vector<string> &words, float score, map<uint64_t, float> &word_map) {

		map<uint64_t, uint64_t> uniq;
		for (const string &word : words) {
			const uint64_t word_hash = hasher(word);
			if (uniq.find(word_hash) == uniq.end()) {
				word_map[word_hash] += score;
				uniq[word_hash] = word_hash;
			}
		}
	}

	vector<string> make_partition_from_files(const vector<string> &files, size_t partition, size_t max_partitions) {
		vector<string> ret;
		for (size_t i = 0; i < files.size(); i++) {
			if ((i % max_partitions) == partition) {
				ret.push_back(files[i]);
			}
		}

		return ret;
	}

	vector<FullTextRecord> search_index_array(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const string &query, size_t limit, struct SearchMetric &metric) {

		vector<future<vector<FullTextRecord>>> futures;
		vector<struct SearchMetric> metrics(index_array.size(), SearchMetric{});

		size_t idx = 0;
		for (FullTextIndex<FullTextRecord> *index : index_array) {
			future<vector<FullTextRecord>> future = async(SearchEngine::search, index->shards(), links, query, limit, ref(metrics[idx]));
			futures.push_back(move(future));
			idx++;
		}

		Profiler profiler1("execute all threads");
		vector<FullTextRecord> complete_result;
		for (auto &future : futures) {
			vector<FullTextRecord> result = future.get();
			complete_result.insert(complete_result.end(), result.begin(), result.end());
		}

		// Sort.
		Profiler profiler2("sort total results");
		sort(complete_result.begin(), complete_result.end(), [](const FullTextRecord &a, const FullTextRecord &b) {
			return a.m_score > b.m_score;
		});
		if (complete_result.size() > limit) {
			complete_result.resize(limit);
		}
		return complete_result;
	}

	vector<LinkFullTextRecord> search_link_array(vector<FullTextIndex<LinkFullTextRecord> *> index_array, const string &query, size_t limit,
		struct SearchMetric &metric) {

		vector<future<vector<LinkFullTextRecord>>> futures;
		vector<struct SearchMetric> metrics(index_array.size(), SearchMetric{});

		size_t idx = 0;
		for (FullTextIndex<LinkFullTextRecord> *index : index_array) {
			future<vector<LinkFullTextRecord>> future = async(SearchEngine::search_links, index->shards(), query, limit, ref(metrics[idx]));
			futures.push_back(move(future));
			idx++;
		}

		Profiler profiler1("execute all threads");
		vector<LinkFullTextRecord> complete_result;
		for (auto &future : futures) {
			vector<LinkFullTextRecord> result = future.get();
			complete_result.insert(complete_result.end(), result.begin(), result.end());
		}

		sort(complete_result.begin(), complete_result.end(), [](const LinkFullTextRecord &a, const LinkFullTextRecord &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		return complete_result;
	}

}
