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
#include "indexer/index_manager.h"
#include "indexer/sharded.h"
#include "indexer/basic_index.h"
#include "indexer/counted_record.h"
#include "URL.h"
#include "transfer/transfer.h"
#include "domain_stats/domain_stats.h"
#include "merger.h"
#include "file/tsv_file_remote.h"
#include "algorithm/bloom_filter.h"
#include "parser/parser.h"
#include "http/server.h"
#include "json.hpp"

namespace indexer {

	void cmd_index(index_manager &idx_manager, const std::vector<std::string> &args) {
		if (args.size() < 2) return;

		merger::start_merge_thread();

		const auto batch = args[1];
		size_t limit = 0;
		if (args.size() > 2) limit = stoull(args[2]);

		file::tsv_file_remote warc_paths_file(std::string("crawl-data/") + batch + "/warc.paths.gz");
		std::vector<std::string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		if (limit && warc_paths.size() > limit) warc_paths.resize(limit);

		for (auto &path : warc_paths) {
			const size_t pos = path.find(".warc.gz");
			if (pos != std::string::npos) {
				path.replace(pos, 8, ".gz");
			}
		}
		auto local_files = transfer::download_gz_files_to_disk(warc_paths);
		cout << "starting indexer" << endl;
		idx_manager.add_index_files_threaded(local_files, 24);
		cout << "done with indexer" << endl;
		transfer::delete_downloaded_files(local_files);

		merger::stop_merge_thread();
	}

	void cmd_search(index_manager &idx_manager, hash_table2::hash_table &ht, hash_table2::hash_table &url_ht, const std::string &query) {

		profiler::instance prof("domain search");
		std::vector<indexer::return_record> res = idx_manager.find(query);
		prof.stop();

		cout << "took " << prof.get() << "ms" << endl;

		cout << setw(50) << "domain";
		cout << setw(20) << "score";
		cout << endl;

		std::vector<uint64_t> domain_hashes;

		for (indexer::return_record &rec : res) {
			const auto host = ht.find(rec.m_value);
			domain_hashes.push_back(rec.m_value);

			cout << setw(50) << host;
			cout << setw(20) << rec.m_score;
			cout << endl;
		}

		profiler::instance prof2("url searches");

		cout << "sending " << domain_hashes.size() << " domain hashes" << endl;

		http::response http_res = transfer::post("http://65.108.132.103/?q=" + parser::urlencode(query), std::string((char *)domain_hashes.data(), domain_hashes.size() * sizeof(uint64_t)));

		const auto url_res = http_res.body();

		std::stringstream ss(url_res);

		std::map<uint64_t, std::vector<url_record>> results;
		while (!ss.eof()) {
			uint64_t incoming_domain_hash;
			ss.read((char *)&incoming_domain_hash, sizeof(uint64_t));
			if (ss.eof()) break;
			size_t num_records;
			ss.read((char *)&num_records, sizeof(size_t));
			for (size_t i = 0; i < num_records; i++) {
				uint64_t value;
				float score;
				ss.read((char *)&value, sizeof(uint64_t));
				ss.read((char *)&score, sizeof(float));
				results[incoming_domain_hash].push_back(url_record(value, score));
			}
		}

		for (auto domain_hash : domain_hashes) {
			for (const auto &url_record : results[domain_hash]) {
				const auto &line = url_ht.find(url_record.m_value);
				std::vector<std::string> cols;

				boost::algorithm::split(cols, line, boost::is_any_of("\t"));
				const auto url = cols[0];
				const auto title = cols[1];
				const auto snippet = cols[4];

				std::cout << url << std::endl;
			}
		}

		cout << "took " << prof2.get() << "ms" << endl;

		cout << "got " << results.size() << " responses" << endl;

	}

	void cmd_word(index_manager &idx_manager, hash_table2::hash_table &ht, const std::string &query) {

		indexer::sharded_builder<indexer::basic_index_builder, indexer::counted_record> word_index_builder("word_index", 256);
		indexer::sharded<indexer::basic_index, indexer::counted_record> word_index("word_index", 256);

		const uint64_t word_hash = ::algorithm::hash(query);
		std::vector<indexer::counted_record> res = word_index.find(word_hash, 100000);

		size_t pos = 0;
		for (auto &rec : res) {
			const auto host = ht.find(rec.m_value);
			cout << host << ": " << rec.m_count << " score: " << rec.m_score << " pos: " << pos << " m_value: " << rec.m_value << " doc_size: " << word_index_builder.document_size(rec.m_value) << endl;
			pos++;
		}

	}

