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

#include "config.h"
#include "text/text.h"
#include "logger/logger.h"

using namespace std;

namespace Config {

	string node = "test0001";
	string master = "localhost";
	string upload = "localhost";
	string data_node;
	//string url_store_host = "http://localhost";
	string url_store_host = "http://node0009.alexandria.org";
	string url_store_path = "/alexandria/urlstore";
	string url_store_cache_path = "/mnt/4/urlstore_cache";

	size_t nodes_in_cluster = 1;
	size_t node_id = 0;

	bool index_snippets = true;
	bool index_text = true;

	vector<string> batches;
	vector<string> link_batches;
	size_t worker_count = 8;
	size_t query_max_words = 10;
	size_t query_max_len = 200;
	size_t deduplicate_domain_count = 5;
	size_t pre_result_limit = 200000;
	size_t result_limit = 1000;
	string file_upload_user = "";
	string file_upload_password = "";
	size_t n_grams = 1;
	size_t shard_hash_table_size = 100000;
	size_t html_parser_long_text_len = 1000;
	size_t ft_shard_builder_buffer_len = 240000;

	size_t ft_num_shards = 2048;
	size_t ft_max_sections = 8;
	size_t ft_max_results_per_section = 100000;
	size_t ft_section_depth = 8;
	size_t ft_max_cache_gb = 30;
	size_t ft_num_threads_indexing = 24;
	size_t ft_num_threads_merging = 24;
	size_t ft_num_threads_appending = 8;

	double ft_cached_bytes_per_shard() {
		return (ft_max_cache_gb * 1000ul*1000ul*1000ul) / (ft_num_shards * ft_num_threads_indexing);
	}

	void read_config(const string &config_file) {

		batches.clear();
		link_batches.clear();

		ifstream in(config_file);

		if (!in.is_open()) {
			LOG_ERROR("Could not read config file: " + config_file);
			return;
		}

		string line;
		while (getline(in, line)) {
			size_t comment_pos = line.find("#");
			if (comment_pos != string::npos) {
				line = line.substr(0, comment_pos);
			}
			if (text::trim(line) == "") {
				continue;
			}
			vector<string> parts;
			boost::split(parts, line, boost::is_any_of("="));

			for (string &part : parts) {
				part = text::trim(part);
			}

			if (parts[0] == "node") {
				node = parts[1];
			} else if (parts[0] == "master") {
				master = parts[1];
				upload = parts[1];
			} else if (parts[0] == "upload") {
				upload = parts[1];
			} else if (parts[0] == "data_node") {
				data_node = parts[1];
			} else if (parts[0] == "url_store_host") {
				url_store_host = parts[1];
			} else if (parts[0] == "url_store_path") {
				url_store_path = parts[1];
			} else if (parts[0] == "nodes_in_cluster") {
				nodes_in_cluster = stoi(parts[1]);
			} else if (parts[0] == "node_id") {
				node_id = stoi(parts[1]);
			} else if (parts[0] == "batches") {
				batches.push_back(parts[1]);
			} else if (parts[0] == "link_batches") {
				link_batches.push_back(parts[1]);
			} else if (parts[0] == "worker_count") {
				worker_count = stoi(parts[1]);
			} else if (parts[0] == "query_max_words") {
				query_max_words = stoi(parts[1]);
			} else if (parts[0] == "query_max_len") {
				query_max_len = stoi(parts[1]);
			} else if (parts[0] == "deduplicate_domain_count") {
				deduplicate_domain_count = stoi(parts[1]);
			} else if (parts[0] == "pre_result_limit") {
				pre_result_limit = stoi(parts[1]);
			} else if (parts[0] == "result_limit") {
				result_limit = stoi(parts[1]);
			} else if (parts[0] == "ft_num_shards") {
				ft_num_shards = stoi(parts[1]);
			} else if (parts[0] == "ft_max_sections") {
				ft_max_sections = stoi(parts[1]);
			} else if (parts[0] == "ft_max_results_per_section") {
				ft_max_results_per_section = stoi(parts[1]);
			} else if (parts[0] == "ft_section_depth") {
				ft_section_depth = stoi(parts[1]);
			} else if (parts[0] == "ft_max_cache_gb") {
				ft_max_cache_gb = stoi(parts[1]);
			} else if (parts[0] == "ft_num_threads_indexing") {
				ft_num_threads_indexing = stoi(parts[1]);
			} else if (parts[0] == "ft_num_threads_merging") {
				ft_num_threads_merging = stoi(parts[1]);
			} else if (parts[0] == "ft_num_threads_appending") {
				ft_num_threads_appending = stoi(parts[1]);
			} else if (parts[0] == "file_upload_user") {
				file_upload_user = parts[1];
			} else if (parts[0] == "file_upload_password") {
				file_upload_password = parts[1];
			} else if (parts[0] == "n_grams") {
				n_grams = stoull(parts[1]);
			} else if (parts[0] == "index_snippets") {
				index_snippets = static_cast<bool>(stoull(parts[1]));
			} else if (parts[0] == "index_text") {
				index_text = static_cast<bool>(stoull(parts[1]));
			} else if (parts[0] == "shard_hash_table_size") {
				shard_hash_table_size = stoull(parts[1]);
			} else if (parts[0] == "html_parser_long_text_len") {
				html_parser_long_text_len = stoull(parts[1]);
			}
		}
	}

}
