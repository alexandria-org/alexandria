
#include "splitter.h"
#include "config.h"
#include <iostream>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <cmath>
#include <thread>
#include <future>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/filesystem.hpp>
#include "url_link/link.h"
#include "algorithm/algorithm.h"
#include "URL.h"
#include "common/system.h"

namespace tools {

	const std::string s_suffix = "-small";
	const size_t s_num_threads = 12;

	std::vector<std::string> target_url_batches() {
		std::vector<std::string> batches;
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			batches.push_back("NODE-" + std::to_string(node_id) + s_suffix);
		}

		return batches;
	}

	std::vector<std::string> target_link_batches() {
		std::vector<std::string> batches;
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			batches.push_back("LINK-" + std::to_string(node_id) + s_suffix);
		}

		return batches;
	}

	std::vector<std::string> generate_list_with_files(const std::vector<std::string> &batches, const std::string &suffix = ".gz", const std::string &warc_paths_suffix = ".gz") {

		std::vector<std::string> file_names;
		for (const auto &batch : batches) {

			const std::string file_name = config::data_path() + "/crawl-data/" + batch + "/warc.paths" + warc_paths_suffix;

			std::ifstream infile(file_name);

			if (warc_paths_suffix == ".gz") {

				boost::iostreams::filtering_istream decompress_stream;
				decompress_stream.push(boost::iostreams::gzip_decompressor());
				decompress_stream.push(infile);

				std::string line;
				while (getline(decompress_stream, line)) {
					std::string warc_path = config::data_path() + "/" + line;
					const size_t pos = warc_path.find(".warc.gz");

					if (pos != std::string::npos) {
						warc_path.replace(pos, 8, suffix);
					}

					file_names.push_back(warc_path);
				}
			} else {
				std::string line;
				while (getline(infile, line)) {
					std::string warc_path = config::data_path() + "/" + line;
					const size_t pos = warc_path.find(".warc.gz");

					if (pos != std::string::npos) {
						warc_path.replace(pos, 8, suffix);
					}

					file_names.push_back(warc_path);
				}
			}
		}

		return file_names;
	}

	std::vector<std::string> generate_list_with_url_files() {

		// create a list with .gz files that contains urls
		return generate_list_with_files(config::batches, ".gz");

	}

	std::vector<std::string> generate_list_with_link_files() {

		// create a list with .gz files that contains links
		return generate_list_with_files(config::link_batches, ".links.gz");

	}

	std::vector<std::string> generate_list_with_target_url_files() {

		// create a list with .gz files that contains urls
		return generate_list_with_files(target_url_batches(), "", "");

	}

	std::vector<std::string> generate_list_with_target_link_files() {

		// create a list with .gz files that contains links
		return generate_list_with_files(target_link_batches(), "", "");

	}

	// File structure is [data_path]/crawl-data/NODE-[node_id]/files/thread_id-file_index.gz
	std::string write_cache(size_t file_index, size_t thread_id, std::vector<std::string> &lines, size_t node_id) {

		const std::string filename = "crawl-data/NODE-" + std::to_string(node_id) + s_suffix + "/files/" + std::to_string(thread_id) + "-" + std::to_string(file_index) + ".gz";
		std::ofstream outfile(config::data_path() + "/" + filename, std::ios::trunc | std::ios::binary);

		boost::iostreams::filtering_ostream compress_stream;
		compress_stream.push(boost::iostreams::gzip_compressor());
		compress_stream.push(outfile);

		for (const std::string &line : lines) {
			compress_stream << line << "\n";
		}
		lines.clear();
		return filename;
	}

	// File structure is [DATA_PATH]/crawl-data/NODE-[node_id]/files/thread_id-file_index.gz
	std::string write_link_cache(size_t file_index, size_t thread_id, std::vector<std::string> &lines, size_t node_id) {

		const std::string filename = "crawl-data/LINK-" + std::to_string(node_id) + s_suffix + "/files/" + std::to_string(thread_id) + "-" + std::to_string(file_index) + ".gz";
		std::ofstream outfile(config::data_path() + "/" + filename, std::ios::trunc | std::ios::binary);

		boost::iostreams::filtering_ostream compress_stream;
		compress_stream.push(boost::iostreams::gzip_compressor());
		compress_stream.push(outfile);

		for (const std::string &line : lines) {
			compress_stream << line << "\n";
		}
		lines.clear();
		return filename;
	}

	void splitter(const std::vector<std::string> &warc_paths, std::mutex &write_file_mutex) {

		const size_t max_cache_size = 10000;
		size_t thread_id = common::thread_id();
		size_t file_index = 1;

		using vec2d_str = std::vector<std::vector<std::string>>;

		vec2d_str file_names(config::nodes_in_cluster);
		vec2d_str cache(config::nodes_in_cluster);
		for (const std::string &warc_path : warc_paths) {
			std::ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			std::string line;
			while (getline(decompress_stream, line)) {
				const URL url(line.substr(0, line.find("\t")));
				const size_t node_id = url.index_on_node();
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
			const std::string filename = config::data_path() + "/crawl-data/NODE-" + std::to_string(node_id) + s_suffix + "/warc.paths";
			std::ofstream outfile(filename, std::ios::app);
			for (const std::string &file : file_names[node_id]) {
				outfile << file << "\n";
			}
		}
		write_file_mutex.unlock();
	}

	void link_splitter(const std::vector<std::string> &warc_paths, std::mutex &write_file_mutex) {

		const size_t max_cache_size = 1000000;
		size_t thread_id = common::thread_id();
		size_t file_index = 1;
		
		using vec2d_str = std::vector<std::vector<std::string>>;

		vec2d_str file_names(config::nodes_in_cluster);
		vec2d_str cache(config::nodes_in_cluster);
		size_t done = 0;
		for (const std::string &warc_path : warc_paths) {

			std::cout << "done " << done << "/" << warc_paths.size() << std::endl;
			done++;

			std::ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			std::string line;
			while (getline(decompress_stream, line)) {
				const url_link::link link(line);
				const size_t node_id = link.index_on_node();
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
			const auto filename = config::data_path() + "/crawl-data/LINK-" + std::to_string(node_id) + s_suffix + "/warc.paths";
			std::ofstream outfile(filename, std::ios::app);
			for (const std::string &file : file_names[node_id]) {
				outfile << file << "\n";
			}
		}
		write_file_mutex.unlock();
	}

	void link_splitter_with_hosts(const std::unordered_set<size_t> &hosts, const std::vector<std::string> &warc_paths, std::mutex &write_file_mutex) {

		const size_t max_cache_size = 1000000;
		size_t thread_id = common::thread_id();
		size_t file_index = 1;
		
		using vec2d_str = std::vector<std::vector<std::string>>;

		vec2d_str file_names(config::nodes_in_cluster);
		vec2d_str cache(config::nodes_in_cluster);
		size_t done = 0;
		for (const std::string &warc_path : warc_paths) {

			std::cout << "done " << done << "/" << warc_paths.size() << std::endl;
			done++;

			std::ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			std::string line;
			while (getline(decompress_stream, line)) {
				const url_link::link link(line);
				const auto target_host = link.target_host_hash();
				if (hosts.count(target_host)) {
					const size_t node_id = link.index_on_node();
					cache[node_id].push_back(line);
				}
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
			const auto filename = config::data_path() + "/crawl-data/LINK-" + std::to_string(node_id) + s_suffix + "/warc.paths";
			std::ofstream outfile(filename, std::ios::app);
			for (const std::string &file : file_names[node_id]) {
				outfile << file << "\n";
			}
		}
		write_file_mutex.unlock();
	}

	void splitter_with_urls(const std::unordered_set<size_t> &urls, const std::vector<std::string> &warc_paths, std::mutex &write_file_mutex) {

		const size_t max_cache_size = 150000;
		size_t thread_id = common::thread_id();
		size_t file_index = 1;

		std::vector<std::vector<std::string>> file_names(config::nodes_in_cluster);
		std::vector<std::vector<std::string>> cache(config::nodes_in_cluster);
		size_t idx = 0;
		for (const std::string &warc_path : warc_paths) {
			std::cout << warc_path << std::endl;
			std::ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			std::string line;
			while (getline(decompress_stream, line)) {
				const URL url(line.substr(0, line.find("\t")));
				if (urls.count(url.hash())) {
					const size_t node_id = url.index_on_node();
					cache[node_id].push_back(line);
				}
			}

			for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
				if (cache[node_id].size() > max_cache_size) {
					file_names[node_id].push_back(write_cache(file_index++, thread_id, cache[node_id], node_id));
				}
			}

			if (idx % 100 == 0) {
				std::cout << warc_path << " done " << idx << "/" << warc_paths.size() << std::endl;
			} 
			idx++;
		}
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			file_names[node_id].push_back(write_cache(file_index++, thread_id, cache[node_id], node_id));
		}

		write_file_mutex.lock();
		for (size_t node_id = 0; node_id < config::nodes_in_cluster; node_id++) {
			const std::string filename = config::data_path() + "/crawl-data/NODE-" + std::to_string(node_id) + s_suffix + "/warc.paths";
			std::ofstream outfile(filename, std::ios::app);
			for (const std::string &file : file_names[node_id]) {
				outfile << file << "\n";
			}
		}
		write_file_mutex.unlock();
	}

	std::unordered_set<size_t> build_link_set(const std::vector<std::string> &warc_paths, size_t hash_min, size_t hash_max) {

		std::unordered_set<size_t> result;
		for (const std::string &warc_path : warc_paths) {
			std::ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			std::string line;
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

	/*
	 * Input is a vector with paths to url files. Returns an unordered set with all the host hashes.
	 * */
	std::unordered_set<size_t> build_url_host_set(const std::vector<std::string> &warc_paths) {

		std::unordered_set<size_t> hosts;
		for (const std::string &warc_path : warc_paths) {
			std::ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			std::string line;
			while (getline(decompress_stream, line)) {
				const URL url(line.substr(0, line.find("\t")));
				hosts.insert(url.host_hash());
			}
		}

		return hosts;
	}

	void create_warc_directories() {
		// Create directories.
		for (const auto &batch : target_url_batches()) {
			boost::filesystem::create_directories(config::data_path() + "/crawl-data/" + batch);
			boost::filesystem::create_directories(config::data_path() + "/crawl-data/" + batch + "/files");
		}
		for (const auto &batch : target_link_batches()) {
			boost::filesystem::create_directories(config::data_path() + "/crawl-data/" + batch);
			boost::filesystem::create_directories(config::data_path() + "/crawl-data/" + batch + "/files");
		}
	}

	void run_splitter() {

		tools::create_warc_directories();

		std::vector<std::thread> threads;
		auto files = generate_list_with_url_files();
		auto link_files = generate_list_with_link_files();

		std::vector<std::vector<std::string>> thread_input;
		algorithm::vector_chunk(files, ceil((double)files.size() / s_num_threads), thread_input);

		std::vector<std::vector<std::string>> link_thread_input;
		algorithm::vector_chunk(link_files, ceil((double)link_files.size() / s_num_threads), link_thread_input);

		std::mutex write_file_mutex;

		/*
		Run splitter threads
		*/
		for (size_t i = 0; i < thread_input.size(); i++) {
			threads.emplace_back(std::thread(splitter, thread_input[i], ref(write_file_mutex)));
		}

		for (std::thread &one_thread : threads) {
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

	void run_url_splitter_on_urls_in_set(const std::unordered_set<size_t> &urls) {

		tools::create_warc_directories();

		std::vector<std::thread> threads;
		auto files = generate_list_with_url_files();

		std::vector<std::vector<std::string>> thread_input;
		algorithm::vector_chunk(files, ceil((double)files.size() / s_num_threads), thread_input);

		std::mutex write_file_mutex;

		/*
		Run splitter threads
		*/
		for (size_t i = 0; i < thread_input.size(); i++) {
			threads.emplace_back(std::thread(splitter_with_urls, std::cref(urls), std::cref(thread_input[i]), ref(write_file_mutex)));
		}

		for (std::thread &one_thread : threads) {
			one_thread.join();
		}

	}

	void run_link_splitter_on_links_with_target_host_in_set(const std::unordered_set<size_t> &hosts) {

		tools::create_warc_directories();

		std::vector<std::thread> threads;
		auto files = generate_list_with_link_files();

		std::vector<std::vector<std::string>> thread_input;
		algorithm::vector_chunk(files, ceil((double)files.size() / s_num_threads), thread_input);

		std::mutex write_file_mutex;

		/*
		Run splitter threads
		*/
		for (size_t i = 0; i < thread_input.size(); i++) {
			threads.emplace_back(std::thread(link_splitter_with_hosts, std::cref(hosts), std::cref(thread_input[i]), ref(write_file_mutex)));
		}

		for (std::thread &one_thread : threads) {
			one_thread.join();
		}

	}

	void run_split_urls_with_direct_links_interval(size_t hash_min, size_t hash_max) {

		std::cout << "running run_split_urls_with_direct_links_interval with hash_min: " << hash_min << " hash_max: " << hash_max << std::endl;

		auto link_files = generate_list_with_link_files();

		std::cout << "num_files: " << link_files.size() << std::endl;

		// create an unordered set that contains target url hashes of all the links. this will be a huge set.
		std::unordered_set<size_t> total_result;

		std::vector<std::vector<std::string>> link_thread_input;
		algorithm::vector_chunk(link_files, ceil((double)link_files.size() / s_num_threads), link_thread_input);

		std::vector<std::future<std::unordered_set<size_t>>> futures;

		for (size_t i = 0; i < link_thread_input.size(); i++) {
			futures.emplace_back(std::async(std::launch::async, build_link_set, link_thread_input[i], hash_min, hash_max));
		}

		for (auto &fut : futures) {
			auto result = fut.get();
			total_result.insert(result.begin(), result.end());
		}

		std::cout << "size: " << total_result.size() << std::endl;

		run_url_splitter_on_urls_in_set(total_result);
	}

	void run_split_urls_with_direct_links() {
		const size_t chunk = (SIZE_MAX >> 3) + 1;
		for (size_t i = 0; i < (1ull << 3); i++) {
			run_split_urls_with_direct_links_interval(chunk * i, chunk * (i + 1) - 1);
		}
	}

	void run_split_links_with_relevant_domains() {

		auto url_files = generate_list_with_target_url_files();

		// create an unordered set that contains host hashes of all the urls.
		std::cout << "building host hashes map" << std::endl;
		std::unordered_set<size_t> host_hashes;

		std::vector<std::vector<std::string>> thread_input;
		algorithm::vector_chunk(url_files, ceil((double)url_files.size() / s_num_threads), thread_input);

		std::vector<std::future<std::unordered_set<size_t>>> futures;

		for (size_t i = 0; i < thread_input.size(); i++) {
			futures.emplace_back(std::async(std::launch::async, build_url_host_set, thread_input[i]));
		}

		for (auto &fut : futures) {
			auto result = fut.get();
			host_hashes.insert(result.begin(), result.end());
		}

		std::cout << "done. the map size is " << host_hashes.size() << std::endl;

		run_link_splitter_on_links_with_target_host_in_set(host_hashes);
	}


}
