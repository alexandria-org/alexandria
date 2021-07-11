
#include "HashTableShardBuilder.h"
#include "system/Logger.h"

HashTableShardBuilder::HashTableShardBuilder(const string &db_name, size_t shard_id)
: m_db_name(db_name), m_shard_id(shard_id), m_cache_limit(25 + rand() % 10)
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

	for (const auto &iter : m_cache) {
		outfile.write((char *)&iter.first, HT_KEY_SIZE);

		// Compress data
		stringstream ss(iter.second);

		filtering_istream compress_stream;
		compress_stream.push(gzip_compressor());
		compress_stream.push(ss);

		stringstream compressed;
		compressed << compress_stream.rdbuf();

		string compressed_string(compressed.str());

		const size_t data_len = compressed_string.size();
		outfile.write((char *)&data_len, sizeof(size_t));

		outfile.write(compressed_string.c_str(), data_len);

		outfile_pos.write((char *)&iter.first, HT_KEY_SIZE);
		outfile_pos.write((char *)&last_pos, sizeof(size_t));
		last_pos += data_len + HT_KEY_SIZE + sizeof(size_t);
	}

	m_cache.clear();
}

void HashTableShardBuilder::truncate() {
	ofstream outfile(filename_data(), ios::binary | ios::trunc);
	ofstream outfile_pos(filename_pos(), ios::binary | ios::trunc);
}

void HashTableShardBuilder::sort() {

}

void HashTableShardBuilder::add(uint64_t key, const string &value) {
	m_cache[key] = value;
}

string HashTableShardBuilder::filename_data() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".data";
}

string HashTableShardBuilder::filename_pos() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".pos";
}

