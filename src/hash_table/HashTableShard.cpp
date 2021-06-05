
#include "HashTableShard.h"
#include "system/Logger.h"

HashTableShard::HashTableShard(size_t shard_id)
: m_shard_id(shard_id)
{

	ifstream infile(filename(), ios::binary);
	if (infile.is_open()) {
		//throw error("Could not open full text shard " + filename() + ". Error: " + string(strerror(errno)));
		const size_t buffer_len = HT_DATA_LENGTH + HT_KEY_SIZE;
		char buffer[buffer_len];
		do {
			size_t cur_pos = (size_t)infile.tellg();
			infile.read(buffer, buffer_len);
			if (infile.eof()) break;

			uint64_t key = *((uint64_t *)&buffer[0]);
			m_pos[key] = cur_pos;

		} while (!infile.eof());
	}

}

HashTableShard::~HashTableShard() {

}

void HashTableShard::add(uint64_t key, const string &value) {
	ofstream outfile(filename(), ios::binary | ios::app);
	size_t cur_pos = (size_t)outfile.tellp();
	outfile.write((char *)&key, HT_KEY_SIZE);

	size_t data_len = min(value.size(), (size_t)HT_DATA_LENGTH);
	char buffer[HT_DATA_LENGTH];
	memset(buffer, '\0', HT_DATA_LENGTH);
	memcpy(buffer, value.c_str(), data_len);

	outfile.write(buffer, HT_DATA_LENGTH);
	m_pos[key] = cur_pos;
}

string HashTableShard::find(uint64_t key) const {
	auto iter = m_pos.find(key);
	if (iter == m_pos.end()) return "";

	size_t pos = iter->second;

	ifstream infile(filename(), ios::binary);
	infile.seekg(pos, ios::beg);

	const size_t buffer_len = HT_DATA_LENGTH + HT_KEY_SIZE;
	char buffer[buffer_len];

	infile.read(buffer, buffer_len);
	return string((char *)&buffer[HT_KEY_SIZE]);
}

string HashTableShard::filename() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + to_string(m_shard_id) + ".ht";
}

