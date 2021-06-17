
#include "HashTableShardBuilder.h"
#include "system/Logger.h"

HashTableShardBuilder::HashTableShardBuilder(size_t shard_id)
//: m_shard_id(shard_id), m_cache_limit(50 + rand() % 50)
: m_shard_id(shard_id), m_cache_limit(150 + rand() % 50)
{

}

HashTableShardBuilder::~HashTableShardBuilder() {

}

bool HashTableShardBuilder::full() const {
	return m_cache.size() > m_cache_limit;
}

void HashTableShardBuilder::write() {
	ofstream outfile(filename_data(), ios::binary | ios::app);
	ofstream outfile_pos(filename_pos(), ios::binary | ios::app);

	size_t last_pos = outfile.tellp();

	char buffer[HT_DATA_LENGTH];
	for (const auto &iter : m_cache) {
		outfile.write((char *)&iter.first, HT_KEY_SIZE);

		size_t data_len = min(iter.second.size(), (size_t)HT_DATA_LENGTH);
		memset(buffer, '\0', HT_DATA_LENGTH);
		memcpy(buffer, iter.second.c_str(), data_len);
		outfile.write(buffer, HT_DATA_LENGTH);

		outfile_pos.write((char *)&iter.first, HT_KEY_SIZE);
		outfile_pos.write((char *)&last_pos, sizeof(size_t));
		last_pos += HT_DATA_LENGTH + HT_KEY_SIZE;
	}

	m_cache.clear();
}

void HashTableShardBuilder::sort() {

}

void HashTableShardBuilder::add(uint64_t key, const string &value) {
	m_cache[key] = value;
}

string HashTableShardBuilder::filename_data() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + to_string(m_shard_id) + ".data";
}

string HashTableShardBuilder::filename_pos() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + to_string(m_shard_id) + ".pos";
}

