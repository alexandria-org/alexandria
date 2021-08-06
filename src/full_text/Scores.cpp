
#include "Scores.h"
#include <cmath>

namespace Scores {

	/*
		Add scores for the given links to the result set. The links are assumed to be ordered by link.m_target_hash ascending.
	*/
	void apply_link_scores(const vector<LinkFullTextRecord> &links, vector<FullTextRecord> &results, struct SearchMetric &metric) {
		{
			Profiler profiler3("Adding domain link scores");

			unordered_map<uint64_t, float> domain_scores;
			{
				Profiler profiler_sum("Summing domain link scores");
				for (const LinkFullTextRecord &link : links) {
					const float domain_score = expm1(5*link.m_score) + 0.1;
					domain_scores[link.m_target_domain] += domain_score;
				}
			}

			metric.m_link_domain_matches = domain_scores.size();

			// Loop over the results and add the calculated domain scores.
			for (size_t i = 0; i < results.size(); i++) {
				const float domain_score = domain_scores[results[i].m_domain_hash];
				results[i].m_score += domain_score;
			}
		}

		{
			Profiler profiler3("Adding url link scores");
			size_t i = 0;
			size_t j = 0;
			while (i < links.size() && j < results.size()) {

				const uint64_t hash1 = links[i].m_target_hash;
				const uint64_t hash2 = results[j].m_value;

				if (hash1 < hash2) {
					i++;
				} else if (hash1 == hash2) {
					const float url_score = expm1(10*links[i].m_score) + 0.1;

					results[j].m_score += url_score;
					metric.m_link_url_matches++;
					i++;
				} else {
					j++;
				}
			}
		}
	}

}
