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

	template<typename DataRecord>
	bool result_has_many_domains(const FullTextResultSet<DataRecord> *results) {

		if (results->size() == 0) return false;

		const DataRecord *data = results->data_pointer();
		const uint64_t first_domain_hash = data[0].m_domain_hash;
		for (size_t i = 0; i < results->size(); i++) {
			if (data[i].m_domain_hash != first_domain_hash) {
				return true;
			}
		}

		return false;
	}

	template<typename DataRecord>
	bool result_has_many_domains_vector(const vector<DataRecord> &results) {

		if (results.size() == 0) return false;

		const uint64_t first_domain_hash = results[0].m_domain_hash;
		for (const DataRecord &record : results) {
			if (record.m_domain_hash != first_domain_hash) {
				return true;
			}
		}

		return false;
	}

	template<typename DataRecord>
	void deduplicate_domains(FullTextResultSet<DataRecord> *results, size_t results_per_domain, size_t limit) {

		vector<DataRecord> deduplicate;
		unordered_map<uint64_t, size_t> domain_counts;
		DataRecord *records = results->data_pointer();
		size_t j = 0;
		for (size_t i = 0; i < results->size() && j < limit; i++) {
			records[j] = records[i];
			if (domain_counts[records[i].m_domain_hash] < results_per_domain) {
				j++;
				domain_counts[records[i].m_domain_hash]++;
			}
		}
		results->resize(j);
	}

	template<typename DataRecord>
	void deduplicate_result(FullTextResultSet<DataRecord> *results, size_t limit) {

		bool has_many_domains = result_has_many_domains<DataRecord>(results);

		if (results->size() > 10 && has_many_domains) {
			deduplicate_domains<DataRecord>(results, 5, limit);
		} else {
			if (results->size() > limit) {
				results->resize(limit);
			}
		}
	}

	template<typename DataRecord>
	vector<DataRecord> deduplicate_domains_vector(const vector<DataRecord> results, size_t results_per_domain, size_t limit) {

		vector<DataRecord> deduplicate;
		unordered_map<uint64_t, size_t> domain_counts;
		for (const DataRecord &record : results) {
			if (deduplicate.size() >= limit) break;
			if (domain_counts[record.m_domain_hash] < results_per_domain) {
				deduplicate.push_back(record);
				domain_counts[record.m_domain_hash]++;
			}
		}

		return deduplicate;
	}

	template<typename DataRecord>
	vector<DataRecord> deduplicate_result_vector(const vector<DataRecord> &results, size_t limit) {

		vector<DataRecord> deduped_result;
		bool has_many_domains = result_has_many_domains_vector<DataRecord>(results);

		if (results.size() > 10 && has_many_domains) {
			deduped_result = deduplicate_domains_vector<DataRecord>(results, 5, limit);
		} else {
			if (results.size() > limit) {
				deduped_result.insert(deduped_result.begin(), results.begin(), results.begin() + limit);
			} else {
				deduped_result = results;
			}
		}
		return deduped_result;
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

		if (input->deduplicate) {
			deduplicate_result<FullTextRecord>(flat_result, input->limit);
		}

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
			.limit = limit,
			.deduplicate = true
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
			.limit = limit,
			.deduplicate = false
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

		if (complete_result.size() < limit) {
			metric.m_total_found = complete_result.size();
		}

		return complete_result;
	}

	/*
		Add scores for the given links to the result set. The links are assumed to be ordered by link.m_target_hash ascending.
	*/
	size_t apply_link_scores(const vector<LinkFullTextRecord> &links, FullTextResultSet<FullTextRecord> *results) {
		size_t applied_links = 0;

		size_t i = 0;
		size_t j = 0;
		map<pair<uint64_t, uint64_t>, uint64_t> domain_unique;
		FullTextRecord *data = results->data_pointer();
		while (i < links.size() && j < results->size()) {

			const uint64_t hash1 = links[i].m_target_hash;
			const uint64_t hash2 = data[j].m_value;

			if (hash1 < hash2) {
				i++;
			} else if (hash1 == hash2) {

				if (domain_unique.count(make_pair(links[i].m_source_domain, links[i].m_target_hash)) == 0) {
					const float url_score = expm1(25.0f*links[i].m_score) / 50.0f;
					data[j].m_score += url_score;
					applied_links++;
					domain_unique[make_pair(links[i].m_source_domain, links[i].m_target_hash)] = links[i].m_source_domain;
				}

				i++;
			} else {
				j++;
			}
		}

		return applied_links;
	}

	size_t apply_domain_link_scores(const vector<DomainLinkFullTextRecord> &links, FullTextResultSet<FullTextRecord> *results) {
		size_t applied_links = 0;
		{
			unordered_map<uint64_t, float> domain_scores;
			unordered_map<uint64_t, int> domain_counts;
			map<pair<uint64_t, uint64_t>, uint64_t> domain_unique;
			{
				for (const DomainLinkFullTextRecord &link : links) {

					if (domain_unique.count(make_pair(link.m_source_domain, link.m_target_domain)) == 0) {

						const float domain_score = expm1(25.0f*link.m_score) / 50.0f;
						domain_scores[link.m_target_domain] += domain_score;
						domain_counts[link.m_target_domain]++;
						domain_unique[make_pair(link.m_source_domain, link.m_target_domain)] = link.m_source_domain;

					}
				}
			}

			// Loop over the results and add the calculated domain scores.
			FullTextRecord *data = results->data_pointer();
			for (size_t i = 0; i < results->size(); i++) {
				const float domain_score = domain_scores[data[i].m_domain_hash];
				data[i].m_score += domain_score;
				applied_links += domain_counts[data[i].m_domain_hash];
			}
		}

		return applied_links;
	}

}
