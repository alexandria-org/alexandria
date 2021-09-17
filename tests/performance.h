
#include "sort/Sort.h"
#include "system/Profiler.h"

BOOST_AUTO_TEST_SUITE(performance)

BOOST_AUTO_TEST_CASE(domain_index) {

	vector<FullTextShard<DomainLinkFullTextRecord> *> shards;

	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_0", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_1", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_2", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_3", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_4", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_5", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_6", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_7", 859));

	const uint64_t key = 10850050818246762331ull;

	Profiler::measure_base_performance();

	vector<DomainLinkFullTextRecord> correct_result;
	{
		Profiler::instance profile("total");
		vector<vector<DomainLinkFullTextRecord>> flat_results;
		for (FullTextShard<DomainLinkFullTextRecord> *shard : shards) {
			FullTextResultSet<DomainLinkFullTextRecord> *result_set = shard->find(key);

			vector<float> scores;
			vector<DomainLinkFullTextRecord> flat_result;
			SearchEngine::merge_results_to_vector<DomainLinkFullTextRecord>({result_set}, flat_result, scores);
			vector<DomainLinkFullTextRecord> top_results =
				SearchEngine::get_results_with_top_scores_vector<DomainLinkFullTextRecord>(flat_result, scores, 20000000);
			SearchEngine::sort_by_value(top_results);

			flat_results.push_back(top_results);

			delete result_set;
		}

		vector<DomainLinkFullTextRecord> complete_result;
		for (const auto &result : flat_results) {
			complete_result.insert(complete_result.end(), result.begin(), result.end());
		}

		sort(complete_result.begin(), complete_result.end(), [](const DomainLinkFullTextRecord &a, const DomainLinkFullTextRecord &b) {
			if (a.m_score == b.m_score) return a.m_value < b.m_value;
			return a.m_score > b.m_score;
		});
		complete_result.resize(10000);

		cout << "took " << profile.get() << " ms" << endl;

		correct_result = complete_result;
	}

	Profiler::instance profile("total");
	vector<vector<DomainLinkFullTextRecord>> flat_results;
	for (FullTextShard<DomainLinkFullTextRecord> *shard : shards) {
		FullTextResultSet<DomainLinkFullTextRecord> *result_set = shard->find(key);

		vector<float> scores;
		vector<DomainLinkFullTextRecord> flat_result;
		SearchEngine::merge_results_to_vector<DomainLinkFullTextRecord>({result_set}, flat_result, scores);
		SearchEngine::get_results_with_top_scores(flat_result, 20000000);

		flat_results.push_back(flat_result);

		delete result_set;
	}

	vector<DomainLinkFullTextRecord> complete_result;

	Sort::merge_arrays(flat_results, [](const DomainLinkFullTextRecord &a, const DomainLinkFullTextRecord &b) {
		return a.m_value < b.m_value;
	}, complete_result);

	nth_element(complete_result.begin(), complete_result.begin() + (10000 - 1), complete_result.end(), SearchEngine::comparator_class{});
	const DomainLinkFullTextRecord nth = complete_result[10000 - 1];

	vector<DomainLinkFullTextRecord> top_results;
	for (const auto &result : complete_result) {
		if (result.m_score == nth.m_score) {
			if (result.m_value <= nth.m_value) {
				top_results.push_back(result);
			}
		} else if (result.m_score > nth.m_score) {
			top_results.push_back(result);
		}
	}

	sort(top_results.begin(), top_results.end(), [](const DomainLinkFullTextRecord &a, const DomainLinkFullTextRecord &b) {
		if (a.m_score == b.m_score) return a.m_value < b.m_value;
		return a.m_score > b.m_score;
	});
	cout << "took " << profile.get() << " ms" << endl;
	cout << "took " << Profiler::get_absolute_performance(profile.get()) << " units" << endl;

	bool all_equal = true;
	for (size_t i = 0; i < top_results.size(); i++) {
		all_equal = all_equal && (top_results[i].m_value == correct_result[i].m_value);
	}

	BOOST_CHECK(all_equal);
	//BOOST_CHECK(num_cycles < 700000000);
}

BOOST_AUTO_TEST_SUITE_END();
