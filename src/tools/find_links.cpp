
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

using namespace std;

namespace tools {

	void find_links_for_hosts_chunk(const set<size_t> &host_hashes, const vector<string> &files) {

		size_t thread_id = common::thread_id();
		size_t links_written = 0;
		const size_t links_per_file = 1000000;

		ofstream outfile;

		outfile.open("/mnt/crawl-data/SMALL-LINK-MIX/files/" + to_string(thread_id) + "_" + to_string(links_written) + "-" +
			to_string(links_written + links_per_file) + ".gz", ios::binary);

		boost::iostreams::filtering_ostream compress_stream;
		compress_stream.push(boost::iostreams::gzip_compressor());
		compress_stream.push(outfile);

		for (auto file : files) {
			ifstream infile("/mnt/" + file);

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				vector<string> col_values;
				boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

				const size_t host_hash = algorithm::hash(col_values[2]);

				if (host_hashes.find(host_hash) != host_hashes.end()) {

					// Write link to current file.

					compress_stream << line << "\n";
					links_written++;
					if ((links_written % links_per_file) == 0) {
						cout << "writing file" << endl;
						compress_stream.strict_sync();
						compress_stream.pop();
						outfile.close();
						outfile.open("/mnt/crawl-data/SMALL-LINK-MIX/files/" + to_string(thread_id) + "_" +
							to_string(links_written) + "-" + to_string(links_written + links_per_file) + ".gz",
							ios::binary);
						compress_stream.push(outfile);
					}
				}
			}
		}
	}

	void find_links_for_hosts(const set<size_t> &host_hashes) {
		const string batch = "LINK-MIX";
		const size_t num_threads = 12;
		size_t limit = 1000;

		file::gz_tsv_file batch_file("/mnt/crawl-data/"+batch+"/warc.paths.gz");

		vector<string> rows;
		batch_file.read_column_into(0, rows);

		if (rows.size() > limit) rows.resize(limit);

		vector<vector<string>> chunks;
		algorithm::vector_chunk<string>(rows, ceil(rows.size() / num_threads) + 1, chunks);

		utils::thread_pool threads(num_threads);

		for (auto chunk : chunks) {
			threads.enqueue([&host_hashes, chunk]() {
				find_links_for_hosts_chunk(host_hashes, chunk);
			});
		}

		threads.run_all();
	}

	void find_links() {
		const string batch = "SMALL-MIX";
		size_t limit = 10;

		file::gz_tsv_file batch_file("/mnt/crawl-data/"+batch+"/warc.paths.gz");

		vector<string> rows;
		batch_file.read_column_into(0, rows);

		if (rows.size() > limit) rows.resize(limit);

		// Load all the host hashes into a set
		set<size_t> host_hashes;

		for (auto row : rows) {
			ifstream infile("/mnt/" + row);

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				vector<string> col_values;
				boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

				URL url(col_values[0]);

				host_hashes.insert(url.host_hash());
			}
		}

		cout << "found " << host_hashes.size() << " hosts" << endl;

		find_links_for_hosts(host_hashes);
	}


}
