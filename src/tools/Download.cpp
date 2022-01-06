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
#include "transfer/Transfer.h"
#include "file/TsvFileRemote.h"
#include "system/Profiler.h"
#include "algorithm/Algorithm.h"
#include "link/Link.h"
#include "system/System.h"
#include "config.h"
#include "full_text/FullText.h"

#include <math.h>
#include <unordered_set>
#include <future>

using namespace std;

namespace Tools {

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

			// DEBUG
			if (files_to_download.size() > 100) break;
		}

		return Transfer::download_gz_files_to_disk(files_to_download);
	}

	vector<string> download_links(size_t batch_start, size_t batch_end) {

		vector<string> return_files;
		for (size_t batch = batch_start; batch < batch_end; batch++) {
			File::TsvFileRemote warc_paths_file(string("crawl-data/LINK-") + to_string(batch) + "/warc.paths.gz");
			vector<string> warc_paths;
			warc_paths_file.read_column_into(0, warc_paths);

			// DEBUG
			warc_paths.resize(100);

			vector<string> files = Transfer::download_gz_files_to_disk(warc_paths);
			return_files.insert(return_files.end(), files.begin(), files.end());
		}

		return return_files;
	}

	unordered_set<size_t> make_url_set_one_thread(const vector<string> &files) {

		unordered_set<size_t> result;
		for (const string &file_path : files) {
			ifstream infile(file_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const Link::Link link(line);
				result.insert(link.target_url().hash());
			}
		}

		return result;
	}

	unordered_set<size_t> make_url_set(const vector<string> &files) {
		const size_t num_threads = 24;

		vector<vector<string>> link_thread_input;
		Algorithm::vector_chunk(files, ceil((double)files.size() / num_threads), link_thread_input);

		vector<future<unordered_set<size_t>>> futures;

		for (size_t i = 0; i < link_thread_input.size(); i++) {
			futures.emplace_back(std::async(launch::async, make_url_set_one_thread, link_thread_input[i]));
		}

		unordered_set<size_t> total_result;
		for (auto &fut : futures) {
			unordered_set<size_t> result = fut.get();
			total_result.insert(result.begin(), result.end());
		}

		return total_result;
	}

	void upload_cache(size_t file_index, size_t thread_id, const string &data, size_t node_id) {
		const string filename = "crawl-data/NODE-" + to_string(node_id) + "/files/" + to_string(thread_id) + "-" + to_string(file_index) + "-" +
			to_string(Profiler::now_micro()) + ".gz";

		Transfer::upload_gz_file(filename, data);
	}

	void parse_urls_with_links_thread(const vector<string> &warc_paths, const unordered_set<size_t> url_set) {

		const size_t max_cache_size = 150000;
		size_t thread_id = System::thread_id();
		size_t file_index = 1;

		vector<vector<string>> file_names(Config::nodes_in_cluster);
		vector<vector<string>> cache(Config::nodes_in_cluster);
		for (const string &warc_path : warc_paths) {
			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const URL url(line.substr(0, line.find("\t")));

				if (url_set.count(url.hash())) {
					const size_t node_id = FullText::url_to_node(url);
					cache[node_id].push_back(line);
				}
			}

			for (size_t node_id = 0; node_id < Config::nodes_in_cluster; node_id++) {
				if (cache[node_id].size() > max_cache_size) {
					upload_cache(file_index++, thread_id, boost::algorithm::join(cache[node_id], "\n"), node_id);
				}
			}
		}
		for (size_t node_id = 0; node_id < Config::nodes_in_cluster; node_id++) {
			upload_cache(file_index++, thread_id, boost::algorithm::join(cache[node_id], "\n"), node_id);
		}
	}

	void upload_urls_with_links(const vector<string> &local_files, const unordered_set<size_t> url_set) {
		size_t num_threads = 24;

		vector<vector<string>> thread_input;
		Algorithm::vector_chunk(local_files, ceil((double)local_files.size() / num_threads), thread_input);

		vector<thread> threads;

		for (size_t i = 0; i < num_threads; i++) {
			threads.emplace_back(thread(parse_urls_with_links_thread, thread_input[i], cref(url_set)));
		}

		for (thread &one_thread : threads) {
			one_thread.join();
		}
	}

	void prepare_batch(const string &batch) {

		const vector<string> files = download_batch(batch);

		for (size_t i = 0; i < 4; i++) {
			const size_t start = i * 6;
			const size_t stop = (i + 1) * 6;
			const vector<string> link_files = download_links(start, stop);
			unordered_set<size_t> url_set = make_url_set(link_files);

			upload_urls_with_links(files, url_set);
		}
	}

}

