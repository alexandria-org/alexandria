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

#include <iostream>
#include <sstream>
#include "file/file.h"
#include "file/archive.h"
#include "hash_table2/builder.h"
#include "utils/thread_pool.hpp"
#include "indexer/index.h"
#include "indexer/index_builder.h"
#include "indexer/index_reader.h"
#include "indexer/value_record.h"

namespace downloader {

	bool internal_links_complete(const std::string &path) {

		for (size_t i = 0; i < 8; i++) {
			if (!file::file_exists(path + "/internal_links_" + std::to_string(i))) {
				return false;
			}
		}

		return true;
	}

	bool hash_table_complete(const std::string &path) {
		const size_t num_shards = 1019;
		for (size_t i = 0; i < num_shards; i++) {
			if (!file::file_exists(path + "/" + std::to_string(i) + ".pos")) {
				return false;
			}
		}
		for (size_t i = 0; i < num_shards; i++) {
			if (!file::file_exists(path + "/" + std::to_string(i) + ".data")) {
				return false;
			}
		}

		return true;
	}

	void merge_internal_links(const std::string &path, const std::string &batch_name) {
		return;
		/*
		const std::string target_path = "/slow_data/internal_links/" + batch_name;
		file::create_directory(target_path);
		for (size_t i = 0; i < 8; i++) {
			file::copy_file(path + "/internal_links_" + std::to_string(i), target_path + "/internal_links_" + std::to_string(i));
		}
		*/
		utils::thread_pool pool(8);
		for (size_t i = 0; i < 8; i++) {
			pool.enqueue([i, path]() {
				file::archive tar(path + "/internal_links_" + std::to_string(i));
				utils::thread_pool pool(4, 10);
				tar.untar([&pool](const std::string &filename, const std::string &data) {

					pool.enqueue([filename, data]() {
						uint64_t host_hash = std::stoull(filename.substr(0, filename.size() - 5));

						std::istringstream ram_reader(data);

						indexer::index_builder<indexer::value_record> idx1("internal_links", host_hash, 1000);
						indexer::index<indexer::value_record> idx2(&ram_reader, 1000);

						try {
							idx1.merge_with(idx2);
						} catch (const std::runtime_error &err) {
							// The file is corrupt. Lets delete it and report.
							std::cout << "internal_links: " << host_hash << " is corrupt" << std::endl;
							idx1.truncate();
						} catch (const std::bad_alloc &err) {
							// The file is corrupt. Lets delete it and report.
							std::cout << "internal_links: " << host_hash << " is corrupt" << std::endl;
							idx1.truncate();
						}
					});
				});
				pool.run_all();
			});
		}
		pool.run_all();
		std::cout << "finished with the merge" << std::endl;
	}

	void merge_hash_table(const std::string &path) {
		utils::thread_pool pool(32);
		hash_table2::builder ht("all_urls", 1019, 1000000, "/slow_data");
		for (size_t i = 0; i < 1019; i++) {
			pool.enqueue([&ht, i, path]() {
				ht.get_shard(i)->merge_with(path + "/" + std::to_string(i) + ".pos", path + "/" + std::to_string(i) + ".data");
			});
		}
		pool.run_all();
	}

	void merge_downloader() {

		indexer::index_builder<indexer::value_record>::create_directories("internal_links");

		file::read_directory(config::data_path() + "/downloader", [](const std::string &node_id) {
			const std::string dir = config::data_path() + "/downloader/" + node_id;
			file::read_directory(dir, [dir](const std::string &file) {
				try {
					size_t ts = std::stoull(file);
					const std::string batch = dir + "/" + std::to_string(ts);
					if (internal_links_complete(batch) && hash_table_complete(batch + "/ht")) {
						std::cout << "merging directory: " << batch << std::endl;
						profiler::instance prof1("merge_internal_links");
						merge_internal_links(batch, std::to_string(ts));
						prof1.stop();
						profiler::instance prof2("merge_hash_table");
						merge_hash_table(batch + "/ht");
						prof2.stop();
						file::delete_directory(batch);
						exit(0);
					}
				} catch (...) {
				}
			});
		});
	}
}

