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

#include "api.h"
#include "api_response.h"
#include "result_with_snippet.h"

#include "hash_table/HashTable.h"
#include "post_processor/PostProcessor.h"

#include "full_text/search_metric.h"
#include "full_text/full_text_index.h"
#include "full_text/full_text_record.h"

#include "url_link/full_text_record.h"
#include "domain_link/full_text_record.h"

#include "search_allocation/search_allocation.h"

#include "search_engine/search_engine.h"
#include "stats/stats.h"

#include "domain_link_result.h"

#include "logger/logger.h"
#include "system/Profiler.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

namespace api {

	using full_text::full_text_index;
	using full_text::full_text_record;
	using full_text::full_text_result_set;

	void search(const string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		search_allocation::allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct full_text::search_metric metric;
		search_engine::reset_search_metric(metric);

		metric.m_total_found = 0;

		vector<full_text_record> results = search_engine::search_deduplicate(allocation->record_storage, index, {}, {}, query, Config::result_limit, metric);

		PostProcessor post_processor(query);

		vector<result_with_snippet> with_snippets;
		for (full_text_record &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(result_with_snippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		api_response response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	void search(const string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		const full_text_index<url_link::full_text_record> &link_index,
		search_allocation::allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct full_text::search_metric metric;
		search_engine::reset_search_metric(metric);

		vector<url_link::full_text_record> links;
		Profiler::instance profiler_links("search_engine::search<url_link::full_text_record>");
		links = search_engine::search<url_link::full_text_record>(allocation->link_storage, link_index, {}, {}, query, 500000, metric);
		profiler_links.stop();

		sort(links.begin(), links.end(), [](const url_link::full_text_record &a, const url_link::full_text_record &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		const size_t links_handled = links.size();
		const size_t total_url_links_found = metric.m_total_found;

		vector<full_text_record> results = search_engine::search_deduplicate(allocation->record_storage, index, links, {}, query,
			Config::result_limit, metric);

		PostProcessor post_processor(query);

		vector<result_with_snippet> with_snippets;
		for (full_text_record &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(result_with_snippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		metric.m_links_handled = links_handled;
		metric.m_total_url_links_found = total_url_links_found;

		api_response response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	void search(const string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		const full_text_index<url_link::full_text_record> &link_index,
		const full_text_index<domain_link::full_text_record> &domain_link_index,
		search_allocation::allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct full_text::search_metric metric;
		search_engine::reset_search_metric(metric);

		vector<url_link::full_text_record> links;
		Profiler::instance profiler_links("search_engine::search<url_link::full_text_record>");
		links = search_engine::search<url_link::full_text_record>(allocation->link_storage, link_index, {}, {}, query, 500000, metric);
		profiler_links.stop();

		sort(links.begin(), links.end(), [](const url_link::full_text_record &a, const url_link::full_text_record &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		const size_t links_handled = links.size();
		const size_t total_url_links_found = metric.m_total_found;

		vector<domain_link::full_text_record> domain_links;
		Profiler::instance profiler_domain_links("search_engine::search<domain_link::full_text_record>");
		domain_links = search_engine::search<domain_link::full_text_record>(allocation->domain_link_storage, domain_link_index, {}, {}, query,
			100000, metric);
		profiler_domain_links.stop();

		const size_t total_domain_links_found = metric.m_total_found;

		Profiler::instance profiler_index("search_engine::search_with_links");
		vector<full_text_record> results = search_engine::search_deduplicate(allocation->record_storage, index, links, domain_links, query,
			Config::result_limit, metric);
		profiler_index.stop();

		PostProcessor post_processor(query);

		vector<result_with_snippet> with_snippets;
		for (full_text_record &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(result_with_snippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		metric.m_links_handled = links_handled;
		metric.m_total_url_links_found = total_url_links_found;
		metric.m_total_domain_links_found = total_domain_links_found;

		api_response response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	void search_all(const string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		search_allocation::allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct full_text::search_metric metric;
		search_engine::reset_search_metric(metric);

		Profiler::instance profiler_index("search_engine::search_with_links");
		vector<full_text_record> results = search_engine::search(allocation->record_storage, index, {}, {}, query, Config::result_limit,
			metric);
		profiler_index.stop();

		PostProcessor post_processor(query);

		vector<result_with_snippet> with_snippets;
		for (full_text_record &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(result_with_snippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		api_response response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	void search_all(const string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		const full_text_index<url_link::full_text_record> &link_index,
		search_allocation::allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct full_text::search_metric metric;
		search_engine::reset_search_metric(metric);

		vector<url_link::full_text_record> links;
		Profiler::instance profiler_links("search_engine::search<url_link::full_text_record>");
		links = search_engine::search<url_link::full_text_record>(allocation->link_storage, link_index, {}, {}, query, 500000, metric);
		profiler_links.stop();

		sort(links.begin(), links.end(), [](const url_link::full_text_record &a, const url_link::full_text_record &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		metric.m_total_url_links_found = metric.m_total_found;
		metric.m_total_found = 0;

		Profiler::instance profiler_index("search_engine::search_with_links");
		vector<full_text_record> results = search_engine::search(allocation->record_storage, index, links, {}, query, Config::result_limit,
			metric);
		profiler_index.stop();

		PostProcessor post_processor(query);

		vector<result_with_snippet> with_snippets;
		for (full_text_record &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(result_with_snippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		api_response response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	void search_all(const string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		const full_text_index<url_link::full_text_record> &link_index, const full_text_index<domain_link::full_text_record> &domain_link_index,
		search_allocation::allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct full_text::search_metric metric;
		search_engine::reset_search_metric(metric);

		vector<url_link::full_text_record> links;
		Profiler::instance profiler_links("search_engine::search<url_link::full_text_record>");
		links = search_engine::search<url_link::full_text_record>(allocation->link_storage, link_index, {}, {}, query, 500000, metric);
		profiler_links.stop();

		sort(links.begin(), links.end(), [](const url_link::full_text_record &a, const url_link::full_text_record &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		metric.m_total_url_links_found = metric.m_total_found;
		metric.m_total_found = 0;

		Profiler::instance profiler_domain_links("search_engine::search<domain_link::full_text_record>");
		vector<domain_link::full_text_record> domain_links = search_engine::search<domain_link::full_text_record>(allocation->domain_link_storage,
			domain_link_index, {}, {}, query, 10000, metric);
		profiler_domain_links.stop();

		metric.m_total_domain_links_found = metric.m_total_found;

		Profiler::instance profiler_index("search_engine::search_with_links");
		vector<full_text_record> results = search_engine::search(allocation->record_storage, index, links, domain_links, query, Config::result_limit,
			metric);
		profiler_index.stop();

		PostProcessor post_processor(query);

		vector<result_with_snippet> with_snippets;
		for (full_text_record &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(result_with_snippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		api_response response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	json generate_word_stats_json(const map<string, double> word_map) {
		json result;
		for (const auto &iter : word_map) {
			result[iter.first] = iter.second;
		}

		return result;
	}

	json generate_index_stats_json(const map<string, double> word_map, size_t index_size) {
		json result;
		result["total"] = index_size;
		result["words"] = generate_word_stats_json(word_map);
		return result;
	}

	void word_stats(const string &query, const full_text_index<full_text_record> &index, const full_text_index<url_link::full_text_record> &link_index,
			size_t index_size, size_t link_index_size, stringstream &response_stream) {

		Profiler::instance profiler;

		map<string, double> word_stats = stats::word_stats<full_text_record>(index, query, index_size);
		map<string, double> link_word_stats = stats::word_stats<url_link::full_text_record>(link_index, query, link_index_size);

		double time_ms = profiler.get();

		json message;
		message["status"] = "success";
		message["time_ms"] = time_ms;
		message["index"] = generate_index_stats_json(word_stats, index_size);
		message["link_index"] = generate_index_stats_json(link_word_stats, link_index_size);

		response_stream << message;
	}

	void url(const string &url_str, HashTable &hash_table, stringstream &response_stream) {
		Profiler::instance profiler;

		URL url(url_str);

		json message;
		message["status"] = "success";
		message["response"] = hash_table.find(url.hash());
		message["time_ms"] = profiler.get();

		response_stream << message;
	}

	void ids(const std::string &query, const full_text_index<full_text_record> &index, search_allocation::allocation *allocation,
			std::stringstream &response_stream) {

		vector<full_text_record> results = search_engine::search_ids(allocation->record_storage, index, query, Config::result_limit);

		for (const full_text_record &result : results) {
			response_stream.write((char *)&result, sizeof(full_text_record));
		}

	}

	void search_remote(const std::string &query, HashTable &hash_table, const full_text_index<url_link::full_text_record> &link_index,
		const full_text_index<domain_link::full_text_record> &domain_link_index, search_allocation::allocation *allocation,
		std::stringstream &response_stream) {

		LOG_INFO("SEARCHING REMOTE");

		Profiler::instance profiler;

		future<full_text_result_set<full_text_record> *> fut = async(search_engine::search_remote<full_text_record>, query, allocation->record_storage);

		struct full_text::search_metric metric;
		search_engine::reset_search_metric(metric);

		vector<url_link::full_text_record> links = search_engine::search<url_link::full_text_record>(allocation->link_storage, link_index, {}, {}, query,
			500000, metric);

		sort(links.begin(), links.end(), [](const url_link::full_text_record &a, const url_link::full_text_record &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		const size_t links_handled = links.size();
		const size_t total_url_links_found = metric.m_total_found;

		vector<domain_link::full_text_record> domain_links = search_engine::search<domain_link::full_text_record>(allocation->domain_link_storage,
			domain_link_index, {}, {}, query, 100000, metric);

		const size_t total_domain_links_found = metric.m_total_found;

		metric.m_links_handled = links_handled;
		metric.m_total_url_links_found = total_url_links_found;
		metric.m_total_domain_links_found = total_domain_links_found;

		full_text_result_set<full_text_record> *result_set = fut.get();

		search_engine::apply_link_scores(links, result_set);
		search_engine::apply_domain_link_scores(domain_links, result_set);

		vector<full_text_record> results(result_set->span_pointer()->begin(), result_set->span_pointer()->end());

		search_engine::sort_by_score(results);

		vector<result_with_snippet> with_snippets;
		for (full_text_record &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(result_with_snippet(tsv_data, res));
		}

		metric.m_links_handled = links_handled;
		metric.m_total_url_links_found = total_url_links_found;
		metric.m_total_domain_links_found = total_domain_links_found;

		api_response response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

}

