
#include <iostream>
#include "http/server.h"
#include "search_server.h"
#include "indexer/index_manager.h"
#include "indexer/domain_level.h"
#include "indexer/url_record.h"
#include "hash_table/hash_table.h"
#include "transfer/transfer.h"
#include "parser/parser.h"
#include "api/result_with_snippet.h"
#include "api/api_response.h"
#include "full_text/full_text_record.h"
#include "full_text/search_metric.h"
#include "json.hpp"

namespace indexer {

	void search_server() {

		indexer::index_manager idx_manager;

		indexer::domain_level domain_level;
		idx_manager.add_level(&domain_level);

		hash_table::hash_table ht("index_manager");
		hash_table::hash_table url_ht("snippets");

		cout << "starting server..." << endl;

		::http::server srv([&idx_manager, &ht, &url_ht](const http::request &req) {
			http::response res;

			URL url = req.url();

			auto query = url.query();

			size_t limit = 1000;
			if (query.count("limit")) limit = std::stoi(query["limit"]);

			(void)limit;

			std::string q = query["q"];

			if (url.path() == "/favicon.ico") {
				res.code(404);
				res.body("404");
				return res;
			}

			stringstream body;

			profiler::instance prof("domain search");
			std::vector<indexer::return_record> domain_records = idx_manager.find(q);

			std::vector<uint64_t> domain_hashes;
			std::map<uint64_t, float> domain_scores;
			std::map<uint64_t, uint64_t> url_to_domain;

			for (indexer::return_record &rec : domain_records) {
				domain_hashes.push_back(rec.m_value);
				domain_scores[rec.m_value] = rec.m_score;
			}

			profiler::instance prof2("url searches");

			http::response http_res = transfer::post("http://65.108.132.103/?q=" + parser::urlencode(q), string((char *)domain_hashes.data(), domain_hashes.size() * sizeof(uint64_t)));

			std::vector<url_record> results;
			if (http_res.code() == 200) {

				const string url_res = http_res.body();

				stringstream ss(url_res);

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
						url_record url_rec(value, score + domain_scores[incoming_domain_hash]);
						results.push_back(url_rec);
						url_to_domain[value] = incoming_domain_hash;
					}
				}
			}

			std::sort(results.begin(), results.end(), url_record::truncate_order());

			prof.stop();

			vector<api::result_with_snippet> results_with_snippets;
			for (const auto &url_record : results) {
				const std::string line = url_ht.find(url_record.m_value);

				full_text::full_text_record ft_rec;
				ft_rec.m_value = url_record.m_value;
				ft_rec.m_score = url_record.m_score;
				ft_rec.m_domain_hash = url_to_domain[url_record.m_value];

				results_with_snippets.emplace_back(api::result_with_snippet(line, ft_rec));
			}

			full_text::search_metric metric;
			api::api_response response(results_with_snippets, metric, prof.get());

			body << response;

			res.code(200);

			res.body(body.str());

			return res;
		});
	}
}