	void cmd_domain_info(index_manager &idx_manager, hash_table2::hash_table &ht, const std::string &domain, size_t limit, size_t offset) {

		indexer::sharded<indexer::basic_index, indexer::counted_record> idx("title_word_counter", 997);

		const uint64_t domain_hash = ::algorithm::hash(domain);
		std::vector<indexer::counted_record> res = idx.find(domain_hash);

		sort(res.begin(), res.end(), indexer::counted_record::truncate_order());

		size_t pos = 0;
		for (auto &rec : res) {
			const auto word = ht.find(rec.m_value);
			cout << word << ": " << rec.m_count << endl;
			if (pos >= limit) break;
			pos++;
		}

	}

	void cmd_word(index_manager &idx_manager, hash_table2::hash_table &ht, const std::string &query, const std::string &domain) {

		indexer::sharded_builder<indexer::basic_index_builder, indexer::counted_record> word_index_builder("word_index", 256);
		indexer::sharded<indexer::basic_index, indexer::counted_record> word_index("word_index", 256);

		const uint64_t word_hash = ::algorithm::hash(query);
		std::vector<indexer::counted_record> res = word_index.find(word_hash);

		size_t pos = 0;
		for (auto &rec : res) {
			const auto host = ht.find(rec.m_value);
			if (host == domain) {
				cout << host << ": " << rec.m_count << " score: " << rec.m_score << " pos: " << pos << " m_value: " << rec.m_value << " doc_size: " << word_index_builder.document_size(rec.m_value) << endl;
			}
			pos++;
		}

	}

	void cmd_word_num(index_manager &idx_manager, hash_table2::hash_table &ht, const std::string &query) {

		indexer::sharded<indexer::basic_index, indexer::counted_record> word_index("word_index", 256);

		const uint64_t word_hash = ::algorithm::hash(query);
		std::vector<indexer::counted_record> res = word_index.find(word_hash);

		cout << "num_records: " << res.size() << endl;

	}

	void cmd_harmonic(const std::vector<std::string> &args) {
		if (args.size() < 2) return;
		float harmonic = domain_stats::harmonic_centrality(URL(args[1]));
		cout << "url: " << args[1] << " has harmonic centrality " << harmonic << endl;
	}

	std::vector<std::string> input_to_args(const std::string &input) {
		const auto word_boundary = " \t,|!";

		std::vector<std::string> raw_words, words;
		boost::split(raw_words, input, boost::is_any_of(word_boundary));

		for (auto &word : raw_words) {
			if (word.size()) {
				words.push_back(word);
			}
		}

		return words;
	}

	void console() {
	}

	void index_link_batch(const std::string &batch) {

		::algorithm::bloom_filter urls_to_index(625000027);
		urls_to_index.read_file(config::data_path() + "/0/url_filter.bloom");

		size_t limit = 1000;
		size_t offset = 0;
		while (true) {
			indexer::index_manager idx_manager;

			merger::start_merge_thread();

			file::tsv_file_remote warc_paths_file(std::string("crawl-data/") + batch + "/warc.paths");
			std::vector<std::string> warc_paths;
			warc_paths_file.read_column_into(0, warc_paths, limit, offset);

			if (warc_paths.size() == 0) {
				merger::stop_merge_thread();
				break;
			}

			auto local_files = transfer::download_gz_files_to_disk(warc_paths);
			cout << "starting indexer" << endl;
			idx_manager.add_link_files_threaded(local_files, 32, urls_to_index);
			cout << "done with indexer" << endl;
			transfer::delete_downloaded_files(local_files);

			merger::stop_merge_thread();

			offset += limit;
		}
	}

	void index_links() {

		domain_stats::download_domain_stats();
		LOG_INFO("Done download_domain_stats");
		
		for (const std::string &batch : config::link_batches) {
			index_link_batch(batch);
		}
	}

