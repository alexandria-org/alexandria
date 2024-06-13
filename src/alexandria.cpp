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
#include <numeric>
#include "logger/logger.h"
#include "downloader/warc_downloader.h"
#include "downloader/merge_downloader.h"
#include "URL.h"
#include "hash_table2/hash_table.h"
#include "hash_table2/hash_table_shard_builder.h"
#include "indexer/index.h"
#include "indexer/index_builder.h"
#include "indexer/value_record.h"
#include "algorithm/hyper_ball.h"
#include "utils/thread_pool.hpp"
#include "file/file.h"
#include "http/server.h"
#include "parser/parser.h"
#include <boost/algorithm/string.hpp>

using namespace std;

void help() {
	std::string content = file::cat("../documentation/alexandria.md");
	std::cout << content << std::endl;
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
	} else if (arg == "--downloader-missing" && argc > 4) {
		downloader::warc_downloader_missing(string(argv[2]), std::stoull(argv[3]), std::stoull(argv[4]));
	} else if (arg == "--hash-table-url" && argc > 2) {
		URL url(argv[2]);
		hash_table2::hash_table ht("all_urls", 1019, 1000000, "/slow_data");

		size_t ver = 0;
		std::string data = ht.find(url.hash(), ver);
		std::cout << ver << std::endl;
		std::cout << data << std::endl;
	} else if (arg == "--hash-table-url-hash" && argc > 2) {
		uint64_t url_hash = std::stoull(argv[2]);
		hash_table2::hash_table ht("all_urls", 1019, 1000000, "/slow_data");

		size_t ver = 0;
		std::string data = ht.find(url_hash, ver);
		std::cout << ver << std::endl;
		std::cout << data << std::endl;
	} else if (arg == "--hash-table-count") {

		hash_table2::hash_table ht("all_urls", 1019, 1000000, "/slow_data");

		std::cout << ht.size() << std::endl;

	} else if (arg == "--hash-table-find-all" && argc > 2) {

		hash_table2::hash_table ht("all_urls", 1019, 1000000, "/slow_data");

		// Put given hosts in array with hashes to search for.
		std::vector<uint64_t> search_for;
		for (int i = 2; i < argc; i++) {
			search_for.push_back(URL(string("https://") + argv[i]).host_hash());
		}

		ht.for_each([&search_for](uint64_t key, std::string value) {

			URL url(value.substr(0, value.find("\t")));

			const auto my_host_hash = url.host_hash();
			for (const auto &host_hash : search_for) {
				if (host_hash == my_host_hash) {
					std::cout << key << "\t" << url.str() << std::endl;
					break;
				}
			}

		});

	} else if (arg == "--hash-table-count" && argc > 2) {

		std::string data = file::cat("domains.txt");
		std::vector<std::string> lines;
		boost::split(lines, data, boost::is_any_of("\n"));
		std::map<std::string, uint64_t> domains;
		std::map<uint64_t, size_t> domain_counts;
		std::vector<std::string> domain_list;
		for (const auto &line : lines) {
			if (line == "") continue;
			const std::string reversed = URL::host_reverse(line);
			std::cout << reversed << std::endl;
			const uint64_t domain_hash = URL(string("https://") + reversed).host_hash();
			domains[reversed] = domain_hash;
			domain_counts[domain_hash] = 0;
			domain_list.push_back(reversed);
		}

		hash_table2::hash_table ht("all_urls", 1019, 1000000, "/slow_data");

		uint64_t thelazy_host_hash = URL(string("https://") + argv[2]).host_hash();

		ht.for_each([thelazy_host_hash, &domain_counts](uint64_t key, std::string value) {

			URL url(value.substr(0, value.find("\t")));

			const auto my_host_hash = url.host_hash();
			for (auto &iter : domain_counts) {
				if (iter.first == my_host_hash) {
					domain_counts[iter.first]++;
					break;
				}
			}

			/*if (url.host_hash() == thelazy_host_hash) {
				std::cout << key << " => " << url.str() << std::endl;
			}*/

		});

		for (auto &domain : domain_list) {
			std::cout << domain << "\t" << domain_counts[domains[domain]] << std::endl;
		}

	} else if (arg == "--hash-table-optimize-shard" && argc > 2) {
		size_t shard_id = std::stoull(argv[2]);
		hash_table2::hash_table_shard_builder ht_shard("all_urls", shard_id, 1000000, "/slow_data");

		ht_shard.optimize();

	} else if (arg == "--internal-harmonic") {
		profiler::instance prof_total("total");
		/*

		std::vector<std::string> all_files;
		file::read_directory("/mnt/0/full_text/internal_links", [&all_files](const std::string &filename) {
			all_files.push_back(filename);
		});

		size_t done_with = 0;
		profiler::instance prof("total");
		for (const auto &filename : all_files) {

			// Read the file.
			std::ifstream infile("/mnt/0/full_text/internal_links/" + filename, std::ios::binary);
			std::string infile_data(std::istreambuf_iterator<char>(infile), {});
			infile.close();
			std::istringstream reader(infile_data);
			indexer::index<indexer::value_record> idx(&reader, 1000);

			// Create vertices vector
			std::vector<uint64_t> vertices;
			std::map<uint64_t, uint64_t> vertex_map;

			size_t record_id = 0;
			for (const auto &record : idx.records()) {
				vertices.push_back(record.m_value);
				vertex_map[record.m_value] = record_id;
				record_id++;
			}

			std::vector<roaring::Roaring> edge_map(vertices.size());

			// Populate edge map
			idx.for_each([&edge_map, &vertex_map, &vertices, &record_id](uint64_t key, roaring::Roaring &bitmap) {
					if (vertex_map.count(key) == 0) {
						vertices.push_back(key);
						edge_map.push_back(roaring::Roaring());
						vertex_map[key] = record_id;
						record_id++;
					}
					edge_map[vertex_map[key]] = std::move(bitmap);
			});


			// Calculate harmonic centrality on graph.
			if (vertices.size() > 500) {
				auto harmonic = algorithm::hyper_ball(vertices.size(), edge_map.data());
			}

			// Sort the results a bit.
			std::vector<size_t> sorted(harmonic.size());
			std::iota(sorted.begin(), sorted.end(), 0);
			std::sort(sorted.begin(), sorted.end(), [&harmonic] (const auto &a, const auto &b) {
				return harmonic[a] > harmonic[b];
			});

			done_with++;
			float percent = ((float)done_with / all_files.size()) * 100.0f;
			float elapsed_milliseconds = prof.get();
			size_t items_left = all_files.size() - done_with;
			float milliseconds_per_file = elapsed_milliseconds/done_with;
			float milliseconds_left = milliseconds_per_file * items_left;
			float hours_left = milliseconds_left / (1000.0f * 3600.0f);
			std::cout << "done with " << done_with << " out of " << all_files.size() << " (" <<
				percent << "% done) time left: " << hours_left << " hours"<< std::endl;
		}

		return 0;*/

		// load the file
		std::string content = file::cat("multiple_domains.tsv");
		std::vector<std::string> lines;
		boost::split(lines, content, boost::is_any_of("\n"));
		std::vector<std::vector<std::string>> csv_data;
		for (auto line : lines) {
			std::vector<std::string> cols;
			boost::split(cols, line, boost::is_any_of("\t"));
			if (cols.size() > 1) {
				if (URL(cols[1]).host_hash() == URL("http://abc13.com").host_hash()) {
					csv_data.push_back(cols);
				}
			}
		}

		profiler::instance prof_load("load");
		//std::ifstream infile("/mnt/5/full_text/internal_links/3492248666075096845.data", std::ios::binary);
		std::ifstream infile("/mnt/6/full_text/internal_links/12854855988816217414.data", std::ios::binary);
		std::string infile_data(std::istreambuf_iterator<char>(infile), {});
		infile.close();
		std::istringstream reader(infile_data);
		indexer::index<indexer::value_record> idx(&reader, 1000);
		prof_load.stop();

		profiler::instance prof("make vertices");

		std::vector<uint64_t> vertices;
		std::map<uint64_t, uint64_t> vertex_map;

		size_t record_id = 0;
		for (const auto &record : idx.records()) {
			vertices.push_back(record.m_value);
			vertex_map[record.m_value] = record_id;
			record_id++;
		}

		std::vector<roaring::Roaring> edge_map(vertices.size());

		idx.for_each([&edge_map, &vertex_map, &vertices, &record_id](uint64_t key, roaring::Roaring &bitmap) {
				if (vertex_map.count(key) == 0) {
					vertices.push_back(key);
					edge_map.push_back(roaring::Roaring());
					vertex_map[key] = record_id;
					record_id++;
				}
				edge_map[vertex_map[key]] = std::move(bitmap);
		});

		prof.stop();
		profiler::instance prof2("run hyper_ball");

		auto harmonic = algorithm::hyper_ball(vertices.size(), edge_map.data());

		prof2.stop();

		prof_total.stop();

		std::vector<size_t> sorted(harmonic.size());
		std::iota(sorted.begin(), sorted.end(), 0);
		std::sort(sorted.begin(), sorted.end(), [&harmonic] (const auto &a, const auto &b) {
			return harmonic[a] > harmonic[b];
		});
		std::map<uint64_t, double> harmonic_by_url;
		for (size_t i = 0; i < harmonic.size(); i++) {
			harmonic_by_url[vertices[sorted[i]]] = harmonic[sorted[i]] / vertices.size();
		}

		for (auto row : csv_data) {
			uint64_t url_hash = stoull(row[0]);
			double harmonic = harmonic_by_url[url_hash];
			std::cout << row[0] << "\t" << row[1] << "\t" << harmonic << std::endl;
		}

		/*
		profiler::instance prof_load("load");
		//std::ifstream infile("/mnt/5/full_text/internal_links/3492263685688109621.data", std::ios::binary);
		//std::ifstream infile("/mnt/5/full_text/internal_links/3492528524383210893.data", std::ios::binary);
		//std::ifstream infile("/mnt/0/full_text/internal_links/7131549202223940368.data", std::ios::binary);
		std::ifstream infile("/mnt/0/full_text/internal_links/10401139885298228528.data", std::ios::binary);
		std::string infile_data(std::istreambuf_iterator<char>(infile), {});
		infile.close();
		std::istringstream reader(infile_data);
		indexer::index<indexer::value_record> idx(&reader, 1000);
		prof_load.stop();

		profiler::instance prof("make vertices");

		std::vector<uint64_t> vertices;
		std::map<uint64_t, uint64_t> vertex_map;

		size_t record_id = 0;
		for (const auto &record : idx.records()) {
			vertices.push_back(record.m_value);
			vertex_map[record.m_value] = record_id;
			record_id++;
		}

		std::vector<roaring::Roaring> edge_map(vertices.size());

		idx.for_each([&edge_map, &vertex_map, &vertices, &record_id](uint64_t key, roaring::Roaring &bitmap) {
				if (vertex_map.count(key) == 0) {
					vertices.push_back(key);
					edge_map.push_back(roaring::Roaring());
					vertex_map[key] = record_id;
					record_id++;
				}
				edge_map[vertex_map[key]] = std::move(bitmap);
		});

		prof.stop();
		profiler::instance prof2("run hyper_ball");

		auto harmonic = algorithm::hyper_ball(vertices.size(), edge_map.data());

		prof2.stop();

		prof_total.stop();

		std::vector<size_t> sorted(harmonic.size());
		std::iota(sorted.begin(), sorted.end(), 0);
		std::sort(sorted.begin(), sorted.end(), [&harmonic] (const auto &a, const auto &b) {
			return harmonic[a] > harmonic[b];
		});

		//for (size_t i = 0; i < harmonic.size(); i++) {
			//std::cout << "vertex: " << vertices[sorted[i]] << " has harmonic: " << harmonic[sorted[i]] << std::endl;
		//}
		*/
	} else if (arg == "--url-server") {
		// Spin up a simple url server.

		hash_table2::hash_table ht("all_urls", 1019, 1000000, "/slow_data");

		http::server url_server([&ht](auto request) {
			http::response res;

			URL url = request.url();
			auto query = url.query();
			URL find_url(parser::urldecode(query["url"]));

			size_t ver;
			const auto find_str = ht.find(find_url.hash(), ver);

			if (find_str == "") {
				res.code(404);
				res.body("Not found 404");
			} else {
				res.code(200);
				res.body(find_str);
			}

			return res;
		});
	} else {
		help();
	}

	logger::join_logger_thread();

	return 0;
}
