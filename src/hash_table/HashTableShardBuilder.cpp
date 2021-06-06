
#include "HashTableShardBuilder.h"

HashTableShardBuilder::HashTableShardBuilder(size_t shard_id)
: m_shard_id(shard_id)
{

}

HashTableShardBuilder::~HashTableShardBuilder() {

}

bool HashTableShardBuilder::full() const {
	return m_cache.size() > 100;
}

void HashTableShardBuilder::write() {
	ofstream outfile(filename(), ios::binary | ios::app);

	char buffer[HT_DATA_LENGTH];
	for (const auto &iter : m_cache) {
		outfile.write((char *)&iter.first, HT_KEY_SIZE);

		size_t data_len = min(iter.second.size(), (size_t)HT_DATA_LENGTH);
		memset(buffer, '\0', HT_DATA_LENGTH);
		memcpy(buffer, iter.second.c_str(), data_len);
		outfile.write(buffer, HT_DATA_LENGTH);
	}

	m_cache.clear();

}

void HashTableShardBuilder::add(uint64_t key, const string &value) {
	m_cache[key] = value;
}

string HashTableShardBuilder::filename() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + to_string(m_shard_id) + ".ht";
}

