
#include "find_links.h"
#include "file/gz_tsv_file.h"
#include "URL.h"
#include "algorithm/algorithm.h"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <vector>
#include <set>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/algorithm/string.hpp>
#include <math.h>
#include "utils/thread_pool.hpp"
#include "algorithm/hash.h"
#include "common/system.h"
#include "config.h"

namespace tools {

	void find_links_for_hosts_chunk(const std::set<size_t> &host_hashes, const std::vector<std::string> &files) {

		size_t thread_id = common::thread_id();
		size_t links_written = 0;
		const size_t links_per_file = 1000000;

		std::ofstream outfile;

		outfile.open(config::data_path() + "/crawl-data/SMALL-LINK-MIX/files/" + std::to_string(thread_id) + "_" + std::to_string(links_written) + "-" +
			std::to_string(links_written + links_per_file) + ".gz", std::ios::binary);

		boost::iostreams::filtering_ostream compress_stream;
		compress_stream.push(boost::iostreams::gzip_compressor());
		compress_stream.push(outfile);

		for (auto file : files) {
			std::ifstream infile(config::data_path() + "/" + file);

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			std::string line;
			while (getline(decompress_stream, line)) {
				std::vector<std::string> col_values;
				boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

				const size_t host_hash = algorithm::hash(col_values[2]);

				if (host_hashes.find(host_hash) != host_hashes.end()) {

					// Write link to current file.

					compress_stream << line << "\n";
					links_written++;
					if ((links_written % links_per_file) == 0) {
						std::cout << "writing file" << std::endl;
						compress_stream.strict_sync();
						compress_stream.pop();
						outfile.close();
						outfile.open(config::data_path() + "/crawl-data/SMALL-LINK-MIX/files/" + std::to_string(thread_id) +
							"_" + std::to_string(links_written) + "-" + std::to_string(links_written + links_per_file) + ".gz",
							std::ios::binary);
						compress_stream.push(outfile);
					}
				}
			}
		}
	}

	void find_links_for_hosts(const std::set<size_t> &host_hashes) {
		const std::string batch = "LINK-MIX";
		const size_t num_threads = 12;
		size_t limit = 4000;

		file::gz_tsv_file batch_file(config::data_path() + "/crawl-data/" + batch + "/warc.paths.gz");

		std::vector<std::string> rows;
		batch_file.read_column_into(0, rows);

		if (rows.size() > limit) rows.resize(limit);

		std::vector<std::vector<std::string>> chunks;
		algorithm::vector_chunk<std::string>(rows, ceil(rows.size() / num_threads) + 1, chunks);

		utils::thread_pool threads(num_threads);

		for (auto chunk : chunks) {
			threads.enqueue([&host_hashes, chunk]() {
				find_links_for_hosts_chunk(host_hashes, chunk);
			});
		}

		threads.run_all();
	}

	void find_links() {
		const auto batch = "SMALL-MIX";
		size_t limit = 20;

		file::gz_tsv_file batch_file(config::data_path() + "/crawl-data/"+batch+"/warc.paths.gz");

		std::vector<std::string> rows;
		batch_file.read_column_into(0, rows);

		if (rows.size() > limit) rows.resize(limit);

		// Load all the host hashes into a set
		std::set<size_t> host_hashes;

		for (auto row : rows) {
			std::ifstream infile(config::data_path() + "/" + row);

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			std::string line;
			while (getline(decompress_stream, line)) {
				std::vector<std::string> col_values;
				boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

				URL url(col_values[0]);

				host_hashes.insert(url.host_hash());
			}
		}

		std::cout << "found " << host_hashes.size() << " hosts" << std::endl;

		find_links_for_hosts(host_hashes);
	}


}
