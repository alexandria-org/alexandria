
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <thread>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include "full_text/FullText.h"
#include "algorithm/Algorithm.h"
#include "parser/URL.h"
#include "system/System.h"
#include "config.h"

using namespace std;

// File structure is /mnt/crawl-data/ALEXANDRIA-NODE-[node_id]/files/thread_id-file_index.gz
string write_cache(size_t file_index, size_t thread_id, vector<string> &lines, size_t node_id) {
	const string filename = "crawl-data/ALEXANDRIA-NODE-" + to_string(node_id) + "/files/" + to_string(thread_id) + "-" + to_string(file_index) + ".gz";
	ofstream outfile("/mnt/" + filename, ios::trunc | ios::binary);

	boost::iostreams::filtering_ostream compress_stream;
	compress_stream.push(boost::iostreams::gzip_compressor());
	compress_stream.push(outfile);

	for (const string &line : lines) {
		compress_stream << line << "\n";
	}
	lines.clear();
	return filename;
}

void splitter(const vector<string> &warc_paths, mutex &write_file_mutex) {

	const size_t max_cache_size = 250000;
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
			const uint64_t hash = url.hash();
			const size_t node_id = FullText::hash_to_node(hash);
			cache[node_id].push_back(line);
		}

		for (size_t node_id = 0; node_id < Config::nodes_in_cluster; node_id++) {
			if (cache[node_id].size() > max_cache_size) {
				file_names[node_id].push_back(write_cache(file_index++, thread_id, cache[node_id], node_id));
			}
		}
	}
	for (size_t node_id = 0; node_id < Config::nodes_in_cluster; node_id++) {
		file_names[node_id].push_back(write_cache(file_index++, thread_id, cache[node_id], node_id));
	}

	write_file_mutex.lock();
	for (size_t node_id = 0; node_id < Config::nodes_in_cluster; node_id++) {
		const string filename = "crawl-data/ALEXANDRIA-NODE-" + to_string(node_id) + "/warc.paths";
		ofstream outfile(filename, ios::app);
		for (const string &file : file_names[node_id]) {
			outfile << file << "\n";
		}
	}
	write_file_mutex.unlock();
}

int main() {

	Config::read_config("/etc/alexandria.conf");

	const size_t num_threads = 12;

	vector<string> batches = {
		"CC-MAIN-2021-31",
		"CC-MAIN-2021-25",
	};

	vector<thread> threads;
	vector<string> files;

	for (const string &batch : batches) {

		const string file_name = string("/mnt/crawl-data/") + batch + "/warc.paths.gz";

		ifstream infile(file_name);

		boost::iostreams::filtering_istream decompress_stream;
		decompress_stream.push(boost::iostreams::gzip_decompressor());
		decompress_stream.push(infile);

		string line;
		while (getline(decompress_stream, line)) {
			string warc_path = string("/mnt/") + line;
			const size_t pos = warc_path.find(".warc.gz");
			if (pos != string::npos) {
				warc_path.replace(pos, 8, ".gz");
			}

			files.push_back(warc_path);
		}
	}

	vector<vector<string>> thread_input;
	Algorithm::vector_chunk(files, ceil((double)files.size() / num_threads), thread_input);

	mutex write_file_mutex;
	for (size_t i = 0; i < num_threads; i++) {
		threads.emplace_back(thread(splitter, thread_input[i], ref(write_file_mutex)));
	}

	for (thread &one_thread : threads) {
		one_thread.join();
	}

	return 0;
}
