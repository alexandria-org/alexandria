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

#include "Counter.h"

#include <iostream>
#include <future>
#include <vector>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/filesystem.hpp>
#include "config.h"
#include "parser/URL.h"
#include "link/Link.h"
#include "link/LinkCounter.h"
#include "transfer/Transfer.h"
#include "algorithm/HyperLogLog.h"
#include "algorithm/Algorithm.h"

using namespace std;

namespace Tools {

	Algorithm::HyperLogLog<size_t> *count_urls(const vector<string> &warc_paths) {

		Algorithm::HyperLogLog<size_t> *counter = new Algorithm::HyperLogLog<size_t>();

		size_t idx = 0;
		for (const string &warc_path : warc_paths) {
			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const URL url(line.substr(0, line.find("\t")));
				counter->insert_hash(url.hash());
			}

			if (idx % 100 == 0) {
				cout << warc_path << " done " << idx << "/" << warc_paths.size() << endl;
			} 

			idx++;
		}

		return counter;
	}

	Algorithm::HyperLogLog<size_t> *count_links(const vector<string> &warc_paths) {

		Algorithm::HyperLogLog<size_t> *counter = new Algorithm::HyperLogLog<size_t>();

		size_t idx = 0;
		for (const string &warc_path : warc_paths) {
			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const Link::Link link(line);
				counter->insert_hash(link.target_url().hash());
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

		for (const string &batch : Config::batches) {

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

		for (const string &batch : Config::link_batches) {

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
		Algorithm::vector_chunk(files, ceil((double)files.size() / num_threads), thread_input);

		vector<vector<string>> link_thread_input;
		Algorithm::vector_chunk(link_files, ceil((double)link_files.size() / num_threads), link_thread_input);

		mutex write_file_mutex;

		/*
		Run url counters
		*/
		vector<future<Algorithm::HyperLogLog<size_t> *>> futures;
		for (size_t i = 0; i < num_threads && i < thread_input.size(); i++) {
			futures.emplace_back(std::async(launch::async, count_urls, thread_input[i]));
		}

		Algorithm::HyperLogLog<size_t> url_counter;
		for (auto &future : futures) {
			Algorithm::HyperLogLog<size_t> *result = future.get();
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

		Algorithm::HyperLogLog<size_t> link_counter;
		for (auto &future : futures) {
			Algorithm::HyperLogLog<size_t> *result = future.get();
			link_counter += *(result);
			delete result;
		}

		cout << "Uniq urls: " << url_counter.size() << endl;
		cout << "Uniq links: " << link_counter.size() << endl;
	}

	vector<string> download_link_batch(const string &batch, size_t limit, size_t offset) {
		
		File::TsvFileRemote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
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

		return Transfer::download_gz_files_to_disk(files_to_download);
	}

	void count_link_batch(const string &batch, map<size_t, map<size_t, float>> &counter) {

		const size_t limit = 10;
		size_t offset = 0;

		while (true) {
			vector<string> files = download_link_batch(batch, limit, offset);
			if (files.size() == 0) break;
			Link::run_link_counter(batch, files, counter);
			Transfer::delete_downloaded_files(files);
			offset += files.size();
			break;
		}
	}

	void count_all_links() {
		for (const string &batch : Config::link_batches) {
			map<size_t, map<size_t, float>> counter;
			count_link_batch(batch, counter);

			// Upload link counts.
			Link::upload_link_counts(batch, counter);
		}
	}
}

