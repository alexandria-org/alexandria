
#include "HashTableShard.h"
#include "system/Logger.h"

HashTableShard::HashTableShard(size_t shard_id)
: m_shard_id(shard_id), m_loaded(false)
{
	load();
}

HashTableShard::~HashTableShard() {

}

void HashTableShard::add(uint64_t key, const string &value) {
	/*
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
	*/
}

string HashTableShard::find(uint64_t key) {
	Profiler pf1("HashTableShard::find1");
	const uint64_t key_significant = key >> (64-m_significant);
	auto iter = m_pos.find(key_significant);
	if (iter == m_pos.end()) return "";

	auto pos_pair = iter->second;
	size_t pos_in_posfile = pos_pair.first;
	size_t len_in_posfile = pos_pair.second;

	ifstream infile_pos(filename_pos(), ios::binary);
	infile_pos.seekg(pos_in_posfile, ios::beg);

	const size_t record_len = HT_KEY_SIZE + sizeof(size_t);
	const size_t byte_len = len_in_posfile * record_len;
	const size_t pos_buffer_len = 2000;
	char pos_buffer[pos_buffer_len];
	if (byte_len > pos_buffer_len) {
		throw error("len larger then buffer");
	}

	infile_pos.read(pos_buffer, byte_len);
	infile_pos.close();

	size_t pos = string::npos;
	for (size_t i = 0; i < byte_len; i+= record_len) {
		uint64_t tmp_key = *((uint64_t *)&pos_buffer[i]);
		if (tmp_key == key) {
			pos = *((size_t *)&pos_buffer[i + HT_KEY_SIZE]);
		}
	}
	if (pos == string::npos) return "";
	Profiler pf2("HashTableShard::find2");


	ifstream infile(filename_data(), ios::binary);
	infile.seekg(pos, ios::beg);

	const size_t buffer_len = HT_DATA_LENGTH + HT_KEY_SIZE;
	char buffer[buffer_len];

	Profiler pf3("HashTableShard::find3");
	infile.read(buffer, buffer_len);
	pf3.stop();
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
	const size_t buffer_len = record_len * 10000;
	char buffer[buffer_len];
	size_t latest_pos = 0;

	vector<uint64_t> keys;
	vector<size_t> positions;
	if (infile.is_open()) {
		do {
			infile.read(buffer, buffer_len);

			size_t read_bytes = infile.gcount();

			for (size_t i = 0; i < read_bytes; i += record_len) {
				keys.push_back(*((uint64_t *)&buffer[i]));
				positions.push_back(latest_pos);
				latest_pos += record_len;
			}

		} while (!infile.eof());
	}
	infile.close();

	size_t idx = 0;
	for (uint64_t key : keys) {
		const uint64_t key_significant = key >> (64-m_significant);
		if (m_pos.find(key_significant) == m_pos.end()) {
			m_pos[key_significant] = make_pair(positions[idx], 0);
		}
		m_pos[key_significant].second++;
		idx++;
	}

	/*for (const auto &iter : m_pos) {
		cout << "first: " << iter.first << " second.first: " << iter.second.first << " second.second: " << iter.second.second << endl;
	}*/

	//exit(0);

	// Store the position file again but sorted.
	/*ofstream outfile_pos(filename_pos(), ios::binary | ios::trunc);
	for (const auto &iter : m_pos) {
		outfile_pos.write((char *)&iter.first, HT_KEY_SIZE);
		outfile_pos.write((char *)&iter.second, sizeof(size_t));
	}
	outfile_pos.close();
	m_pos.clear();*/

	LogInfo("Loaded shard " + to_string(m_shard_id));
	//cout << "find on 4162737955575775232: " << find(4162737955575775232ull) << endl;
	//exit(0);
}

