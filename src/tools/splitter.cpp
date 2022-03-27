
#include "splitter.h"
#include "config.h"
#include <iostream>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <cmath>
#include <thread>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/filesystem.hpp>
#include "url_link/link.h"
#include "full_text/full_text.h"
#include "algorithm/algorithm.h"
#include "URL.h"
#include "common/system.h"

using namespace std;

namespace tools {

	// File structure is /mnt/crawl-data/NODE-[node_id]/files/thread_id-file_index.gz
	string write_cache(size_t file_index, size_t thread_id, vector<string> &lines, size_t node_id) {
		const string filename = "crawl-data/NODE-" + to_string(node_id) + "/files/" + to_string(thread_id) + "-" + to_string(file_index) + ".gz";
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

	// File structure is /mnt/crawl-data/NODE-[node_id]/files/thread_id-file_index.gz
	string write_link_cache(size_t file_index, size_t thread_id, vector<string> &lines, size_t node_id) {
		const string filename = "crawl-data/LINK-" + to_string(node_id) + "/files/" + to_string(thread_id) + "-" + to_string(file_index) + ".gz";
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

		const size_t max_cache_size = 150000;
		size_t thread_id = common::thread_id();
		size_t file_index = 1;

		vector<vector<string>> file_names(config::nodes_in_cluster);
		vector<vector<string>> cache(config::nodes_in_cluster);
		for (const string &warc_path : warc_paths) {
			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const URL url(line.substr(0, line.find("\t")));
				const size_t node_id = full_text::url_to_node(url);
				cache[node_id].push_back(line);
			}

			for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
				if (cache[node_id].size() > max_cache_size) {
					file_names[node_id].push_back(write_cache(file_index++, thread_id, cache[node_id], node_id));
				}
			}
		}
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			file_names[node_id].push_back(write_cache(file_index++, thread_id, cache[node_id], node_id));
		}

		write_file_mutex.lock();
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			const string filename = "/mnt/crawl-data/NODE-" + to_string(node_id) + "/warc.paths";
			ofstream outfile(filename, ios::app);
			for (const string &file : file_names[node_id]) {
				outfile << file << "\n";
			}
		}
		write_file_mutex.unlock();
	}

	void link_splitter(const vector<string> &warc_paths, mutex &write_file_mutex) {

		const size_t max_cache_size = 1000000;
		size_t thread_id = common::thread_id();
		size_t file_index = 1;

		vector<vector<string>> file_names(config::nodes_in_cluster);
		vector<vector<string>> cache(config::nodes_in_cluster);
		size_t done = 0;
		for (const string &warc_path : warc_paths) {

			cout << "done " << done << "/" << warc_paths.size() << endl;
			done++;

			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const url_link::link link(line);
				const size_t node_id = full_text::link_to_node(link);
				cache[node_id].push_back(line);
			}

			for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
				if (cache[node_id].size() > max_cache_size) {
					file_names[node_id].push_back(write_link_cache(file_index++, thread_id, cache[node_id], node_id));
				}
			}
		}
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			file_names[node_id].push_back(write_link_cache(file_index++, thread_id, cache[node_id], node_id));
		}

		write_file_mutex.lock();
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			const string filename = "/mnt/crawl-data/LINK-" + to_string(node_id) + "/warc.paths";
			ofstream outfile(filename, ios::app);
			for (const string &file : file_names[node_id]) {
				outfile << file << "\n";
			}
		}
		write_file_mutex.unlock();
	}

	void splitter_with_urls(const unordered_set<size_t> &urls, const vector<string> &warc_paths, mutex &write_file_mutex) {

		const size_t max_cache_size = 150000;
		size_t thread_id = common::thread_id();
		size_t file_index = 1;

		vector<vector<string>> file_names(config::nodes_in_cluster);
		vector<vector<string>> cache(config::nodes_in_cluster);
		size_t idx = 0;
		for (const string &warc_path : warc_paths) {
			cout << warc_path << endl;
			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const URL url(line.substr(0, line.find("\t")));
				if (urls.count(url.hash())) {
					const size_t node_id = full_text::url_to_node(url);
					cache[node_id].push_back(line);
				}
			}

			for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
				if (cache[node_id].size() > max_cache_size) {
					file_names[node_id].push_back(write_cache(file_index++, thread_id, cache[node_id], node_id));
				}
			}

			if (idx % 100 == 0) {
				cout << warc_path << " done " << idx << "/" << warc_paths.size() << endl;
			} 
			idx++;
		}
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			file_names[node_id].push_back(write_cache(file_index++, thread_id, cache[node_id], node_id));
		}

		write_file_mutex.lock();
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			const string filename = "/mnt/crawl-data/NODE-" + to_string(node_id) + "/warc.paths";
			ofstream outfile(filename, ios::app);
			for (const string &file : file_names[node_id]) {
				outfile << file << "\n";
			}
		}
		write_file_mutex.unlock();
	}

	unordered_set<size_t> build_link_set(const vector<string> &warc_paths, size_t hash_min, size_t hash_max) {

		unordered_set<size_t> result;
		for (const string &warc_path : warc_paths) {
			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const url_link::link link(line);
				const size_t hash = link.target_url().hash();
				if (hash >= hash_min && hash <= hash_max) {
					result.insert(hash);
				}
			}
		}

		return result;
	}

	void create_warc_directories() {
		// Create directories.
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			boost::filesystem::create_directories("/mnt/crawl-data/NODE-" + to_string(node_id));
			boost::filesystem::create_directories("/mnt/crawl-data/NODE-" + to_string(node_id) + "/files");
		}
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			boost::filesystem::create_directories("/mnt/crawl-data/LINK-" + to_string(node_id));
			boost::filesystem::create_directories("/mnt/crawl-data/LINK-" + to_string(node_id) + "/files");
		}
	}

	void run_splitter() {

		tools::create_warc_directories();

		const size_t num_threads = 12;

		vector<thread> threads;
		vector<string> files;
		vector<string> link_files;

		for (const string &batch : config::batches) {

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

		for (const string &batch : config::link_batches) {

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
					warc_path.replace(pos, 8, ".links.gz");
				}

				link_files.push_back(warc_path);
			}
		}

		vector<vector<string>> thread_input;
		algorithm::vector_chunk(files, ceil((double)files.size() / num_threads), thread_input);

		vector<vector<string>> link_thread_input;
		algorithm::vector_chunk(link_files, ceil((double)link_files.size() / num_threads), link_thread_input);

		mutex write_file_mutex;

		/*
		Run splitter threads
		*/
		for (size_t i = 0; i < thread_input.size(); i++) {
			threads.emplace_back(thread(splitter, thread_input[i], ref(write_file_mutex)));
		}

		for (thread &one_thread : threads) {
			one_thread.join();
		}

		threads.clear();

		/*
		Run link_splitter threads
		for (size_t i = 0; i < link_thread_input.size(); i++) {
			threads.emplace_back(thread(link_splitter, link_thread_input[i], ref(write_file_mutex)));
		}

		for (thread &one_thread : threads) {
			one_thread.join();
		}
		*/
	}

	void run_url_splitter_on_urls_in_set(const unordered_set<size_t> &urls) {

		tools::create_warc_directories();

		const size_t num_threads = 12;

		vector<thread> threads;
		vector<string> files;
		for (const string &batch : config::batches) {

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

				if (files.size() > 100) break;
			}
		}

		vector<vector<string>> thread_input;
		algorithm::vector_chunk(files, ceil((double)files.size() / num_threads), thread_input);

		mutex write_file_mutex;

		/*
		Run splitter threads
		*/
		for (size_t i = 0; i < thread_input.size(); i++) {
			threads.emplace_back(thread(splitter_with_urls, cref(urls), cref(thread_input[i]), ref(write_file_mutex)));
		}

		for (thread &one_thread : threads) {
			one_thread.join();
		}

	}

	void run_splitter_with_links_interval(size_t hash_min, size_t hash_max) {

		cout << "running run_splitter_with_links_interval with hash_min: " << hash_min << " hash_max: " << hash_max << endl;

		const size_t num_threads = 12;

		vector<string> link_files;
		for (const string &batch : config::link_batches) {

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
					warc_path.replace(pos, 8, ".links.gz");
				}

				link_files.push_back(warc_path);

				break;
			}
		}

		vector<vector<string>> link_thread_input;
		algorithm::vector_chunk(link_files, ceil((double)link_files.size() / num_threads), link_thread_input);

		vector<future<unordered_set<size_t>>> futures;

		for (size_t i = 0; i < link_thread_input.size(); i++) {
			futures.emplace_back(std::async(launch::async, build_link_set, link_thread_input[i], hash_min, hash_max));
		}

		unordered_set<size_t> total_result;
		for (auto &fut : futures) {
			unordered_set<size_t> result = fut.get();
			total_result.insert(result.begin(), result.end());
		}

		cout << "size: " << total_result.size() << endl;

		run_url_splitter_on_urls_in_set(total_result);
	}

	void run_splitter_with_links() {
		const size_t chunk = (SIZE_MAX >> 3) + 1;
		for (size_t i = 0; i < (1ull << 3); i++) {
			run_splitter_with_links_interval(chunk * i, chunk * (i + 1) - 1);
		}
	}


}
