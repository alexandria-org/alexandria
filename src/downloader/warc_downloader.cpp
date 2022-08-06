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

using namespace std;

namespace downloader {

	void run_downloader(const string &warc_path, utils::id_allocator<indexer::index_builder<indexer::value_record>> &internal_link_allocator,
			std::unordered_map<uint64_t, indexer::index_builder<indexer::value_record> *> &internal_link_cache, hash_table2::builder &ht) {

		std::string all_links;

		warc::parser pp;
		warc::multipart_download("http://data.commoncrawl.org/" + warc_path, [&pp, &ht, &internal_link_cache, &internal_link_allocator, &all_links](const string &chunk) {
			stringstream ss(chunk);
			pp.parse_stream(ss, [&ht, &internal_link_cache, &internal_link_allocator, &all_links](const string &url_str, const parser::html_parser &html, const std::string &ip, const std::string &date) {
					URL url(url_str);
					std::tm t = {};
					std::istringstream ss(date);
					ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%SZ");
					size_t time = (t.tm_year + 1900) * 10000000000ull + (t.tm_mon + 1) * 100000000ull + (t.tm_mday) * 1000000ull + (t.tm_hour) * 10000ull + (t.tm_min) * 100ull + t.tm_sec;

					uint64_t host_hash = url.host_hash();
					if (!internal_link_cache.count(host_hash)) {
						internal_link_cache[host_hash] = internal_link_allocator.get(host_hash, "internal_links", host_hash, 1000);
					}
					auto internal_link_builder = internal_link_cache[host_hash];

					const std::string data = (url.str()
						+ '\t' + html.title()
						+ '\t' + html.h1()
						+ '\t' + html.meta()
						+ '\t' + html.text()
						+ '\t' + date
						+ '\t' + ip
						+ '\n');

					ht.add(url.hash(), data, time);

					for (const auto &link : html.links()) {
						all_links += (link.host()
							+ '\t' + link.path()
							+ '\t' + link.target_host()
							+ '\t' + link.target_path()
							+ '\t' + link.text()
							+ '\t' + (link.nofollow() ? "1" : "0")
							+ '\n');
					}

					for (const auto &link : html.internal_links()) {
						// link is a std::pair<uint64_t, uint64_t> link_from -> link_to
						// but we store the internal links as link_to -> link_from because the hyper_ball algorithm requires it.
						// see src/algorithm/hyper_ball.h
						internal_link_builder->add(link.second, indexer::value_record(link.first));
					}
			});
		});

		LOG_INFO("uploading: " + warc_path);
		int error;
		//error = transfer::upload_gz_file(warc::get_result_path(warc_path), pp.result());
		error = transfer::upload_gz_file(warc::get_link_result_path(warc_path), all_links);

		if (error) {
			LOG_INFO("error uploading: " + warc_path);
		}

	}

	vector<string> download_warc_paths() {
		int error;
		string content = transfer::file_to_string("nodes/" + config::node + "/warc.paths", error);
		if (error == transfer::ERROR) return {};

		content = text::trim(content);

		vector<string> raw_warc_paths;
		boost::algorithm::split(raw_warc_paths, content, boost::is_any_of("\n"));

		vector<string> warc_paths;
		for (const string &warc_path : raw_warc_paths) {
			if (text::trim(warc_path).size()) {
				warc_paths.push_back(text::trim(warc_path));
			}
		}

		return warc_paths;
	}

	bool upload_warc_paths(const vector<string> &warc_paths) {
		string content = boost::algorithm::join(warc_paths, "\n");
		int error = transfer::upload_file("nodes/" + config::node + "/warc.paths", content);
		return error == transfer::OK;
	}

	void start_downloaders(const std::vector<std::string> &warc_paths) {

		indexer::delete_db_directories("internal_links");
		indexer::create_db_directories("internal_links");

		indexer::merger::set_mem_limit(0.1);
		indexer::merger::start_merge_thread();

		const size_t num_threads = 24;

		std::vector<std::vector<std::string>> chunks;
		algorithm::vector_chunk<std::string>(warc_paths, std::ceil(warc_paths.size() / num_threads) + 1, chunks);

		utils::thread_pool pool(num_threads);

		hash_table2::builder ht("crawl_index", 1019);
		ht.truncate();
		utils::id_allocator<indexer::index_builder<indexer::value_record>> internal_link_allocator;

		for (const auto &chunk : chunks) {
			pool.enqueue([chunk, &ht, &internal_link_allocator] {
				std::unordered_map<uint64_t, indexer::index_builder<indexer::value_record> *> internal_link_cache;
				size_t count = 0;
				for (const auto &warc_path : chunk) {
					run_downloader(warc_path, internal_link_allocator, internal_link_cache, ht);
					count++;
					std::cout << "done with " << warc_path << " done with " << count << "/" << chunk.size() << std::endl;
				}
			});
		}

		pool.run_all();

		indexer::merger::stop_merge_thread();
	}

	void upload_all() {

		std::string upload_id = std::to_string(common::cur_datetime());

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

			const std::string filename = "internal_links_" + std::to_string(i);
			file::archive tar(filename);
			tar.read_dir(config::data_path() + "/" + std::to_string(i) + "/full_text/internal_links");

			transfer::upload_file_from_disk("downloader/" + config::node + "/" + upload_id + "/" + filename, filename);

			file::delete_file(filename);
		}

		hash_table2::hash_table ht("crawl_index", 1019);
		ht.for_each_shard([upload_id](auto shard) {

			const std::string pos_filename = shard->filename_pos();
			const std::string data_filename = shard->filename_data();
			const std::string target_filename = std::to_string(shard->shard_id());

			transfer::upload_file_from_disk("downloader/" + config::node + "/" + upload_id + "/ht/" + target_filename + ".pos", pos_filename);
			transfer::upload_file_from_disk("downloader/" + config::node + "/" + upload_id + "/ht/" + target_filename + ".data", data_filename);
		});

	}

	void warc_downloader(const std::string &batch, size_t limit, size_t offset) {

		std::vector<std::string> warc_paths;

		int error;
		string content = transfer::gz_file_to_string("https://data.commoncrawl.org/crawl-data/" + batch + "/warc.paths.gz", error);

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

		upload_all();
	}
}

