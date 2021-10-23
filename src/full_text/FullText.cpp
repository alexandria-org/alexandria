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

#include "FullText.h"
#include "FullTextShardBuilder.h"
#include "FullTextIndexerRunner.h"
#include "link_index/LinkIndexerRunner.h"
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

	void truncate_index(const string &index_name) {
		for (size_t partition = 0; partition < Config::ft_num_partitions; partition++) {

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

	void index_files(const string &batch, const string &db_name, const string &hash_table_name, const vector<string> files,
			const SubSystem *sub_system) {
		for (size_t partition_num = 0; partition_num < Config::ft_num_partitions; partition_num++) {
			FullTextIndexerRunner indexer(db_name + "_" + to_string(partition_num), hash_table_name, batch, sub_system);
			indexer.run(files, partition_num);
		}
	}

	vector<string> download_batch(const string &batch, size_t limit, size_t offset) {
		
		TsvFileRemote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
		vector<string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		vector<string> files_to_download;
		for (size_t i = offset; i < warc_paths.size() && i < (offset + limit); i++) {
			string warc_path = warc_paths[i];
			const size_t pos = warc_path.find(".warc.gz");
			if (pos != string::npos) {
				warc_path.replace(pos, 8, ".gz");
			}
			files_to_download.push_back(warc_path);
		}

		return Transfer::download_gz_files_to_disk(files_to_download);
	}

	vector<string> download_link_batch(const string &batch, size_t limit, size_t offset) {
		
		TsvFileRemote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
		vector<string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		vector<string> files_to_download;
		for (size_t i = offset; i < warc_paths.size() && i < (offset + limit); i++) {
			string warc_path = warc_paths[i];
			const size_t pos = warc_path.find(".warc.gz");
			if (pos != string::npos) {
				warc_path.replace(pos, 8, ".links.gz");
			}
			files_to_download.push_back(warc_path);
		}

		return Transfer::download_gz_files_to_disk(files_to_download);
	}

	void index_all_batches(const string &db_name, const string &hash_table_name) {
		SubSystem *sub_system = new SubSystem();
		for (const string &batch : Config::batches) {
			index_batch(db_name, hash_table_name, batch, sub_system);
		}
	}

	void index_batch(const string &db_name, const string &hash_table_name, const string &batch, const SubSystem *sub_system) {

		vector<string> files;
		const size_t limit = 15000;
		size_t offset = 0;

		while (true) {
			vector<string> files = download_batch(batch, limit, offset);
			if (files.size() == 0) break;
			index_files(batch, db_name, hash_table_name, files, sub_system);
			Transfer::delete_downloaded_files(files);
			offset += files.size();
		}

	}

	void index_single_batch(const string &db_name, const string &hash_table_name, const string &batch) {

		SubSystem *sub_system = new SubSystem();
		index_batch(db_name, hash_table_name, batch, sub_system);

	}

	void index_all_link_batches(const string &db_name, const string &domain_db_name, const string &hash_table_name,
			const string &domain_hash_table_name) {

		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		SubSystem *sub_system = new SubSystem();

		url_to_domain->read();

		for (const string &batch : Config::link_batches) {
			index_link_batch(db_name, domain_db_name, hash_table_name, domain_hash_table_name, batch, sub_system, url_to_domain);
		}
	}
	
	void index_link_files(const string &batch, const string &db_name, const string &domain_db_name, const string &hash_table_name,
		const string &domain_hash_table_name, const vector<string> &files, const SubSystem *sub_system, UrlToDomain *url_to_domain) {

		LinkIndexerRunner indexer(db_name, domain_db_name, hash_table_name, domain_hash_table_name, batch, sub_system, url_to_domain);
		indexer.run(files);
	}

	void index_link_batch(const string &db_name, const string &domain_db_name, const string &hash_table_name, const string &domain_hash_table_name,
		const string &batch, const SubSystem *sub_system, UrlToDomain *url_to_domain) {

		vector<string> files;
		const size_t limit = 15000;
		size_t offset = 0;

		while (true) {
			vector<string> files = download_link_batch(batch, limit, offset);
			if (files.size() == 0) break;
			index_link_files(batch, db_name, domain_db_name, hash_table_name, domain_hash_table_name, files, sub_system, url_to_domain);
			Transfer::delete_downloaded_files(files);
			offset += files.size();
		}

	}

	void index_single_link_batch(const string &db_name, const string &domain_db_name, const string &hash_table_name,
		const string &domain_hash_table_name, const string &batch) {

		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		SubSystem *sub_system = new SubSystem();

		url_to_domain->read();

		index_link_batch(db_name, domain_db_name, hash_table_name, domain_hash_table_name, batch, sub_system, url_to_domain);
	}

	bool should_index_url(const URL &url, size_t partition) {
		return should_index_hash(url.host_hash(), partition);
	}

	bool should_index_hash(size_t hash, size_t partition) {
		size_t mod = hash % (Config::nodes_in_cluster * Config::ft_num_partitions);
		bool in_partition = (mod % Config::ft_num_partitions) == partition;
		bool in_node = (mod / Config::ft_num_partitions) == Config::node_id;
		return in_partition && in_node;
	}

	bool should_index_link(const Link &link, size_t partition) {
		return true;
	}

	bool should_index_link_hash(size_t hash, size_t partition) {
		return (hash % 8) == partition;
	}

}
