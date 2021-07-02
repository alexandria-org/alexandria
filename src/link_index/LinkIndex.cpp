
#include "LinkIndex.h"
#include "system/Logger.h"

LinkIndex::LinkIndex(const string &db_name)
: m_db_name(db_name)
{
	for (size_t shard_id = 0; shard_id < LI_NUM_SHARDS; shard_id++) {
		m_shards.push_back(new LinkShard(m_db_name, shard_id));
	}
}

LinkIndex::~LinkIndex() {
	for (LinkShard *shard : m_shards) {
		delete shard;
	}
}

vector<LinkResult> LinkIndex::search_phrase(const string &phrase, int limit, size_t &total_found) {

	total_found = 0;

	vector<string> words = get_full_text_words(phrase);

	vector<string> searched_words;
	map<size_t, LinkResultSet *> result_map;
	map<size_t, LinkResultSet *> or_result_map;

	size_t idx = 0;
	double total_time = 0.0;
	for (const string &word : words) {

		// One word should only be searched once.
		if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
		
		searched_words.push_back(word);

		uint64_t word_hash = m_hasher(word);
		Profiler profiler0("->find: " + word);
		LinkResultSet *results = new LinkResultSet();

		m_shards[word_hash % LI_NUM_SHARDS]->find(word_hash, results);
		profiler0.stop();

		Profiler profiler1("Accumuating "+to_string(results->len())+" results for: " + word);

		if (results->total_num_results() > results->len()) {
			or_result_map[idx] = results;
		} else {
			result_map[idx] = results;
		}
		idx++;
		total_time += profiler1.get();
	}

	if (result_map.size() == 0) {
		result_map = or_result_map;
		for (auto &iter : or_result_map) {
			if (iter.second->total_num_results() > total_found) {
				total_found = iter.second->total_num_results();
			}
		}
		or_result_map.clear();
	}

	Profiler profiler2("value_intersection");
	size_t shortest_vector;
	vector<size_t> result_ids = value_intersection(result_map, shortest_vector);

	vector<LinkResult> result;

	LinkResultSet *shortest = result_map[shortest_vector];
	vector<uint32_t> score_vector;
	uint64_t *link_hash_arr = shortest->link_hash_pointer();
	uint64_t *source_arr = shortest->source_pointer();
	uint64_t *target_arr = shortest->target_pointer();
	uint64_t *source_domain_arr = shortest->source_domain_pointer();
	uint64_t *target_domain_arr = shortest->target_domain_pointer();
	uint32_t *score_arr = shortest->score_pointer();
	for (size_t result_id : result_ids) {

		result.emplace_back(LinkResult(link_hash_arr[result_id], source_arr[result_id], target_arr[result_id],
			source_domain_arr[result_id], target_domain_arr[result_id], score_arr[result_id]));

		score_vector.push_back(score_arr[result_id]);
	}
	profiler2.stop();

	if (total_found == 0) {
		total_found = score_vector.size();
	}

	for (auto &iter : result_map) {
		delete iter.second;
	}
	for (auto &iter : or_result_map) {
		delete iter.second;
	}

	Profiler profiler3("sorting results");
	if (result.size() > limit) {
		nth_element(score_vector.begin(), score_vector.begin() + (limit - 1), score_vector.end());
		const uint32_t nth = score_vector[limit - 1];

		vector<LinkResult> top_result;
		for (const LinkResult &res : result) {
			if (res.m_score >= nth) {
				top_result.push_back(res);
				if (top_result.size() >= limit) break;
			}
		}

		sort_results(top_result);

		return top_result;
	}

	sort_results(result);

	return result;
}

size_t LinkIndex::disk_size() const {
	size_t size = 0;
	for (auto shard : m_shards) {
		size += shard->disk_size();
	}
	return size;
}

void LinkIndex::upload(const SubSystem *sub_system) {
	const size_t num_threads_uploading = 100;
	ThreadPool pool(num_threads_uploading);
	std::vector<std::future<void>> results;

	for (auto shard : m_shards) {
		results.emplace_back(
			pool.enqueue([this, sub_system, shard] {
				run_upload_thread(sub_system, shard);
			})
		);
	}

	for(auto && result: results) {
		result.get();
	}
}

void LinkIndex::download(const SubSystem *sub_system) {
	const size_t num_threads_downloading = 100;
	ThreadPool pool(num_threads_downloading);
	std::vector<std::future<void>> results;

	for (auto shard : m_shards) {
		results.emplace_back(
			pool.enqueue([this, sub_system, shard] {
				run_download_thread(sub_system, shard);
			})
		);
	}

	for(auto && result: results) {
		result.get();
	}
}

vector<size_t> LinkIndex::value_intersection(const map<size_t, LinkResultSet *> &values_map,
	size_t &shortest_vector_position) const {
	Profiler value_intersection("LinkIndex::value_intersection");
	
	if (values_map.size() == 0) return {};

	shortest_vector_position = 0;
	size_t shortest_len = SIZE_MAX;
	for (auto &iter : values_map) {
		if (shortest_len > iter.second->len()) {
			shortest_len = iter.second->len();
			shortest_vector_position = iter.first;
		}
	}

	vector<size_t> positions(values_map.size(), 0);
	vector<size_t> result_ids;

	auto value_iter = values_map.find(shortest_vector_position);
	uint64_t *link_hash_ptr = value_iter->second->link_hash_pointer();

	while (positions[shortest_vector_position] < shortest_len) {

		bool all_equal = true;
		uint64_t link_hash = link_hash_ptr[positions[shortest_vector_position]];

		for (auto &iter : values_map) {
			const uint64_t *val_arr = iter.second->link_hash_pointer();
			const size_t len = iter.second->len();
			size_t *pos = &(positions[iter.first]);
			while (link_hash > val_arr[*pos] && *pos < len) {
				(*pos)++;
			}
			if (link_hash < val_arr[*pos] && *pos < len) {
				all_equal = false;
			}
			if (*pos >= len) {
				all_equal = false;
			}
		}
		if (all_equal) {
			result_ids.push_back(positions[shortest_vector_position]);
		}

		positions[shortest_vector_position]++;
	}

	return result_ids;
}

void LinkIndex::sort_results(vector<LinkResult> &results) {
	sort(results.begin(), results.end(), [](const LinkResult a, const LinkResult b) {
		return a.m_score > b.m_score;
	});
}

void LinkIndex::run_upload_thread(const SubSystem *sub_system, const LinkShard *shard) {
	ifstream infile(shard->filename());
	if (infile.is_open()) {
		const string key = "link_index/" + m_db_name + "/" + to_string(shard->shard_id()) + ".gz";
		sub_system->upload_from_stream("alexandria-index", key, infile);
	}
}

void LinkIndex::run_download_thread(const SubSystem *sub_system, const LinkShard *shard) {
	ofstream outfile(shard->filename(), ios::binary | ios::trunc);
	if (outfile.is_open()) {
		const string key = "link_index/" + m_db_name + "/" + to_string(shard->shard_id()) + ".gz";
		sub_system->download_to_stream("alexandria-index", key, outfile);
	}
}
