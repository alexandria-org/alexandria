
#include "HashTable.h"
#include "system/Logger.h"

HashTable::HashTable() {
	for (size_t shard_id = 0; shard_id < HT_NUM_SHARDS; shard_id++) {
		m_shards.push_back(new HashTableShard(shard_id));
	}
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

