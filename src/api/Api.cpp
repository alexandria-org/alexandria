
#include "Api.h"
#include "ApiResponse.h"

#include "post_processor/PostProcessor.h"

#include "full_text/SearchMetric.h"

#include "search_engine/SearchEngine.h"

#include "link_index/LinkIndex.h"

#include "system/Logger.h"
#include "system/Profiler.h"

namespace Api {

	void search(const string &query, HashTable &hash_table, vector<FullTextIndex<FullTextRecord> *> index_array,
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array, stringstream &response_stream) {

		Profiler profiler("total");

		struct SearchMetric metric;

		vector<LinkFullTextRecord> links = SearchEngine::search_link_array(link_index_array, query, 1000000, metric);
		vector<FullTextRecord> results = SearchEngine::search_index_array(index_array, links, query, 1000, metric);

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

	void word_stats(const string &query, vector<FullTextIndex<FullTextRecord> *> index_array,
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array, stringstream &response_stream) {

		response_stream << "{\"status\": \"success\", \"index\": {\"words\": {}}, \"link_index\": {\"words\": {}}}";
	}

}

