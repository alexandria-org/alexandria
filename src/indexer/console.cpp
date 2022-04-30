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

#include "console.h"
#include <vector>
#include <iomanip>
#include "text/text.h"
#include "indexer/index_tree.h"
#include "indexer/sharded.h"
#include "indexer/counted_index.h"
#include "indexer/counted_record.h"
#include "URL.h"
#include "transfer/transfer.h"
#include "domain_stats/domain_stats.h"
#include "merger.h"
#include "file/tsv_file_remote.h"

using namespace std;

namespace indexer {

	void cmd_index(index_tree &idx_tree, const vector<string> &args) {
		if (args.size() < 2) return;

		merger::start_merge_thread();

		const string batch = args[1];
		size_t limit = 0;
		if (args.size() > 2) limit = stoull(args[2]);

		file::tsv_file_remote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
		vector<string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		if (limit && warc_paths.size() > limit) warc_paths.resize(limit);

		for (string &path : warc_paths) {
			const size_t pos = path.find(".warc.gz");
			if (pos != string::npos) {
				path.replace(pos, 8, ".gz");
			}
		}
		std::vector<std::string> local_files = transfer::download_gz_files_to_disk(warc_paths);
		cout << "starting indexer" << endl;
		idx_tree.add_index_files_threaded(local_files, 24);
		cout << "done with indexer" << endl;
		transfer::delete_downloaded_files(local_files);

		merger::stop_merge_thread();
	}

	void cmd_index_link(index_tree &idx_tree, const vector<string> &args) {
		if (args.size() < 2) return;

		merger::start_merge_thread();

		const string batch = args[1];
		size_t limit = 0;
		if (args.size() > 2) limit = stoull(args[2]);

		file::tsv_file_remote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
		vector<string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		if (limit && warc_paths.size() > limit) warc_paths.resize(limit);

		for (string &path : warc_paths) {
			const size_t pos = path.find(".warc.gz");
			if (pos != string::npos) {
				path.replace(pos, 8, ".gz");
			}
		}
		std::vector<std::string> local_files = transfer::download_gz_files_to_disk(warc_paths);
		cout << "starting indexer" << endl;
		idx_tree.add_link_files_threaded(local_files, 24);
		cout << "done with indexer" << endl;
		transfer::delete_downloaded_files(local_files);

		merger::stop_merge_thread();
	}

	void cmd_search(index_tree &idx_tree, hash_table::hash_table &ht, const string &query) {

		profiler::instance prof("domain search");
		std::vector<indexer::return_record> res = idx_tree.find(query);
		prof.stop();

		cout << "took " << prof.get() << "ms" << endl;

		cout << setw(50) << "domain";
		cout << setw(20) << "score";
		cout << endl;

		for (indexer::return_record &rec : res) {
			const string host = ht.find(rec.m_value);

			cout << setw(50) << host;
			cout << setw(20) << rec.m_score;
			cout << endl;
		}
	}

	void cmd_word(index_tree &idx_tree, hash_table::hash_table &ht, const string &query) {

		indexer::sharded<indexer::counted_index, indexer::counted_record> word_index("word_index", 256);

		const uint64_t word_hash = ::algorithm::hash(query);
		std::vector<indexer::counted_record> res = word_index.find(word_hash, 25);

		for (auto &rec : res) {
			const string host = ht.find(rec.m_value);
			cout << host << ": " << rec.m_count << " score: " << rec.m_score << endl;
		}

	}

	void cmd_harmonic(const vector<string> &args) {
		if (args.size() < 2) return;
		float harmonic = domain_stats::harmonic_centrality(URL(args[1]));
		cout << "url: " << args[1] << " has harmonic centrality " << harmonic << endl;
	}

	vector<string> input_to_args(const string &input) {
		const string word_boundary = " \t,|!";

		vector<string> raw_words, words;
		boost::split(raw_words, input, boost::is_any_of(word_boundary));

		for (string &word : raw_words) {
			text::trim(word);
			if (word.size()) {
				words.push_back(word);
			}
		}

		return words;
	}

