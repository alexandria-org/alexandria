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

#include <math.h>
#include "hash/Hash.h"
#include "hash_table/HashTable.h"
#include "hash_table/HashTableShardBuilder.h"
#include "parser/URL.h"
#include "transfer/Transfer.h"
#include "system/Logger.h"
#include "algorithm/Algorithm.h"
#include "config.h"
#include <vector>
#include <mutex>

using namespace std;

namespace Link {

	void count_links_in_stream(const SubSystem *sub_system, vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream,
			map<size_t, map<size_t, float>> &counter) {

		string line;
		while (getline(stream, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			const string link_text = col_values[4].substr(0, 1000);
			URL target_url(col_values[2], col_values[3]);
			URL source_url(col_values[0], col_values[1]);

			const uint64_t link_hash = Hash::str(source_url.host_top_domain());
			const uint64_t target_url_hash = target_url.hash();

			float harmonic = source_url.harmonic(sub_system);

			counter[target_url_hash][link_hash] = expm1(25.0*harmonic) / 50.0;
			shard_builders[target_url_hash % Config::ht_num_shards]->add(target_url_hash, target_url.str());
		}
	}

	map<size_t, map<size_t, float>> run_count_thread_with_local_files(const SubSystem *sub_system, const string &db_name, const vector<string> &local_files,
			vector<mutex> &write_mutexes) {

		map<size_t, map<size_t, float>> counter;

		vector<HashTableShardBuilder *> shard_builders;
		for (size_t i = 0; i < Config::ht_num_shards; i++) {
			shard_builders.push_back(new HashTableShardBuilder(db_name, i));
		}

		size_t idx = 1;
		for (const string &local_file : local_files) {

			ifstream stream(local_file, ios::in);

			if (stream.is_open()) {
				{
					count_links_in_stream(sub_system, shard_builders, stream, counter);
				}
			}

			stream.close();

			for (size_t i = 0; i < Config::ht_num_shards; i++) {
				if (shard_builders[i]->full()) {
					write_mutexes[i].lock();
					shard_builders[i]->write();
					write_mutexes[i].unlock();
				}
			}

			LOG_INFO("Done " + to_string(idx) + " out of " + to_string(local_files.size()));

			idx++;
		}

		for (size_t i = 0; i < Config::ht_num_shards; i++) {
			write_mutexes[i].lock();
			shard_builders[i]->write();
			write_mutexes[i].unlock();
		}

		for (HashTableShardBuilder *shard_builder : shard_builders) {
			delete shard_builder;
		}

		return counter;
	}

	void sort_hash_table(const string &db_name) {

		LOG_INFO("Sorting...");
		
		// Loop over hash table shards and sort them.
		for (size_t shard_id = 0; shard_id < Config::ht_num_shards; shard_id++) {
			HashTableShardBuilder *shard = new HashTableShardBuilder(db_name, shard_id);
			shard->sort();
			delete shard;
		}
	}

	void run_link_counter(const string &db_name, const string &batch, const vector<string> &local_files, map<size_t, map<size_t, float>> &counter) {

		SubSystem *sub_system = new SubSystem();

		ThreadPool pool(Config::ft_num_threads_indexing);
		std::vector<std::future<map<size_t, map<size_t, float>>>> results;

		vector<vector<string>> chunks;
		Algorithm::vector_chunk<string>(local_files, ceil(local_files.size() / Config::ft_num_threads_indexing) + 1, chunks);

		vector<mutex> write_mutexes(Config::ht_num_shards);

		for (const vector<string> &chunk : chunks) {

			results.emplace_back(
				pool.enqueue([sub_system, db_name, chunk, &write_mutexes] {
					return run_count_thread_with_local_files(sub_system, db_name, chunk, write_mutexes);
				})
			);

		}

		for(auto && result: results) {
			map<size_t, map<size_t, float>> count_part = result.get();
			for (const auto &iter : count_part) {
				counter[iter.first].insert(iter.second.begin(), iter.second.end());
			}
		}

		delete sub_system;

		sort_hash_table(db_name);
	}

	void upload_link_counts_file(const vector<string> &lines, const string &batch, size_t file_num) {
		const string file_data = boost::algorithm::join(lines, "\n");
		Transfer::upload_gz_file("urls/link_counts/" + batch + "/top_" + to_string(file_num) + ".gz", file_data);
	}

	float calculate_score(const map<size_t, float> &scores) {
		float score = 0.0f;
		for (const auto &iter : scores) {
			score += iter.second;
		}
		return score;
	}

	void upload_link_counts(const std::string &db_name, const string &batch, std::map<size_t, std::map<size_t, float>> &counter) {

		struct link_count {
			size_t link_hash;
			size_t count;
			float score;
		};

		vector<struct link_count> counter_vec;

		for (auto &iter : counter) {
			counter_vec.emplace_back(link_count {.link_hash = iter.first, .count = iter.second.size(), .score = calculate_score(iter.second)});
			iter.second.clear();
		}

		counter.clear();

		sort(counter_vec.begin(), counter_vec.end(), [](const struct link_count &a, const struct link_count &b) {
			return a.score > b.score;
		});

		HashTable hash_table(db_name);
		vector<string> file_lines;
		size_t file_num = 0;
		for (const auto &count : counter_vec) {
			const string url = hash_table.find(count.link_hash);

			file_lines.push_back(url + "\t" + to_string(count.count) + "\t" + to_string(count.score));

			if (file_lines.size() >= 1000000) {
				upload_link_counts_file(file_lines, batch, ++file_num);
				file_lines.clear();
			}

		}
		upload_link_counts_file(file_lines, batch, ++file_num);
	}

}
