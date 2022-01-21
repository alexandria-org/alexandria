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
#include "KeyValueStore.h"
#include "parser/URL.h"
#include "transfer/Transfer.h"
#include "system/Logger.h"
#include "system/Profiler.h"
#include "urlstore/UrlStore.h"
#include "algorithm/Algorithm.h"
#include "config.h"
#include <vector>
#include <mutex>
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

using namespace std;

namespace Link {

	void count_links_in_stream(const SubSystem *sub_system, size_t sub_batch, KeyValueStore &kv_store, basic_istream<char> &stream,
			map<string, map<size_t, float>> &counter) {

		string line;
		//map<size_t, string> small_cache;
		while (getline(stream, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL target_url(col_values[2], col_values[3]);
			const uint64_t target_url_hash = target_url.hash();
			if (target_url_hash % 5 == sub_batch) {

				const string link_text = col_values[4].substr(0, 1000);
				URL source_url(col_values[0], col_values[1]);

				const uint64_t link_hash = Hash::str(source_url.host_top_domain());
				float harmonic = source_url.harmonic(sub_system);

				counter[target_url.str()][link_hash] = expm1(25.0*harmonic) / 50.0;
			}
			/*small_cache[target_url_hash] = target_url.str();

			if (small_cache.size() > 1000000) {
				leveldb::WriteBatch batch;
				for (const auto &iter : small_cache) {
					//kv_store.set(to_string(iter.first), iter.second);
					batch.Put(to_string(iter.first), iter.second);
				}
				kv_store.db()->Write(leveldb::WriteOptions(), &batch);
				small_cache.clear();
			}*/
		}

		/*leveldb::WriteBatch batch;
		for (const auto &iter : small_cache) {
			//kv_store.set(to_string(iter.first), iter.second);
			batch.Put(to_string(iter.first), iter.second);
		}
		kv_store.db()->Write(leveldb::WriteOptions(), &batch);
		*/
	}

	map<string, map<size_t, float>> run_count_thread_with_local_files(const SubSystem *sub_system, size_t sub_batch,
			const vector<string> &local_files, KeyValueStore &kv_store) {

		map<string, map<size_t, float>> counter;

		size_t idx = 1;
		for (const string &local_file : local_files) {

			ifstream stream(local_file, ios::in);

			if (stream.is_open()) {
				{
					count_links_in_stream(sub_system, sub_batch, kv_store, stream, counter);
				}
			}

			stream.close();

			LOG_INFO("Done " + to_string(idx) + " out of " + to_string(local_files.size()) + " on sub_batch = " + to_string(sub_batch));

			idx++;
		}

		return counter;
	}

	void run_link_counter(const SubSystem *sub_system, const string &batch, size_t sub_batch, const vector<string> &local_files,
			map<string, map<size_t, float>> &counter) {

		ThreadPool pool(Config::ft_num_threads_indexing);
		std::vector<std::future<map<string, map<size_t, float>>>> results;

		vector<vector<string>> chunks;
		Algorithm::vector_chunk<string>(local_files, ceil(local_files.size() / Config::ft_num_threads_indexing) + 1, chunks);

		KeyValueStore kv_store("/mnt/0/tmp_kwstore");

		for (const vector<string> &chunk : chunks) {

			results.emplace_back(
				pool.enqueue([sub_system, sub_batch, chunk, &kv_store] {
					return run_count_thread_with_local_files(sub_system, sub_batch, chunk, kv_store);
				})
			);

		}

		for (auto && result: results) {
			map<string, map<size_t, float>> count_part = result.get();
			for (const auto &iter : count_part) {
				counter[iter.first].insert(iter.second.begin(), iter.second.end());
			}
		}
	}

	void upload_link_counts_file(const vector<string> &lines, const string &batch, size_t sub_batch, size_t file_num) {
		const string file_data = boost::algorithm::join(lines, "\n");
		Transfer::upload_gz_file("urls/link_counts/" + batch + "/"+to_string(sub_batch)+"/top_" + to_string(file_num) + ".gz", file_data);
	}

	float calculate_score(const map<size_t, float> &scores) {
		float score = 0.0f;
		for (const auto &iter : scores) {
			score += iter.second;
		}
		return score;
	}

	void upload_link_counts(const string &batch, size_t sub_batch, std::map<string, std::map<size_t, float>> &counter) {

		//KeyValueStore kv_store("/mnt/0/tmp_kwstore");
		//LOG_INFO("Compacting /mnt/0/tmp_kwstore");
		//kv_store.db()->CompactRange(nullptr, nullptr);

		struct link_count {
			string url;
			size_t count;
			float score;
		};

		vector<struct link_count> counter_vec;

		for (auto &iter : counter) {
			counter_vec.emplace_back(link_count {.url = iter.first, .count = iter.second.size(),
					.score = calculate_score(iter.second)});
			iter.second.clear();
		}

		counter.clear();

		sort(counter_vec.begin(), counter_vec.end(), [](const struct link_count &a, const struct link_count &b) {
			return a.score > b.score;
		});

		vector<string> file_lines;
		vector<UrlStore::UrlData> url_data;
		size_t file_num = 0;
		for (const auto &count : counter_vec) {
			//const string url = kv_store.get(to_string(count.link_hash));

			file_lines.push_back(count.url + "\t" + to_string(count.count) + "\t" + to_string(count.score));
			url_data.emplace_back(UrlStore::UrlData{
				.url = URL(count.url),
				.redirect = URL(),
				.link_count = count.count,
				.http_code = 0,
				.last_visited = 0
			});

			if (file_lines.size() >= 1000000) {
				Profiler::instance prof1("Uploading results");
				file_num++;
				thread th1(upload_link_counts_file, file_lines, batch, sub_batch, file_num);
				thread th2([&url_data] (){ UrlStore::set_deferred(url_data); });
				th1.join();
				th2.join();
				file_lines.clear();
				url_data.clear();
			}

		}
		upload_link_counts_file(file_lines, batch, sub_batch, ++file_num);
		UrlStore::set(url_data);
	}

}
