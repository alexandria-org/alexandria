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

#include "counter.h"

#include <iostream>
#include <future>
#include <vector>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/filesystem.hpp>
#include "config.h"
#include "URL.h"
#include "url_link/link.h"
#include "url_link/link_counter.h"
#include "transfer/transfer.h"
#include "algorithm/hyper_log_log.h"
#include "algorithm/algorithm.h"
#include "url_store/url_store.h"
#include "file/tsv_file_remote.h"

using namespace std;

namespace tools {

	algorithm::hyper_log_log *count_urls(const vector<string> &warc_paths) {

		algorithm::hyper_log_log *counter = new algorithm::hyper_log_log();

		size_t idx = 0;
		for (const string &warc_path : warc_paths) {
			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const URL url(line.substr(0, line.find("\t")));
				counter->insert(url.hash());
			}

			if (idx % 100 == 0) {
				cout << warc_path << " done " << idx << "/" << warc_paths.size() << endl;
			} 

			idx++;
		}

		return counter;
	}

	algorithm::hyper_log_log *count_links(const vector<string> &warc_paths) {

		algorithm::hyper_log_log *counter = new algorithm::hyper_log_log();

		size_t idx = 0;
		for (const string &warc_path : warc_paths) {
			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const url_link::link link(line);
				counter->insert(link.target_url().hash());
			}

			if (idx % 100 == 0) {
				cout << warc_path << " done " << idx << "/" << warc_paths.size() << endl;
			} 

			idx++;
		}

		return counter;
	}

	void run_counter() {

		const size_t num_threads = 12;

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
		Run url counters
		*/
		vector<future<algorithm::hyper_log_log *>> futures;
		for (size_t i = 0; i < num_threads && i < thread_input.size(); i++) {
			futures.emplace_back(std::async(launch::async, count_urls, thread_input[i]));
		}

		algorithm::hyper_log_log url_counter;
		for (auto &future : futures) {
			algorithm::hyper_log_log *result = future.get();
			url_counter += *(result);
			delete result;
		}

		futures.clear();

		/*
		Run link counters
		*/
		for (size_t i = 0; i < num_threads && i < link_thread_input.size(); i++) {
			futures.emplace_back(std::async(launch::async, count_links, link_thread_input[i]));
		}

		algorithm::hyper_log_log link_counter;
		for (auto &future : futures) {
			algorithm::hyper_log_log *result = future.get();
			link_counter += *(result);
			delete result;
		}

		cout << "Uniq urls: " << url_counter.count() << endl;
		cout << "Uniq links: " << link_counter.count() << endl;
	}

	vector<string> download_link_batch(const string &batch, size_t limit, size_t offset) {
		
		file::tsv_file_remote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
		vector<string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		vector<string> files_to_download;
		for (size_t i = offset; i < warc_paths.size() && i < (offset + limit); i++) {
			string warc_path = warc_paths[i];
			const size_t pos = warc_path.find(".warc.gz");
			if (pos != string::npos) {
				warc_path.replace(pos, 8, ".links.gz");
			}
			files_to_download.push_back(warc_path);
		}

		return transfer::download_gz_files_to_disk(files_to_download);
	}

	void count_link_batch(const common::sub_system *sub_system, const string &batch, size_t sub_batch, const vector<string> &files,
			map<string, map<size_t, float>> &counter) {

		const size_t chunk_size = 500;

		vector<vector<string>> chunks;
		algorithm::vector_chunk(files, chunk_size, chunks);

		for (const vector<string> &chunk : chunks) {
			url_link::run_link_counter(sub_system, batch, sub_batch, chunk, counter);
		}

	}

	void count_all_links() {

		common::sub_system *sub_system = new common::sub_system();

		for (const string &batch : config::link_batches) {

			vector<string> files = download_link_batch(batch, 50000, 0);

			for (size_t sub_batch = 0; sub_batch < 5; sub_batch++) {

				map<string, map<size_t, float>> counter;
				count_link_batch(sub_system, batch, sub_batch, files, counter);
				// Upload link counts.
				url_link::upload_link_counts(batch, sub_batch, counter);
			}

			transfer::delete_downloaded_files(files);
		}
		delete sub_system;
	}
}

