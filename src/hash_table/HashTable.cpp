
#include "HashTable.h"
#include "system/Logger.h"

HashTable::HashTable(const string &db_name)
: m_db_name(db_name)
{
	size_t num_items = 0;
	for (size_t shard_id = 0; shard_id < HT_NUM_SHARDS; shard_id++) {
		auto shard = new HashTableShard(shard_id);
		num_items += shard->size();
		m_shards.push_back(shard);
	}

	cout << "HashTable contains " << num_items << " (" << ((double)num_items/1000000000) << "b) urls" << endl;
}

HashTable::~HashTable() {

	for (HashTableShard *shard : m_shards) {
		delete shard;
	}
}

void HashTable::add(uint64_t key, const string &value) {

	m_shards[key % HT_NUM_SHARDS]->add(key, value);

}

string HashTable::find(uint64_t key) {
	return m_shards[key % HT_NUM_SHARDS]->find(key);
}

void HashTable::upload(const SubSystem *sub_system) {
	const size_t num_threads_downloading = 100;
	ThreadPool pool(num_threads_downloading);
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

void HashTable::download(const SubSystem *sub_system) {
	const size_t num_threads_uploading = 100;
	ThreadPool pool(num_threads_uploading);
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

void HashTable::run_upload_thread(const SubSystem *sub_system, const HashTableShard *shard) {
	ifstream infile_data(shard->filename_data());
	if (infile_data.is_open()) {
		const string key = "hash_table/" + m_db_name + "/" + to_string(shard->shard_id()) + ".data.gz";
		sub_system->upload_from_stream("alexandria-index", key, infile_data);
	}

	ifstream infile_pos(shard->filename_pos());
	if (infile_pos.is_open()) {
		const string key = "hash_table/" + m_db_name + "/" + to_string(shard->shard_id()) + ".pos.gz";
		sub_system->upload_from_stream("alexandria-index", key, infile_pos);
	}
}

void HashTable::run_download_thread(const SubSystem *sub_system, const HashTableShard *shard) {
	ofstream outfile_data(shard->filename_data(), ios::binary | ios::trunc);
	if (outfile_data.is_open()) {
		const string key = "hash_table/" + m_db_name + "/" + to_string(shard->shard_id()) + ".data.gz";
		sub_system->download_to_stream("alexandria-index", key, outfile_data);
	}

	ofstream outfile_pos(shard->filename_pos(), ios::binary | ios::trunc);
	if (outfile_pos.is_open()) {
		const string key = "hash_table/" + m_db_name + "/" + to_string(shard->shard_id()) + ".pos.gz";
		sub_system->download_to_stream("alexandria-index", key, outfile_pos);
	}
}