	void index_url_batch(const std::string &batch) {

		size_t limit = 1000;
		size_t offset = 0;
		while (true) {
			indexer::index_manager idx_manager;

			merger::start_merge_thread();

			file::tsv_file_remote warc_paths_file(std::string("crawl-data/") + batch + "/warc.paths");
			std::vector<std::string> warc_paths;
			warc_paths_file.read_column_into(0, warc_paths, limit, offset);

			if (warc_paths.size() == 0) {
				merger::stop_merge_thread();
				break;
			}

			cout << "downloading " << warc_paths.size() << " to disc" << endl;
			auto local_files = transfer::download_gz_files_to_disk(warc_paths);
			cout << "starting indexer" << endl;
			idx_manager.add_index_files_threaded(local_files, 32);
			cout << "done with indexer" << endl;
			transfer::delete_downloaded_files(local_files);

			merger::stop_merge_thread();

			offset += limit;
		}
		profiler::print_report();
	}

	void index_urls() {

		domain_stats::download_domain_stats();
		LOG_INFO("Done download_domain_stats");
		
		for (const std::string &batch : config::batches) {
			index_url_batch(batch);
		}
	}

	void truncate_links() {
		{
			indexer::index_manager idx_manager;
			idx_manager.truncate_links();
		}
	}

	void domain_info_server() {

		domain_stats::download_domain_stats();
		LOG_INFO("Done download_domain_stats");

		indexer::index_manager idx_manager;
		hash_table2::hash_table ht("word_hash_table");

		indexer::sharded<indexer::basic_index, counted_record> fp_title_counter("first_page_title_word_counter", 101);
		indexer::sharded<indexer::basic_index, indexer::counted_record> title_counter("title_word_counter", 997);
		indexer::sharded<indexer::basic_index, indexer::counted_record> link_counter("link_word_counter", 4001);

		cout << "starting server..." << endl;

		::http::server srv([&ht, &fp_title_counter, &title_counter, &link_counter](const http::request &req) {
			http::response res;

			URL url = req.url();

			auto query = url.query();

			size_t limit = 1000;
			if (query.count("limit")) limit = std::stoi(query["limit"]);

			size_t offset = 0;
			if (query.count("offset")) offset = std::stoi(query["offset"]);

			if (url.path() == "/favicon.ico") {
				res.code(404);
				res.body("404");
				return res;
			}

			std::stringstream body;

			auto domain = url.path();
			domain.erase(0, 1);

			body << "<html><head><meta http-equiv='Content-type' content='text/html; charset=utf-8'></head><body>";

			body << "<h1>" << domain << "</h1>" << endl;
			body << "<h3>harmonic: " << domain_stats::harmonic_centrality(domain) << "</h3>" << endl;
			body << "<h3>hash: " << ::algorithm::hash(domain) << "</h3>" << endl;

			body << "<pre>";

			const uint64_t domain_hash = ::algorithm::hash(domain);
			auto fp_results = fp_title_counter.find(domain_hash);
			auto results = title_counter.find(domain_hash);
			auto link_results = link_counter.find(domain_hash);

			sort(fp_results.begin(), fp_results.end(), indexer::counted_record::truncate_order());
			sort(results.begin(), results.end(), indexer::counted_record::truncate_order());
			sort(link_results.begin(), link_results.end(), indexer::counted_record::truncate_order());

			body << "Limit: " + std::to_string(limit) << endl;
			body << "Offset: " + std::to_string(offset) << endl << endl;
			const size_t original_offset = offset;
			body << "</pre>";
			body << "<div class=lefter>";
			body << "<pre class=green>";
			for (auto &rec : fp_results) {
				const auto word = ht.find(rec.m_value);
				body << word << ": " << rec.m_count << endl;
			}
			body << "</pre>";
			body << "<pre class=green>";
			double threshold = results.size() ? results[0].m_count : 0.0;
			size_t offset_start = 0;
			for (auto &rec : results) {
				if (rec.m_count >= threshold * 0.8) {
					const auto word = ht.find(rec.m_value);
					body << word << ": " << rec.m_count << endl;
					offset_start++;
				} else {
					break;
				}
			}
			if (offset < offset_start) offset = offset_start;
			body << "</pre>";

			body << "<pre>";

			size_t pos = 0;
			for (auto &rec : results) {
				if (pos >= offset) {
					const auto word = ht.find(rec.m_value);
					body << word << ": " << rec.m_count << endl;
				}
				if (pos >= limit + offset) break;
				pos++;
			}

			body << "</pre></div><pre class=righter>";

			pos = 0;
			for (auto &rec : link_results) {
				if (pos >= original_offset) {
					const auto word = ht.find(rec.m_value);
					body << word << ": " << rec.m_count << endl;
				}
				if (pos >= limit + original_offset) break;
				pos++;
			}

			body << "</pre><style>.lefter {width: 50%; float: left; }";

			res.code(200);

			res.body(body.str());

			return res;
		});
	}

