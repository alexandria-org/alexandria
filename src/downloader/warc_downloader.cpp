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

#include <iomanip>

#include "config.h"
#include "common/datetime.h"
#include "warc/warc.h"
#include "utils/thread_pool.hpp"
#include "utils/id_allocator.h"
#include "file/archive.h"
#include "logger/logger.h"
#include "text/text.h"
#include "transfer/transfer.h"
#include <iostream>
#include "hash_table2/builder.h"
#include "algorithm/algorithm.h"
#include "indexer/index_utils.h"
#include "indexer/index_builder.h"
#include "indexer/value_record.h"
#include "indexer/merger.h"

namespace downloader {

	void run_downloader(const std::string &warc_path) {

		warc::parser pp;
		for (int retry = 0; retry < 3; retry++) {
			try {
				warc::multipart_download("http://data.commoncrawl.org/" + warc_path, [&pp](const std::string &chunk) {
					std::stringstream ss(chunk);
					pp.parse_stream(ss);
				});
				break;
			} catch (const std::runtime_error &err) {
				std::cout << "GOT ERROR: " << err.what() << std::endl;
				std::cout << "Retrying... try " << retry << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(5));
			}
		}

		LOG_INFO("uploading: " + warc_path);
		int error;
		error = transfer::upload_gz_file(warc::get_result_path(warc_path), pp.result());
		error = transfer::upload_gz_file(warc::get_link_result_path(warc_path), pp.link_result());

		if (error) {
			LOG_INFO("error uploading: " + warc_path);
		}

	}

	std::vector<std::string> download_warc_paths() {
		int error;
		auto content = transfer::file_to_string("nodes/" + config::node + "/warc.paths", error);
		if (error == transfer::ERROR) return {};

		content = text::trim(content);

		std::vector<std::string> raw_warc_paths;
		boost::algorithm::split(raw_warc_paths, content, boost::is_any_of("\n"));

		std::vector<std::string> warc_paths;
		for (const auto &warc_path : raw_warc_paths) {
			if (text::trim(warc_path).size()) {
				warc_paths.push_back(text::trim(warc_path));
			}
		}

		return warc_paths;
	}

	bool upload_warc_paths(const std::vector<std::string> &warc_paths) {
		auto content = boost::algorithm::join(warc_paths, "\n");
		int error = transfer::upload_file("nodes/" + config::node + "/warc.paths", content);
		return error == transfer::OK;
	}

	void start_downloaders(const std::vector<std::string> &warc_paths) {

		const size_t num_threads = 12;

		std::vector<std::vector<std::string>> chunks;
		algorithm::vector_chunk<std::string>(warc_paths, std::ceil(warc_paths.size() / num_threads) + 1, chunks);

		utils::thread_pool pool(num_threads);

		for (const auto &chunk : chunks) {
			pool.enqueue([chunk] {
				size_t count = 0;
				for (const auto &warc_path : chunk) {
					run_downloader(warc_path);
					count++;
					std::cout << "done with " << warc_path << " done with " << count << "/" << chunk.size() << std::endl;
				}
			});
		}

		pool.run_all();
	}

	void upload_all() {

		/*auto upload_id = std::to_string(common::cur_datetime());

		// Upload internal links.
		for (size_t i = 0; i < 8; i++) {

			// Optimize all internal links.
			utils::thread_pool pool(32);
			file::read_directory(config::data_path() + "/" + std::to_string(i) + "/full_text/internal_links", [&pool](const std::string &filename) {
				uint64_t host_hash = std::stoull(filename.substr(0, filename.size() - 5));
				indexer::index_builder<indexer::value_record> idx("internal_links", host_hash, 1000);
				idx.optimize();
			});
			pool.run_all();

			const auto filename = "internal_links_" + std::to_string(i);
			file::archive tar(filename);
			tar.read_dir(config::data_path() + "/" + std::to_string(i) + "/full_text/internal_links");

			transfer::upload_file_from_disk("downloader/" + config::node + "/" + upload_id + "/" + filename, filename);

			file::delete_file(filename);
		}

		hash_table2::hash_table ht("crawl_index", 1019);
		ht.for_each_shard([upload_id](auto shard) {

			const auto pos_filename = shard->filename_pos();
			const auto data_filename = shard->filename_data();
			const auto target_filename = std::to_string(shard->shard_id());

			transfer::upload_file_from_disk("downloader/" + config::node + "/" + upload_id + "/ht/" + target_filename + ".pos", pos_filename);
			transfer::upload_file_from_disk("downloader/" + config::node + "/" + upload_id + "/ht/" + target_filename + ".data", data_filename);
		});
		*/

	}

	void warc_downloader_with_url(const std::string &batch, size_t limit, size_t offset, const std::string &warc_paths_url) {
	
		std::vector<std::string> warc_paths;

		int error;
		auto content = transfer::gz_file_to_string(warc_paths_url, error);

		std::stringstream ss(content);

		std::string line;
		size_t line_num = 0;
		while (std::getline(ss, line)) {
			if (line_num >= offset) {
				warc_paths.emplace_back(std::move(line));
			}

			if (warc_paths.size() >= limit) break;
			line_num++;
		}

		start_downloaders(warc_paths);
	}

	void warc_downloader(const std::string &batch, size_t limit, size_t offset) {
		warc_downloader_with_url(batch, limit, offset, "https://data.commoncrawl.org/crawl-data/" + batch + "/warc.paths.gz");
	}

	void warc_downloader_missing(const std::string &batch, size_t limit, size_t offset) {
		warc_downloader_with_url(batch, limit, offset, "crawl-data/" + batch + "/missing.paths.gz");
	}
}