	void console() {

		//domain_stats::download_domain_stats();

		//LOG_INFO("Done download_domain_stats");
		
		indexer::index_tree idx_tree;

		indexer::domain_level domain_level;
		//indexer::url_level url_level;
		//indexer::snippet_level snippet_level;

		idx_tree.add_level(&domain_level);
		//idx_tree.add_level(&url_level);
		//idx_tree.add_level(&snippet_level);

		//idx_tree.truncate();
		
		hash_table::hash_table ht("index_tree");

		string input;
		while (cout << "# " && getline(cin, input)) {

			const vector<string> args = input_to_args(input);

			if (args.size() == 0) continue;

			const string cmd = args[0];
			if (cmd == "index") {
				cmd_index(idx_tree, args);
			} else if (cmd == "index_link") {
				cmd_index_link(idx_tree, args);
			} else if (cmd == "harmonic") {
				cmd_harmonic(args);
			} else if (cmd == "search") {
				vector<string> query_words(args.begin() + 1, args.end());
				const string query = boost::algorithm::join(query_words, " ");
				cmd_search(idx_tree, ht, query);
			} else if (cmd == "word") {
				vector<string> query_words(args.begin() + 1, args.end());
				const string query = boost::algorithm::join(query_words, " ");
				cmd_word(idx_tree, ht, query);
			} else if (cmd == "quit") {
				break;
			}
		}
	}

	
	void index_urls(const string &batch) {

		domain_stats::download_domain_stats();
		LOG_INFO("Done download_domain_stats");

		{
			indexer::index_tree idx_tree;

			indexer::domain_level domain_level;
			//indexer::url_level url_level;
			//indexer::snippet_level snippet_level;

			idx_tree.add_level(&domain_level);
			//idx_tree.add_level(&url_level);
			//idx_tree.add_level(&snippet_level);

			//idx_tree.truncate();

			merger::start_merge_thread();

			size_t limit = 0;
			//size_t limit = 1;

			file::tsv_file_remote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
			vector<string> warc_paths;
			warc_paths_file.read_column_into(0, warc_paths);

			if (limit && warc_paths.size() > limit) warc_paths.resize(limit);

			for (string &path : warc_paths) {
				const size_t pos = path.find(".warc.gz");
				if (pos != string::npos) {
					path.replace(pos, 8, ".gz");
				}
			}
			std::vector<std::string> local_files = transfer::download_gz_files_to_disk(warc_paths);
			cout << "starting indexer, allocated_memory: " << memory::allocated_memory() << endl;
			idx_tree.add_index_files_threaded(local_files, 32);
			cout << "done with indexer, allocated_memory: " << memory::allocated_memory() << endl;
			transfer::delete_downloaded_files(local_files);

			merger::stop_merge_thread();
		}

		//indexer::sharded_index_builder<domain_record> dom_index("domain", 1024);
		//dom_index.optimize();

		profiler::print_report();
	}

	void index_links(const string &batch) {

		/*indexer::sharded_index_builder<domain_record> dom_index("domain", 1024);
		dom_index.optimize();

		return;

		indexer::sharded_index_builder<link_record> link_index_builder("link_index", 2001);
		indexer::sharded_index_builder<domain_link_record> domain_link_index_builder("domain_link_index", 2001);

		link_index_builder.optimize();
		domain_link_index_builder.optimize();

		return;*/

		domain_stats::download_domain_stats();
		LOG_INFO("Done download_domain_stats");

		{
			indexer::index_tree idx_tree;

			merger::start_merge_thread();

			file::tsv_file_remote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
			vector<string> warc_paths;
			warc_paths_file.read_column_into(0, warc_paths, 1000, 1200);

			std::vector<std::string> local_files = transfer::download_gz_files_to_disk(warc_paths);
			cout << "starting indexer" << endl;
			idx_tree.add_link_files_threaded(local_files, 32);
			cout << "done with indexer" << endl;
			transfer::delete_downloaded_files(local_files);

			merger::stop_merge_thread();

			idx_tree.merge();
		}
		profiler::print_report();
	}

	void index_words(const string &batch) {
		{
			indexer::index_tree idx_tree;

			merger::start_merge_thread();

			file::tsv_file_remote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
			vector<string> warc_paths;
			warc_paths_file.read_column_into(0, warc_paths, 300);

			std::vector<std::string> local_files = transfer::download_gz_files_to_disk(warc_paths);
			cout << "starting indexer" << endl;
			idx_tree.add_word_files_threaded(local_files, 32);
			cout << "done with indexer" << endl;
			transfer::delete_downloaded_files(local_files);

			merger::stop_merge_thread();

			idx_tree.merge_word();
		}
		{
			indexer::sharded_builder<indexer::counted_index_builder, indexer::counted_record> word_index("word_index", 256);
			word_index.calculate_scores();
			word_index.sort_by_scores();
		}
	}

	void truncate_words() {
		{
			indexer::index_tree idx_tree;
			idx_tree.truncate_words();
		}
	}

	void truncate_links() {
		{
			indexer::index_tree idx_tree;
			idx_tree.truncate_links();
		}
	}

	void print_info() {

		indexer::sharded_index_builder<domain_link_record> domain_link_index_builder("domain_link_index", 2001);
		domain_link_index_builder.check();

		return;

		{
			indexer::sharded<indexer::counted_index, indexer::counted_record> word_index("word_index", 256);

			const uint64_t word_hash = ::algorithm::hash("väder");
			std::vector<indexer::counted_record> res = word_index.find(word_hash);
			std::sort(res.begin(), res.end(), [](const indexer::counted_record &a, const indexer::counted_record &b) {
				return a.m_score > b.m_score;	
			});

			hash_table::hash_table ht("index_tree");

			for (auto &rec : res) {

				const string host = ht.find(rec.m_value);

				cout << host << ": " << rec.m_count << " score: " << rec.m_score << endl;
			}

			return;
		}
		indexer::sharded_index<domain_record> dom_index("domain", 1024);

		cout << "num domains: " << dom_index.num_records() << endl;

		{
			indexer::index<indexer::domain_record> idx("domain", 123);
			idx.print_stats();
		}
		{
			indexer::index<indexer::domain_record> idx("domain", 842);
			idx.print_stats();
		}
		{
			indexer::index<indexer::domain_record> idx("domain", 1);
			idx.print_stats();
		}
	}

	void calc_scores() {
		{
			indexer::sharded_builder<indexer::counted_index_builder, indexer::counted_record> word_index("word_index", 256);
			word_index.calculate_scores();
			word_index.sort_by_scores();
		}
	}

}
