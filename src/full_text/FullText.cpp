
#include "FullText.h"
#include "FullTextShardBuilder.h"
#include "search_engine/SearchEngine.h"

namespace FullText {

	hash<string> hasher;

	void truncate_url_to_domain(const string &index_name) {

		for (size_t bucket_id = 1; bucket_id < 8; bucket_id++) {
			const string file_name = "/mnt/"+to_string(bucket_id)+"/full_text/url_to_domain_"+index_name+".fti";
			ofstream outfile(file_name, ios::binary | ios::trunc);
			outfile.close();
		}

	}

	void truncate_index(const string &index_name, size_t partitions) {
		for (size_t partition = 0; partition < partitions; partition++) {

			const string db_name = index_name + "_" + to_string(partition);

			for (size_t shard_id = 0; shard_id < Config::ft_num_shards; shard_id++) {
				FullTextShardBuilder<struct FullTextRecord> *shard_builder =
					new FullTextShardBuilder<struct FullTextRecord>(db_name, shard_id);
				shard_builder->truncate();
				delete shard_builder;
			}
		}
	}

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

}
