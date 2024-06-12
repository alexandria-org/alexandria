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

#include "download.h"
#include "transfer/transfer.h"
#include "file/tsv_file_remote.h"
#include "profiler/profiler.h"
#include "algorithm/algorithm.h"
#include "url_link/link.h"
#include "common/system.h"
#include "config.h"
#include "logger/logger.h"

#include <math.h>
#include <unordered_set>
#include <future>

namespace tools {

	const size_t max_num_batches = 24;

	std::vector<std::string> download_batch(const std::string &batch) {

		file::tsv_file_remote warc_paths_file(std::string("crawl-data/") + batch + "/warc.paths.gz");
		std::vector<std::string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		std::vector<std::string> files_to_download;
		for (const std::string &str : warc_paths) {
			std::string warc_path = str;
			const size_t pos = warc_path.find(".warc.gz");
			if (pos != std::string::npos) {
				warc_path.replace(pos, 8, ".gz");
			}
			files_to_download.push_back(warc_path);

			//if (files_to_download.size() == 100) break;
		}

		return transfer::download_gz_files_to_disk(files_to_download);
	}

	std::vector<std::string> download_missing(const std::string &batch) {

		file::tsv_file_remote warc_paths_file(std::string("crawl-data/") + batch + "/missing.paths.gz");
		std::vector<std::string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		std::vector<std::string> files_to_download;
		for (const std::string &str : warc_paths) {
			std::string warc_path = str;
			const size_t pos = warc_path.find(".warc.gz");
			if (pos != std::string::npos) {
				warc_path.replace(pos, 8, ".gz");
			}
			files_to_download.push_back(warc_path);

			//if (files_to_download.size() == 100) break;
		}

		return transfer::download_gz_files_to_disk(files_to_download);
	}

	std::unordered_set<size_t> make_url_set_one_thread(const std::vector<std::string> &files) {

		std::unordered_set<size_t> result;
		for (const std::string &file_path : files) {
			std::ifstream infile(file_path);
			std::string line;
			while (getline(infile, line)) {
				const url_link::link link(line);
				result.insert(link.target_url().hash());
			}
		}

		return result;
	}

	std::unordered_set<size_t> make_url_set(const std::vector<std::string> &files) {

		std::unordered_set<size_t> total_result;
		size_t idx = 0;
		for (const std::string &file_path : files) {
			std::ifstream infile(file_path);
			std::string line;
			while (getline(infile, line)) {
				const url_link::link link(line);
				total_result.insert(link.target_url().hash());
			}
			std::cout << "size: " << total_result.size() << " done " << (++idx) << "/" << files.size() << std::endl;
		}

		return total_result;
	}

	void upload_cache(size_t file_index, size_t thread_id, const std::string &data, size_t node_id) {
		const std::string filename = "crawl-data/NODE-" + std::to_string(node_id) + "-small/files/" + std::to_string(thread_id) + "-" + std::to_string(file_index) + "-" +
			std::to_string(profiler::now_micro()) + ".gz";

		int error = transfer::upload_gz_file(filename, data);
		if (error == transfer::ERROR) {
			LOG_INFO("Upload failed!");
		}
	}

	void parse_urls_with_links_thread(const std::vector<std::string> &warc_paths, const std::unordered_set<size_t> &url_set) {

		const size_t max_cache_size = 150000;
		size_t thread_id = common::thread_id();
		size_t file_index = 1;

		LOG_INFO("url_set.size() == " + std::to_string(url_set.size()));

		std::vector<std::vector<std::string>> cache(max_num_batches);
		for (const std::string &warc_path : warc_paths) {
			std::ifstream infile(warc_path);

			std::string line;
			while (getline(infile, line)) {
				const URL url(line.substr(0, line.find("\t")));

				if (url_set.count(url.hash())) {
					const size_t node_id = url.host_hash() % max_num_batches;
					cache[node_id].push_back(line);
				}
			}

			for (size_t node_id = 0; node_id < max_num_batches; node_id++) {
				if (cache[node_id].size() > max_cache_size) {
					const std::string cache_data = boost::algorithm::join(cache[node_id], "\n");
					cache[node_id].clear();
					upload_cache(file_index++, thread_id, cache_data, node_id);
				}
			}
		}
		for (size_t node_id = 0; node_id < max_num_batches; node_id++) {
			if (cache[node_id].size() > 0) {
				const std::string cache_data = boost::algorithm::join(cache[node_id], "\n");
				cache[node_id].clear();
				upload_cache(file_index++, thread_id, cache_data, node_id);
			}
		}
	}

	void upload_urls_with_links(const std::vector<std::string> &local_files, const std::unordered_set<size_t> &url_set) {
		size_t num_threads = 24;

		std::vector<std::vector<std::string>> thread_input;
		algorithm::vector_chunk(local_files, ceil((double)local_files.size() / num_threads), thread_input);

		std::vector<std::thread> threads;

		for (size_t i = 0; i < thread_input.size(); i++) {
			threads.emplace_back(std::thread(parse_urls_with_links_thread, thread_input[i], cref(url_set)));
		}

		for (std::thread &one_thread : threads) {
			one_thread.join();
		}
	}

	void prepare_batch(size_t batch_num) {

		const std::vector<std::string> files = download_batch("NODE-" + std::to_string(batch_num));
		const std::vector<std::string> link_files = download_batch("LINK-" + std::to_string(batch_num));

		std::unordered_set<size_t> url_set = make_url_set(link_files);
		upload_urls_with_links(files, url_set);

		transfer::delete_downloaded_files(link_files);
		transfer::delete_downloaded_files(files);
	}

}

