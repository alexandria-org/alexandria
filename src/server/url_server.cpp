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

#include "url_server.h"

#include <iostream>
#include "http/server.h"
#include "indexer/index_manager.h"
#include "indexer/domain_level.h"
#include "indexer/url_record.h"
#include "hash_table/hash_table.h"

namespace server {
	void url_server() {

		cout << "starting server..." << endl;

		::http::server srv([](const http::request &req) {
			http::response res;

			URL url = req.url();

			auto query = url.query();

			stringstream body;

			if (req.request_method() == "POST") {
				const string req_body = req.request_body();

				const size_t num_hashes = req_body.size() / sizeof(uint64_t);
				std::vector<uint64_t> domain_hashes(num_hashes);
				memcpy((char *)domain_hashes.data(), req_body.c_str(), num_hashes * sizeof(uint64_t));

				auto tokens = text::get_tokens(query["q"]);

				size_t len = std::stoull(query["len"]);

				std::map<uint64_t, std::vector<indexer::url_record>> results;

				utils::thread_pool pool(32);
				std::mutex result_lock;
				cout << "received " << domain_hashes.size() << " hashes" << endl;
				size_t all_total_num_results = 0;
				for (auto dom_hash : domain_hashes) {
					pool.enqueue([dom_hash, tokens, &query, &result_lock, &results, &all_total_num_results, len]() {
						std::vector<indexer::url_record> res;

						vector<indexer::link_record> links;
						{
							// read links
							const string file = config::data_path() + "/" + to_string(dom_hash % 8) +
								"/full_text/url_links/" + to_string(dom_hash) + ".data";
							indexer::index_reader_file reader(file);

							if (reader.size()) {
								if (reader.size() > 10 * 1024* 1024) {
									indexer::index<indexer::link_record> idx("url_links", dom_hash, 1000);
									links = idx.find_top(tokens, 1000);
								} else {
									const size_t size = reader.size();
									std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
									reader.seek(0);
									reader.read(buffer.get(), size);
									std::istringstream ram_reader(string(buffer.get(), size));
									indexer::index<indexer::link_record> idx(&ram_reader, 1000);
									links = idx.find_top(tokens, 1000);
								}
							}

							std::sort(links.begin(), links.end(), indexer::link_record::storage_order());

							auto link_formula = [](float score) {
								return expm1(20.0f * score) / 10.0f;
							};

							std::vector<indexer::link_record> grouped;
							for (auto rec : links) {
								if (grouped.size() && grouped.back().storage_equal(rec)) {
									grouped.back().m_score += link_formula(rec.m_score);
								} else {
									grouped.emplace_back(rec);
									grouped.back().m_score = link_formula(rec.m_score);
								}
							}

							links = grouped;
						}

						const string file = config::data_path() + "/" + to_string(dom_hash % 8) + "/full_text/url/" +
							to_string(dom_hash) + ".data";
						indexer::index_reader_file reader(file);

						size_t mod_incr = 0;
						auto score_mod = [&mod_incr, &links](const indexer::url_record &record) {
							while (mod_incr < links.size() && links[mod_incr].m_target_hash < record.m_value) {
								mod_incr++;
							}
							float link_score = 0.0f;
							if (mod_incr < links.size() && links[mod_incr].m_target_hash == record.m_value) {
								link_score += links[mod_incr].m_score;
							}
							return record.m_score + ((1000.0f - record.url_length()) / 500.0f) + link_score;
						};

						size_t total_num_results = 0;

						if (reader.size()) {
							if (reader.size() > 10 * 1024* 1024) {
								indexer::index<indexer::url_record> idx("url", dom_hash, 1000);
								res = idx.find_top(total_num_results, tokens, len, score_mod);
							} else {
								const size_t size = reader.size();
								std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
								reader.seek(0);
								reader.read(buffer.get(), size);
								std::istringstream ram_reader(std::string(buffer.get(), size));
								indexer::index<indexer::url_record> idx(&ram_reader, 1000);
								res = idx.find_top(total_num_results, tokens, len, score_mod);
							}
						}

						std::lock_guard lock(result_lock);
						all_total_num_results += total_num_results;
						results[dom_hash] = res;
					});
				}

				pool.run_all();

				// Output result.
				body.write((char *)&all_total_num_results, sizeof(size_t));
				for (auto domain_hash : domain_hashes) {
					body.write((char *)&domain_hash, sizeof(uint64_t));
					size_t num_records = results[domain_hash].size();
					body.write((char *)&num_records, sizeof(size_t));

					for (const auto &record : results[domain_hash]) {
						body.write((char *)&(record.m_value), sizeof(uint64_t));
						body.write((char *)&(record.m_score), sizeof(float));
					}
				}

				res.content_type("application/octet-stream");
			}

			res.code(200);

			const string res_str = body.str();
			cout << "outputting: " << res_str.size() << endl;
			res.body(res_str);

			return res;
		});
	}
}
