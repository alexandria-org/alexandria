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

#include "Download.h"
#include "transfer/transfer.h"
#include "file/TsvFileRemote.h"
#include "system/Profiler.h"
#include "algorithm/algorithm.h"
#include "link/Link.h"
#include "system/System.h"
#include "config.h"
#include "full_text/FullText.h"

#include <math.h>
#include <unordered_set>
#include <future>

using namespace std;

namespace Tools {

	const size_t max_num_batches = 24;

	vector<string> download_batch(const string &batch) {

		File::TsvFileRemote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
		vector<string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		vector<string> files_to_download;
		for (const string &str : warc_paths) {
			string warc_path = str;
			const size_t pos = warc_path.find(".warc.gz");
			if (pos != string::npos) {
				warc_path.replace(pos, 8, ".gz");
			}
			files_to_download.push_back(warc_path);

			//if (files_to_download.size() == 100) break;
		}

		return transfer::download_gz_files_to_disk(files_to_download);
	}

	unordered_set<size_t> make_url_set_one_thread(const vector<string> &files) {

		unordered_set<size_t> result;
		for (const string &file_path : files) {
			ifstream infile(file_path);
			string line;
			while (getline(infile, line)) {
				const Link::Link link(line);
				result.insert(link.target_url().hash());
			}
		}

		return result;
	}

	unordered_set<size_t> make_url_set(const vector<string> &files) {

		unordered_set<size_t> total_result;
		size_t idx = 0;
		for (const string &file_path : files) {
			ifstream infile(file_path);
			string line;
			while (getline(infile, line)) {
				const Link::Link link(line);
				total_result.insert(link.target_url().hash());
			}
			cout << "size: " << total_result.size() << " done " << (++idx) << "/" << files.size() << endl;
		}

		return total_result;
	}

	void upload_cache(size_t file_index, size_t thread_id, const string &data, size_t node_id) {
		const string filename = "crawl-data/NODE-" + to_string(node_id) + "-small/files/" + to_string(thread_id) + "-" + to_string(file_index) + "-" +
			to_string(Profiler::now_micro()) + ".gz";

		int error = transfer::upload_gz_file(filename, data);
		if (error == transfer::ERROR) {
			LOG_INFO("Upload failed!");
		}
	}

	void parse_urls_with_links_thread(const vector<string> &warc_paths, const unordered_set<size_t> &url_set) {

		const size_t max_cache_size = 150000;
		size_t thread_id = System::thread_id();
		size_t file_index = 1;

		LOG_INFO("url_set.size() == " + to_string(url_set.size()));

		vector<vector<string>> cache(max_num_batches);
		for (const string &warc_path : warc_paths) {
			ifstream infile(warc_path);

			string line;
			while (getline(infile, line)) {
				const URL url(line.substr(0, line.find("\t")));

				if (url_set.count(url.hash())) {
					const size_t node_id = url.host_hash() % max_num_batches;
					cache[node_id].push_back(line);
				}
			}

			for (size_t node_id = 0; node_id < max_num_batches; node_id++) {
				if (cache[node_id].size() > max_cache_size) {
					const string cache_data = boost::algorithm::join(cache[node_id], "\n");
					cache[node_id].clear();
					upload_cache(file_index++, thread_id, cache_data, node_id);
				}
			}
		}
		for (size_t node_id = 0; node_id < max_num_batches; node_id++) {
			if (cache[node_id].size() > 0) {
				const string cache_data = boost::algorithm::join(cache[node_id], "\n");
				cache[node_id].clear();
				upload_cache(file_index++, thread_id, cache_data, node_id);
			}
		}
	}

	void upload_urls_with_links(const vector<string> &local_files, const unordered_set<size_t> &url_set) {
		size_t num_threads = 24;

		vector<vector<string>> thread_input;
		algorithm::vector_chunk(local_files, ceil((double)local_files.size() / num_threads), thread_input);

		vector<thread> threads;

		for (size_t i = 0; i < thread_input.size(); i++) {
			threads.emplace_back(thread(parse_urls_with_links_thread, thread_input[i], cref(url_set)));
		}

		for (thread &one_thread : threads) {
			one_thread.join();
		}
	}

	void prepare_batch(size_t batch_num) {

		const vector<string> files = download_batch("NODE-" + to_string(batch_num));
		const vector<string> link_files = download_batch("LINK-" + to_string(batch_num));

		unordered_set<size_t> url_set = make_url_set(link_files);
		upload_urls_with_links(files, url_set);

		transfer::delete_downloaded_files(link_files);
		transfer::delete_downloaded_files(files);
	}

}