	void make_domain_index() {

		/*sharded_index<domain_record> idx("domain_info", 997);

		size_t count = 0;
		idx.for_each([&count](uint64_t key, roaring::Roaring &recs) {
			count++;
		});

		cout << "num_words: " << count << endl;

		return;*/

		domain_stats::download_domain_stats();
		LOG_INFO("Done download_domain_stats");

		indexer::sharded<indexer::basic_index, counted_record> fp_title_counter("first_page_title_word_counter", 101);
		indexer::sharded<indexer::basic_index, indexer::counted_record> title_counter("title_word_counter", 997);
		indexer::sharded<indexer::basic_index, indexer::counted_record> link_counter("link_word_counter", 4001);

		merger::start_merge_thread();

		sharded_index_builder<domain_record> idx("domain_info", 997);
		idx.truncate();

		fp_title_counter.for_each([&idx](uint64_t domain_hash, std::vector<counted_record> &records) {
			for (const auto &record : records) {
				idx.add(record.m_value, domain_record(domain_hash, 0.0f));
			}
		});

		merger::stop_merge_thread_only_append();
		idx.merge();
		merger::start_merge_thread();

		title_counter.for_each([&idx](uint64_t domain_hash, std::vector<counted_record> &records) {

			// Sort by score.
			sort(records.begin(), records.end(), counted_record::truncate_order());
			float threshold = records.size() > 0 ? records[0].m_count * 0.8f : 0.0f;
			for (const auto &record : records) {
				if (record.m_count < threshold) break;
				idx.add(record.m_value, domain_record(domain_hash, 0.0f));
			}
		});

		merger::stop_merge_thread_only_append();
		idx.merge();
		merger::start_merge_thread();

		link_counter.for_each([&idx](uint64_t domain_hash, std::vector<counted_record> &records) {

			// Sort by score.
			sort(records.begin(), records.end(), counted_record::truncate_order());
			for (size_t i = 0; i < records.size() && i < 100; i++) {
				idx.add(records[i].m_value, domain_record(domain_hash, 0.0f));
			}
		});

		merger::stop_merge_thread_only_append();
		idx.merge();
		idx.optimize();
	}

	void make_domain_index_scores() {

		domain_stats::download_domain_stats();
		LOG_INFO("Done download_domain_stats");

		hash_table2::hash_table ht("index_manager");

		sharded_index_builder<domain_record> idx("domain_info", 997);

		idx.for_each_record([&ht](domain_record &rec) {
			URL u;
			const auto domain = ht.find(rec.m_value);

			float harmonic = domain_stats::harmonic_centrality(domain);

			rec.m_score = harmonic;
		});
		
	}

	void make_url_bloom_filter() {

		hash_table2::hash_table ht("index_manager");

		::algorithm::bloom_filter urls_to_index(625000027);

		ht.for_each_key([&urls_to_index](uint64_t key) {
			urls_to_index.insert(key);
		});

		urls_to_index.write_file(config::data_path() + "/0/url_filter.bloom");

	}

	void count_words_that_hit_max() {

		sharded<basic_index, url_record> url_index("url_index", 4001);

		size_t counter = 0;
		url_index.for_each([&](uint64_t key, auto &records) {
			if (records.size() >= config::ft_max_results_per_section) {
				counter++;
				std::cout << counter << std::endl;
			}
		});

	}

}
