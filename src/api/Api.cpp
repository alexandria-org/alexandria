
#include "Api.h"
#include "ApiResponse.h"

#include "post_processor/PostProcessor.h"

#include "full_text/SearchMetric.h"

#include "search_engine/SearchEngine.h"
#include "stats/Stats.h"

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

	Aws::Utils::Json::JsonValue generate_word_stats_json(const map<string, double> word_map) {
		Aws::Utils::Json::JsonValue result;
		for (const auto &iter : word_map) {
			Aws::Utils::Json::JsonValue json_number;
			result.WithObject(iter.first, json_number.AsDouble(iter.second));
		}

		return result;
	}

	Aws::Utils::Json::JsonValue generate_index_stats_json(const map<string, double> word_map, size_t index_size) {
		Aws::Utils::Json::JsonValue result, total;
		result.WithObject("total", total.AsInt64(index_size));
		result.WithObject("words", generate_word_stats_json(word_map));
		return result;
	}

	void word_stats(const string &query, vector<FullTextIndex<FullTextRecord> *> index_array,
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array, size_t index_size, size_t link_index_size, stringstream &response_stream) {

		Profiler profiler("total");

		map<string, double> word_stats = Stats::word_stats<FullTextRecord>(index_array, query, index_size);
		map<string, double> link_word_stats = Stats::word_stats<LinkFullTextRecord>(link_index_array, query, link_index_size);

		double time_ms = profiler.get();

		Aws::Utils::Json::JsonValue message("{}"), json_string;
		message.WithObject("status", json_string.AsString("success"));
		message.WithObject("time_ms", json_string.AsDouble(time_ms));
		message.WithObject("index", generate_index_stats_json(word_stats, index_size));
		message.WithObject("link_index", generate_index_stats_json(link_word_stats, link_index_size));

		response_stream << message.View().WriteReadable();
	}

}

