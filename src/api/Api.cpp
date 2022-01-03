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

#include "Api.h"
#include "ApiResponse.h"

#include "hash_table/HashTable.h"
#include "post_processor/PostProcessor.h"

#include "full_text/SearchMetric.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"

#include "link/FullTextRecord.h"
#include "domain_link/FullTextRecord.h"

#include "search_engine/SearchAllocation.h"

#include "search_engine/SearchEngine.h"
#include "stats/Stats.h"

#include "LinkResult.h"
#include "DomainLinkResult.h"

#include "system/Logger.h"
#include "system/Profiler.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

namespace Api {

	void search(const string &query, HashTable &hash_table, const FullTextIndex<FullTextRecord> &index,
		SearchAllocation::Allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct SearchMetric metric;
		SearchEngine::reset_search_metric(metric);

		metric.m_total_found = 0;

		vector<FullTextRecord> results = SearchEngine::search_deduplicate(allocation->storage, index, {}, {}, query, Config::result_limit, metric);

		PostProcessor post_processor(query);

		vector<ResultWithSnippet> with_snippets;
		for (FullTextRecord &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(ResultWithSnippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		ApiResponse response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	void search(const string &query, HashTable &hash_table, const FullTextIndex<FullTextRecord> &index,
		const FullTextIndex<Link::FullTextRecord> &link_index,
		SearchAllocation::Allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct SearchMetric metric;
		SearchEngine::reset_search_metric(metric);

		vector<Link::FullTextRecord> links;
		Profiler::instance profiler_links("SearchEngine::search<Link::FullTextRecord>");
		links = SearchEngine::search<Link::FullTextRecord>(allocation->link_storage, link_index, {}, {}, query, 500000, metric);
		profiler_links.stop();

		sort(links.begin(), links.end(), [](const Link::FullTextRecord &a, const Link::FullTextRecord &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		const size_t links_handled = links.size();
		const size_t total_url_links_found = metric.m_total_found;

		vector<FullTextRecord> results = SearchEngine::search_deduplicate(allocation->storage, index, links, {}, query,
			Config::result_limit, metric);

		PostProcessor post_processor(query);

		vector<ResultWithSnippet> with_snippets;
		for (FullTextRecord &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(ResultWithSnippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		metric.m_links_handled = links_handled;
		metric.m_total_url_links_found = total_url_links_found;

		ApiResponse response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	void search(const string &query, HashTable &hash_table, const FullTextIndex<FullTextRecord> &index,
		const FullTextIndex<Link::FullTextRecord> &link_index,
		const FullTextIndex<DomainLink::FullTextRecord> &domain_link_index,
		SearchAllocation::Allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct SearchMetric metric;
		SearchEngine::reset_search_metric(metric);

		vector<Link::FullTextRecord> links;
		Profiler::instance profiler_links("SearchEngine::search<Link::FullTextRecord>");
		links = SearchEngine::search<Link::FullTextRecord>(allocation->link_storage, link_index, {}, {}, query, 500000, metric);
		profiler_links.stop();

		sort(links.begin(), links.end(), [](const Link::FullTextRecord &a, const Link::FullTextRecord &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		const size_t links_handled = links.size();
		const size_t total_url_links_found = metric.m_total_found;

		vector<DomainLink::FullTextRecord> domain_links;
		Profiler::instance profiler_domain_links("SearchEngine::search<DomainLink::FullTextRecord>");
		domain_links = SearchEngine::search<DomainLink::FullTextRecord>(allocation->domain_link_storage, domain_link_index, {}, {}, query,
			100000, metric);
		profiler_domain_links.stop();

		const size_t total_domain_links_found = metric.m_total_found;

		Profiler::instance profiler_index("SearchEngine::search_with_links");
		vector<FullTextRecord> results = SearchEngine::search_deduplicate(allocation->storage, index, links, domain_links, query,
			Config::result_limit, metric);
		profiler_index.stop();

		PostProcessor post_processor(query);

		vector<ResultWithSnippet> with_snippets;
		for (FullTextRecord &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(ResultWithSnippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		metric.m_links_handled = links_handled;
		metric.m_total_url_links_found = total_url_links_found;
		metric.m_total_domain_links_found = total_domain_links_found;

		ApiResponse response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	void search_all(const string &query, HashTable &hash_table, const FullTextIndex<FullTextRecord> &index,
		SearchAllocation::Allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct SearchMetric metric;
		SearchEngine::reset_search_metric(metric);

		Profiler::instance profiler_index("SearchEngine::search_with_links");
		vector<FullTextRecord> results = SearchEngine::search(allocation->storage, index, {}, {}, query, Config::result_limit,
			metric);
		profiler_index.stop();

		PostProcessor post_processor(query);

		vector<ResultWithSnippet> with_snippets;
		for (FullTextRecord &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(ResultWithSnippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		ApiResponse response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	void search_all(const string &query, HashTable &hash_table, const FullTextIndex<FullTextRecord> &index,
		const FullTextIndex<Link::FullTextRecord> &link_index,
		SearchAllocation::Allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct SearchMetric metric;
		SearchEngine::reset_search_metric(metric);

		vector<Link::FullTextRecord> links;
		Profiler::instance profiler_links("SearchEngine::search<Link::FullTextRecord>");
		links = SearchEngine::search<Link::FullTextRecord>(allocation->link_storage, link_index, {}, {}, query, 500000, metric);
		profiler_links.stop();

		sort(links.begin(), links.end(), [](const Link::FullTextRecord &a, const Link::FullTextRecord &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		metric.m_total_url_links_found = metric.m_total_found;
		metric.m_total_found = 0;

		Profiler::instance profiler_index("SearchEngine::search_with_links");
		vector<FullTextRecord> results = SearchEngine::search(allocation->storage, index, links, {}, query, Config::result_limit,
			metric);
		profiler_index.stop();

		PostProcessor post_processor(query);

		vector<ResultWithSnippet> with_snippets;
		for (FullTextRecord &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(ResultWithSnippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		ApiResponse response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

	void search_all(const string &query, HashTable &hash_table, const FullTextIndex<FullTextRecord> &index,
		const FullTextIndex<Link::FullTextRecord> &link_index, const FullTextIndex<DomainLink::FullTextRecord> &domain_link_index,
		SearchAllocation::Allocation *allocation, stringstream &response_stream) {

		Profiler::instance profiler;

		struct SearchMetric metric;
		SearchEngine::reset_search_metric(metric);

		vector<Link::FullTextRecord> links;
		Profiler::instance profiler_links("SearchEngine::search<Link::FullTextRecord>");
		links = SearchEngine::search<Link::FullTextRecord>(allocation->link_storage, link_index, {}, {}, query, 500000, metric);
		profiler_links.stop();

		sort(links.begin(), links.end(), [](const Link::FullTextRecord &a, const Link::FullTextRecord &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		metric.m_total_url_links_found = metric.m_total_found;
		metric.m_total_found = 0;

		Profiler::instance profiler_domain_links("SearchEngine::search<DomainLink::FullTextRecord>");
		vector<DomainLink::FullTextRecord> domain_links = SearchEngine::search<DomainLink::FullTextRecord>(allocation->domain_link_storage,
			domain_link_index, {}, {}, query, 10000, metric);
		profiler_domain_links.stop();

		metric.m_total_domain_links_found = metric.m_total_found;

		Profiler::instance profiler_index("SearchEngine::search_with_links");
		vector<FullTextRecord> results = SearchEngine::search(allocation->storage, index, links, domain_links, query, Config::result_limit,
			metric);
		profiler_index.stop();

		PostProcessor post_processor(query);

		vector<ResultWithSnippet> with_snippets;
		for (FullTextRecord &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(ResultWithSnippet(tsv_data, res));
		}

		post_processor.run(with_snippets);

		ApiResponse response(with_snippets, metric, profiler.get());

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

	void word_stats(const string &query, const FullTextIndex<FullTextRecord> &index, const FullTextIndex<Link::FullTextRecord> &link_index,
			size_t index_size, size_t link_index_size, stringstream &response_stream) {

		Profiler::instance profiler;

		map<string, double> word_stats = Stats::word_stats<FullTextRecord>(index, query, index_size);
		map<string, double> link_word_stats = Stats::word_stats<Link::FullTextRecord>(link_index, query, link_index_size);

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

	void ids(const std::string &query, const FullTextIndex<FullTextRecord> &index, SearchAllocation::Allocation *allocation,
			std::stringstream &response_stream) {

		vector<FullTextRecord> results = SearchEngine::search_ids(allocation->storage, index, query, Config::result_limit);

		for (const FullTextRecord &result : results) {
			response_stream.write((char *)&result, sizeof(FullTextRecord));
		}

	}

	void search_remote(const std::string &query, HashTable &hash_table, const FullTextIndex<Link::FullTextRecord> &link_index,
		const FullTextIndex<DomainLink::FullTextRecord> &domain_link_index, SearchAllocation::Allocation *allocation,
		std::stringstream &response_stream) {

		Profiler::instance profiler;

		future<FullTextResultSet<FullTextRecord> *> fut = async(SearchEngine::search_remote<FullTextRecord>, query, allocation->storage);

		struct SearchMetric metric;
		SearchEngine::reset_search_metric(metric);

		vector<Link::FullTextRecord> links = SearchEngine::search<Link::FullTextRecord>(allocation->link_storage, link_index, {}, {}, query,
			500000, metric);

		sort(links.begin(), links.end(), [](const Link::FullTextRecord &a, const Link::FullTextRecord &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		const size_t links_handled = links.size();
		const size_t total_url_links_found = metric.m_total_found;

		vector<DomainLink::FullTextRecord> domain_links = SearchEngine::search<DomainLink::FullTextRecord>(allocation->domain_link_storage,
			domain_link_index, {}, {}, query, 100000, metric);

		const size_t total_domain_links_found = metric.m_total_found;

		metric.m_links_handled = links_handled;
		metric.m_total_url_links_found = total_url_links_found;
		metric.m_total_domain_links_found = total_domain_links_found;

		FullTextResultSet<FullTextRecord> *result_set = fut.get();

		SearchEngine::apply_link_scores(links, result_set);
		SearchEngine::apply_domain_link_scores(domain_links, result_set);

		vector<FullTextRecord> results(result_set->span_pointer()->begin(), result_set->span_pointer()->end());

		SearchEngine::sort_by_score(results);

		vector<ResultWithSnippet> with_snippets;
		for (FullTextRecord &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(ResultWithSnippet(tsv_data, res));
		}

		metric.m_links_handled = links_handled;
		metric.m_total_url_links_found = total_url_links_found;
		metric.m_total_domain_links_found = total_domain_links_found;

		ApiResponse response(with_snippets, metric, profiler.get());

		response_stream << response;
	}

}

