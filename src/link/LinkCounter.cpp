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

	void count_links_in_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream, map<size_t, set<size_t>> &counter) {

		string line;
		while (getline(stream, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			const string link_text = col_values[4].substr(0, 1000);
			URL target_url(col_values[2], col_values[3]);
			URL source_url(col_values[0], col_values[1]);

			const uint64_t link_hash = source_url.link_hash(target_url, link_text);
			const uint64_t target_url_hash = target_url.hash();

			counter[target_url_hash].insert(link_hash);
			shard_builders[target_url_hash % Config::ht_num_shards]->add(target_url_hash, target_url.str());
		}
	}

	map<size_t, set<size_t>> run_count_thread_with_local_files(const string &db_name, const vector<string> &local_files,
			vector<mutex> &write_mutexes) {

		map<size_t, set<size_t>> counter;

		vector<HashTableShardBuilder *> shard_builders;
		for (size_t i = 0; i < Config::ht_num_shards; i++) {
			shard_builders.push_back(new HashTableShardBuilder(db_name, i));
		}

		size_t idx = 1;
		for (const string &local_file : local_files) {

			ifstream stream(local_file, ios::in);

			if (stream.is_open()) {
				{
					count_links_in_stream(shard_builders, stream, counter);
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

	void run_link_counter(const string &db_name, const string &batch, const vector<string> &local_files, map<size_t, set<size_t>> &counter) {

		ThreadPool pool(Config::ft_num_threads_indexing);
		std::vector<std::future<map<size_t, set<size_t>>>> results;

		vector<vector<string>> chunks;
		Algorithm::vector_chunk<string>(local_files, ceil(local_files.size() / Config::ft_num_threads_indexing) + 1, chunks);

		vector<mutex> write_mutexes(Config::ht_num_shards);

		for (const vector<string> &chunk : chunks) {

			results.emplace_back(
				pool.enqueue([db_name, chunk, &write_mutexes] {
					return run_count_thread_with_local_files(db_name, chunk, write_mutexes);
				})
			);

		}

		for(auto && result: results) {
			map<size_t, set<size_t>> count_part = result.get();
			for (const auto &iter : count_part) {
				counter[iter.first].insert(iter.second.begin(), iter.second.end());
			}
		}

		sort_hash_table(db_name);
	}

	void upload_link_counts_file(const vector<string> &lines, size_t file_num) {
		const string file_data = boost::algorithm::join(lines, "\n");
		Transfer::upload_gz_file("urls/link_counts/top_" + to_string(file_num) + ".gz", file_data);
	}

	void upload_link_counts(const std::string &db_name, std::map<size_t, std::set<size_t>> &counter) {

		struct link_count {
			size_t link_hash;
			size_t count;
		};

		vector<struct link_count> counter_vec;

		for (auto &iter : counter) {
			counter_vec.emplace_back(link_count {.link_hash = iter.first, .count = iter.second.size()});
			iter.second.clear();
		}

		counter.clear();

		sort(counter_vec.begin(), counter_vec.end(), [](const struct link_count &a, const struct link_count &b) {
			return a.count > b.count;
		});

		HashTable hash_table(db_name);
		vector<string> file_lines;
		size_t file_num = 0;
		for (const auto &count : counter_vec) {
			const string url = hash_table.find(count.link_hash);

			file_lines.push_back(url + "\t" + to_string(count.count));

			if (file_lines.size() >= 1000000) {
				upload_link_counts_file(file_lines, ++file_num);
				file_lines.clear();
			}

		}
		upload_link_counts_file(file_lines, ++file_num);
	}

}
