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

#include "SearchEngine.h"
#include "text/Text.h"
#include "sort/Sort.h"
#include "system/Profiler.h"
#include <cmath>

namespace SearchEngine {

	hash<string> hasher;

	void reset_search_metric(struct SearchMetric &metric) {
		metric.m_total_found = 0;
		metric.m_total_url_links_found = 0;
		metric.m_total_domain_links_found = 0;
		metric.m_links_handled = 0;
		metric.m_link_domain_matches = 0;
		metric.m_link_url_matches = 0;
	}

	void *search_with_domain_links(void *ptr) {

		struct SearchArguments<FullTextRecord> *input = (struct SearchArguments<FullTextRecord> *)ptr;

		vector<string> words = Text::get_full_text_words(input->query);
		if (words.size() == 0) return {};

		vector<FullTextResultSet<FullTextRecord> *> result_vector = search_shards<FullTextRecord>(*(input->shards), words);

		FullTextResultSet<FullTextRecord> *flat_result;
		if (result_vector.size() > 1) {
			flat_result = merge_results_to_one<FullTextRecord>(result_vector);

			set_total_found<FullTextRecord>(result_vector, *(input->metric), (double)flat_result->size() / largest_result(result_vector));
		} else {
			flat_result = result_vector[0];
			set_total_found<FullTextRecord>(result_vector, *(input->metric), 1.0);
			result_vector.clear();
		}

		delete_result_vector<FullTextRecord>(result_vector);

		input->metric->m_link_domain_matches = apply_domain_link_scores(*(input->domain_links), flat_result);
		input->metric->m_link_url_matches = apply_link_scores(*(input->links), flat_result);

		// Narrow down results directly to top 200K
		get_results_with_top_scores<FullTextRecord>(flat_result, 200000);

		return (void *)flat_result;
	}

	vector<FullTextRecord> search_with_links(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit, struct SearchMetric &metric) {

		if (links.size() == 0) {
			reset_search_metric(metric);
		}

		metric.m_links_handled = links.size();

		vector<pthread_t> threads;
		vector<struct SearchArguments<FullTextRecord>> args(index_array.size(), SearchArguments<FullTextRecord>{
			.query = query,
			.limit = limit
		});
		vector<struct SearchMetric> metrics_vector(index_array.size(), SearchMetric{});

		size_t idx = 0;
		for (FullTextIndex<FullTextRecord> *index : index_array) {
			pthread_t thread;

			args[idx].shards = index->shard_ptr();
			args[idx].metric = &metrics_vector[idx];
			args[idx].links = &links;
			args[idx].domain_links = &domain_links;

			pthread_create(&thread, NULL, search_with_domain_links, (void *)&(args[idx]));
			threads.push_back(thread);
			idx++;
		}

		vector<span<FullTextRecord> *> result_arrays;
		vector<FullTextResultSet<FullTextRecord> *> result_pointers;
		for (auto &thread : threads) {
			FullTextResultSet<FullTextRecord> *result;
			pthread_join(thread, (void **)&result);
			result_pointers.push_back(result);
			result_arrays.push_back(result->span_pointer());
		}

		vector<FullTextRecord> complete_result;
		Sort::merge_arrays(result_arrays, [](const FullTextRecord &a, const FullTextRecord &b) {
			return a.m_value < b.m_value;
		}, complete_result);

		// Delete result.
		for (FullTextResultSet<FullTextRecord> *result : result_pointers) {
			delete result;
		}

		sort_by_value<FullTextRecord>(complete_result);
		deduplicate_by_value<FullTextRecord>(complete_result);

		metric.m_total_found = 0;
		metric.m_link_domain_matches = 0;
		metric.m_link_url_matches = 0;
		for (const struct SearchMetric &m : metrics_vector) {
			metric.m_total_found += m.m_total_found;
			metric.m_link_domain_matches += m.m_link_domain_matches;
			metric.m_link_url_matches += m.m_link_url_matches;
		}

		// Sort.
		sort_by_score<FullTextRecord>(complete_result);

		vector<FullTextRecord> deduped_result = deduplicate_result_vector<FullTextRecord>(complete_result, limit);

		if (deduped_result.size() < limit) {
			metric.m_total_found = deduped_result.size();
		}

		return deduped_result;
	}

	vector<FullTextRecord> search_all_with_links(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit, struct SearchMetric &metric) {

		if (links.size() == 0) {
			reset_search_metric(metric);
		}

		metric.m_links_handled = links.size();

		vector<pthread_t> threads;
		vector<struct SearchArguments<FullTextRecord>> args(index_array.size(), SearchArguments<FullTextRecord>{
			.query = query,
			.limit = limit
		});
		vector<struct SearchMetric> metrics_vector(index_array.size(), SearchMetric{});

		size_t idx = 0;
		for (FullTextIndex<FullTextRecord> *index : index_array) {
			pthread_t thread;

			args[idx].shards = index->shard_ptr();
			args[idx].metric = &metrics_vector[idx];
			args[idx].links = &links;
			args[idx].domain_links = &domain_links;

			pthread_create(&thread, NULL, search_with_domain_links, (void *)&(args[idx]));
			threads.push_back(thread);
			idx++;
		}

		vector<span<FullTextRecord> *> result_arrays;
		vector<FullTextResultSet<FullTextRecord> *> result_pointers;
		for (auto &thread : threads) {
			FullTextResultSet<FullTextRecord> *result;
			pthread_join(thread, (void **)&result);
			result_pointers.push_back(result);
			result_arrays.push_back(result->span_pointer());
		}

		vector<FullTextRecord> complete_result;
		Sort::merge_arrays(result_arrays, [](const FullTextRecord &a, const FullTextRecord &b) {
			return a.m_value < b.m_value;
		}, complete_result);

		// Delete result.
		for (FullTextResultSet<FullTextRecord> *result : result_pointers) {
			delete result;
		}

		sort_by_value<FullTextRecord>(complete_result);
		deduplicate_by_value<FullTextRecord>(complete_result);

		metric.m_total_found = 0;
		metric.m_link_domain_matches = 0;
		metric.m_link_url_matches = 0;
		metric.m_links_handled = 0;
		for (const struct SearchMetric &m : metrics_vector) {
			metric.m_total_found += m.m_total_found;
			metric.m_link_domain_matches += m.m_link_domain_matches;
			metric.m_link_url_matches += m.m_link_url_matches;
		}

		// Sort.
		sort_by_score<FullTextRecord>(complete_result);

		if (complete_result.size() < limit) {
			metric.m_total_found = complete_result.size();
		}

		return complete_result;
	}

	vector<FullTextRecord> search_deduplicate(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit, struct SearchMetric &metric) {

		vector<FullTextRecord> complete_result = search_wrapper(index_array, links, domain_links, query, limit, metric);

		vector<FullTextRecord> deduped_result = deduplicate_result_vector<FullTextRecord>(complete_result, limit);

		if (deduped_result.size() < limit) {
			metric.m_total_found = deduped_result.size();
		}

		return deduped_result;
	}

}
