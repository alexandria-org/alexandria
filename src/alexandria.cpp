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
#include <numeric>
#include "logger/logger.h"
#include "downloader/warc_downloader.h"
#include "downloader/merge_downloader.h"
#include "URL.h"
#include "hash_table2/hash_table.h"
#include "indexer/index.h"
#include "indexer/index_builder.h"
#include "indexer/value_record.h"
#include "algorithm/hyper_ball.h"
#include "utils/thread_pool.hpp"
#include "file/file.h"

using namespace std;

void help() {
	cout << "Usage: ./alexandria [OPTION]..." << endl;
	cout << "--downloader [commoncrawl-batch] [limit] [offset]" << endl;
	cout << "--downloader-merge" << endl;
	cout << "--invert-all-internal" << endl;
	cout << "--url [URL]" << endl;
}

int main(int argc, const char **argv) {

	logger::start_logger_thread();
	logger::verbose(true);

	if (getenv("ALEXANDRIA_CONFIG") != NULL) {
		config::read_config(getenv("ALEXANDRIA_CONFIG"));
	} else {
		config::read_config("/etc/alexandria.conf");
	}

	if (argc < 2) {
		help();
		return 0;
	}

	const string arg(argc > 1 ? argv[1] : "");

	if (arg == "--downloader" && argc > 4) {
		downloader::warc_downloader(argv[2], std::stoull(argv[3]), std::stoull(argv[4]));
	} else if (arg == "--downloader-merge") {
		downloader::merge_downloader();
	} else if (arg == "--url" && argc > 2) {
		URL url(argv[2]);
		hash_table2::hash_table ht("all_urls", 1019);

		size_t ver = 0;
		std::string data = ht.find(url.hash(), ver);
		std::cout << ver << std::endl;
		std::cout << data << std::endl;
	} else if (arg == "--invert-all-internal") {

		utils::thread_pool pool(32);

		for (size_t i = 0; i < 8; i++) {
			const std::string dir = "/mnt/" + std::to_string(i) + "/full_text/internal_links";
			file::read_directory(dir, [&pool, dir](const std::string &filename) {
				uint64_t host_hash = std::stoull(filename.substr(0, filename.size() - 5));

				pool.enqueue([host_hash, filename, dir]() {
					std::ifstream infile(dir + "/" + filename, std::ios::binary);
					std::string data(std::istreambuf_iterator<char>(infile), {});
					infile.close();
					std::istringstream data_stream(data);

					std::cout << "data: " << data.size() << std::endl;

					indexer::index<indexer::value_record> idx(&data_stream, 1000);
					indexer::index_builder<indexer::value_record> builder("internal_links", host_hash, 1000);
					builder.truncate();
					
					idx.for_each([&idx, &builder](uint64_t key, roaring::Roaring &bitmap) {
						for (uint32_t x : bitmap) {
							std::cout << key << " => " << idx.records()[x].m_value << std::endl;
							builder.add(idx.records()[x].m_value, indexer::value_record(key));
						}
					});
					builder.append();
					builder.merge();
					builder.optimize();
				});
			});
		}

		pool.run_all();

	} else if (arg == "--internal-harmonic") {
		std::ifstream infile("../3492248666075096845.data", std::ios::binary);
		indexer::index<indexer::value_record> idx(&infile, 1000);

		std::vector<uint64_t> vertices;
		std::map<uint64_t, uint64_t> vertex_map;

		size_t record_id = 0;
		for (const auto &record : idx.records()) {
			vertices.push_back(record.m_value);
			vertex_map[record.m_value] = record_id;
			record_id++;
		}

		std::vector<roaring::Roaring> edge_map(vertices.size());

		idx.for_each([&edge_map, &vertex_map, &vertices](uint64_t key, roaring::Roaring &bitmap) {
				if (vertex_map.count(key)) {
					edge_map[vertex_map[key]] = std::move(bitmap);
				}
		});

		auto harmonic = algorithm::hyper_ball(vertices.size(), edge_map.data());

		std::vector<size_t> sorted(harmonic.size());
		std::iota(sorted.begin(), sorted.end(), 0);
		std::sort(sorted.begin(), sorted.end(), [&harmonic] (const auto &a, const auto &b) {
			return harmonic[a] > harmonic[b];
		});

		for (size_t i = 0; i < harmonic.size(); i++) {
			std::cout << "vertex: " << vertices[sorted[i]] << " has harmonic: " << harmonic[sorted[i]] << std::endl;
		}
	} else {
		help();
	}

	logger::join_logger_thread();

	return 0;
}
