
#include "HashTableShard.h"
#include "system/Logger.h"

HashTableShard::HashTableShard(size_t shard_id)
: m_shard_id(shard_id), m_loaded(false)
{
}

HashTableShard::~HashTableShard() {

}

void HashTableShard::add(uint64_t key, const string &value) {
	ofstream outfile(filename_data(), ios::binary | ios::app);
	const size_t cur_pos = (size_t)outfile.tellp();
	outfile.write((char *)&key, HT_KEY_SIZE);

	size_t data_len = min(value.size(), (size_t)HT_DATA_LENGTH);
	char buffer[HT_DATA_LENGTH];
	memset(buffer, '\0', HT_DATA_LENGTH);
	memcpy(buffer, value.c_str(), data_len);

	outfile.write(buffer, HT_DATA_LENGTH);
	m_pos[key] = cur_pos;

	ofstream outfile_pos(filename_pos(), ios::binary | ios::app);
	outfile_pos.write((char *)&key, HT_KEY_SIZE);
	outfile_pos.write((char *)&cur_pos, sizeof(size_t));
}

string HashTableShard::find(uint64_t key) {
	if (!m_loaded) load();
	auto iter = m_pos.find(key);
	if (iter == m_pos.end()) return "";

	size_t pos = iter->second;

	ifstream infile(filename_data(), ios::binary);
	infile.seekg(pos, ios::beg);

	const size_t buffer_len = HT_DATA_LENGTH + HT_KEY_SIZE;
	char buffer[buffer_len];

	infile.read(buffer, buffer_len);
	return string((char *)&buffer[HT_KEY_SIZE]);
}

string HashTableShard::filename_data() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + to_string(m_shard_id) + ".data";
}

string HashTableShard::filename_pos() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + to_string(m_shard_id) + ".pos";
}

size_t HashTableShard::shard_id() const {
	return m_shard_id;
}

void HashTableShard::load() {
	m_loaded = true;
	ifstream infile(filename_pos(), ios::binary);
	const size_t record_len = HT_KEY_SIZE + sizeof(size_t);
	const size_t buffer_len = record_len * 1000;
	char buffer[buffer_len];
	if (infile.is_open()) {
		do {
			infile.read(buffer, buffer_len);

			size_t read_bytes = infile.gcount();

			for (size_t i = 0; i < read_bytes; i += record_len) {
				m_pos[*((uint64_t *)&buffer[i])] = *((size_t *)&buffer[i + HT_KEY_SIZE]);
			}

		} while (!infile.eof());
	}
}

