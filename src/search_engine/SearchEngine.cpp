
#include "SearchEngine.h"
#include "text/Text.h"
#include "sort/Sort.h"
#include "system/Profiler.h"
#include <cmath>

namespace SearchEngine {

	hash<string> hasher;

	void reset_search_metric(struct SearchMetric &metric) {
		metric.m_total_found = 0;
		metric.m_total_links_found = 0;
		metric.m_links_handled = 0;
		metric.m_link_domain_matches = 0;
		metric.m_link_url_matches = 0;
	}

	template<typename DataRecord>
	bool result_has_many_domains(const vector<DataRecord> &results) {

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
	vector<DataRecord> deduplicate_domains(const vector<DataRecord> results, size_t results_per_domain, size_t limit) {

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
	vector<DataRecord> deduplicate_result(const vector<DataRecord> &results, size_t limit) {

		vector<DataRecord> deduped_result;
		bool has_many_domains = result_has_many_domains<DataRecord>(results);

		if (results.size() > 10 && has_many_domains) {
			deduped_result = deduplicate_domains<DataRecord>(results, 5, limit);
		} else {
			if (results.size() > limit) {
				deduped_result.insert(deduped_result.begin(), results.begin(), results.begin() + limit);
			} else {
				deduped_result = results;
			}
		}
		return deduped_result;
	}

	vector<FullTextRecord> search_with_domain_links(const vector<FullTextShard<FullTextRecord> *> &shards, const vector<LinkFullTextRecord> &links,
		const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit, struct SearchMetric &metric) {

		vector<string> words = Text::get_full_text_words(query);
		if (words.size() == 0) return {};

		vector<FullTextResultSet<FullTextRecord> *> result_vector = search_shards<FullTextRecord>(shards, words);

		FullTextResultSet<FullTextRecord> *flat_result;
		if (result_vector.size() > 1) {
			flat_result = merge_results_to_one<FullTextRecord>(result_vector);

			set_total_found<FullTextRecord>(result_vector, metric, (double)flat_result->size() / largest_result(result_vector));
		} else {
			flat_result = result_vector[0];
			set_total_found<FullTextRecord>(result_vector, metric, 1.0);
			result_vector.clear();
		}

		delete_result_vector<FullTextRecord>(result_vector);

		metric.m_link_domain_matches = apply_domain_link_scores(domain_links, flat_result);
		metric.m_link_url_matches = apply_link_scores(links, flat_result);

		// Narrow down results directly to top 200K
		get_results_with_top_scores<FullTextRecord>(flat_result, 200000);

		vector<FullTextRecord> top_results = vector<FullTextRecord>(flat_result->span_pointer()->begin(), flat_result->span_pointer()->end());

		vector<FullTextRecord> deduped_result = deduplicate_result<FullTextRecord>(top_results, limit);

		return deduped_result;
	}

	vector<FullTextRecord> search_with_links(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit, struct SearchMetric &metric) {

		if (links.size() == 0) {
			reset_search_metric(metric);
		}

		metric.m_links_handled = links.size();

		vector<future<vector<FullTextRecord>>> futures;
		vector<struct SearchMetric> metrics_vector(index_array.size(), SearchMetric{});

		size_t idx = 0;
		for (FullTextIndex<FullTextRecord> *index : index_array) {
			future<vector<FullTextRecord>> future = async(search_with_domain_links, index->shards(), links, domain_links, query, limit,
				ref(metrics_vector[idx]));
			futures.push_back(move(future));
			idx++;
		}

		vector<FullTextRecord> complete_result;
		for (auto &future : futures) {
			vector<FullTextRecord> result = future.get();
			complete_result.insert(complete_result.end(), result.begin(), result.end());
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

		vector<FullTextRecord> deduped_result = deduplicate_result<FullTextRecord>(complete_result, limit);

		if (deduped_result.size() < limit) {
			metric.m_total_found = deduped_result.size();
		}

		return deduped_result;
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
