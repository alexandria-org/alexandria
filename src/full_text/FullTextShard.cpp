
#include "FullTextShard.h"
#include <cstring>
#include "system/Logger.h"
#include "system/Profiler.h"

/*
File format explained

8 bytes = unsigned int number of keys = num_keys
8 bytes * num_keys = list of keys
8 bytes * num_keys = list of positions in file counted from data start
8 bytes * num_keys = list of lengths
[DATA]

*/

FullTextShard::FullTextShard(const string &db_name, size_t shard)
: m_shard_id(shard), m_db_name(db_name), m_keys_read(false) {
	m_filename = "/mnt/"+to_string(m_shard_id % 8)+"/full_text/fti_" + m_db_name + "_" + to_string(m_shard_id) + ".idx";
	m_buffer = new char[m_buffer_len];
}

FullTextShard::~FullTextShard() {
	delete m_buffer;
}

void FullTextShard::find(uint64_t key, FullTextResultSet *result_set) {

	if (!m_keys_read) read_keys();

	Profiler pf("FullTextShard::find");

	auto iter = lower_bound(m_keys.begin(), m_keys.end(), key);

	if (iter == m_keys.end() || *iter > key) {
		return;
	}

	size_t key_pos = iter - m_keys.begin();

	ifstream reader(filename(), ios::binary);

	char buffer[64];

	// Read position and length.
	reader.seekg(m_pos_start + key_pos * 8, ios::beg);
	reader.read(buffer, 8);
	size_t pos = *((size_t *)(&buffer[0]));

	reader.seekg(m_len_start + key_pos * 8, ios::beg);
	reader.read(buffer, 8);
	size_t len = *((size_t *)(&buffer[0]));

	reader.seekg(m_total_start + key_pos * 8, ios::beg);
	reader.read(buffer, 8);
	size_t total_num_results = *((size_t *)(&buffer[0]));
	result_set->set_total_num_results(total_num_results);

	reader.seekg(m_data_start + pos, ios::beg);

	const size_t num_records = len / FULL_TEXT_RECORD_LEN;

	result_set->allocate(num_records);

	uint64_t *value_res = result_set->value_pointer();
	uint32_t *score_res = result_set->score_pointer();

	size_t read_bytes = 0;
	size_t kk = 0;
	Profiler pf2("FullTextShard::find ads");
	while (read_bytes < len) {
		size_t read_len = min(m_buffer_len, len);
		reader.read(m_buffer, read_len);
		read_bytes += read_len;

		size_t num_records_read = read_len / FULL_TEXT_RECORD_LEN;
		for (size_t i = 0; i < num_records_read; i++) {
			value_res[kk] = *((uint64_t *)&m_buffer[i*FULL_TEXT_RECORD_LEN]);
			score_res[kk] = *((uint32_t *)&m_buffer[i*FULL_TEXT_RECORD_LEN + FULL_TEXT_KEY_LEN]);
			kk++;
		}
	}
}

void FullTextShard::read_keys() {

	m_keys_read = true;

	m_keys.clear();
	m_num_keys = 0;

	char buffer[64];

	ifstream reader(filename(), ios::binary);

	if (!reader.is_open()) {
		m_num_keys = 0;
		m_data_start = 0;
		return;
	}

	reader.seekg(0, ios::end);
	size_t file_size = reader.tellg();
	if (file_size == 0) {
		m_num_keys = 0;
		m_data_start = 0;
		return;
	}

	reader.seekg(0, ios::beg);
	reader.read(buffer, 8);

	m_num_keys = *((uint64_t *)(&buffer[0]));

	if (m_num_keys > FULL_TEXT_MAX_KEYS) {
		throw error("Number of keys in file exceeeds maximum: file: " + filename() + " num: " + to_string(m_num_keys));
	}

	char *vector_buffer = new char[m_num_keys * 8];

	// Read the keys.
	reader.read(vector_buffer, m_num_keys * 8);
	for (size_t i = 0; i < m_num_keys; i++) {
		m_keys.push_back(*((size_t *)(&vector_buffer[i*8])));
	}
	delete vector_buffer;

	m_pos_start = reader.tellg();


	m_len_start = m_pos_start + m_num_keys * 8;
	m_total_start = m_len_start + m_num_keys * 8;
	m_data_start = m_total_start + m_num_keys * 8;

}

string FullTextShard::filename() const {
	return m_filename;
}

size_t FullTextShard::shard_id() const {
	return m_shard_id;
}

size_t FullTextShard::disk_size() const {
	return m_keys.size();
}

