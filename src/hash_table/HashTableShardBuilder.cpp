
#include "config.h"
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
		outfile.write((char *)&iter.first, Config::ht_key_size);

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

		outfile_pos.write((char *)&iter.first, Config::ht_key_size);
		outfile_pos.write((char *)&last_pos, sizeof(size_t));
		last_pos += data_len + Config::ht_key_size + sizeof(size_t);
	}

	m_cache.clear();
}

void HashTableShardBuilder::truncate() {
	ofstream outfile(filename_data(), ios::binary | ios::trunc);
	ofstream outfile_pos(filename_pos(), ios::binary | ios::trunc);
}

void HashTableShardBuilder::sort() {

	ifstream infile(filename_pos(), ios::binary);
	const size_t record_len = Config::ht_key_size + sizeof(size_t);
	const size_t buffer_len = record_len * 10000;
	char buffer[buffer_len];
	size_t latest_pos = 0;

	if (infile.is_open()) {
		do {
			infile.read(buffer, buffer_len);

			size_t read_bytes = infile.gcount();

			for (size_t i = 0; i < read_bytes; i += record_len) {
				const uint64_t key = *((uint64_t *)&buffer[i]);
				const size_t pos = *((size_t *)&buffer[i + Config::ht_key_size]);
				m_sort_pos[key] = pos;
			}

		} while (!infile.eof());
	}
	infile.close();

	ofstream outfile_pos(filename_pos(), ios::binary | ios::trunc);
	for (const auto &iter : m_sort_pos) {
		outfile_pos.write((char *)&iter.first, Config::ht_key_size);
		outfile_pos.write((char *)&iter.second, sizeof(size_t));
	}
	outfile_pos.close();
	m_sort_pos.clear();

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

