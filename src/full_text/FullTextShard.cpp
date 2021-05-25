
#include "FullTextShard.h"
#include <cstring>

/*
File format explained

8 bytes = unsigned int number of keys = num_keys
8 bytes * num_keys = list of keys
8 bytes * num_keys = list of positions in file counted from data start
8 bytes * num_keys = list of lengths
[DATA]

*/

FullTextShard::FullTextShard(const string &db_name, size_t shard)
: m_shard(shard), m_db_name(db_name) {
	m_buffer = new char[m_buffer_len];
	
	m_writer.open(filename(), ios::binary | ios::app);
	if (!m_writer.is_open()) {
		throw runtime_error("Could not open full text shard. Error: " + string(strerror(errno)));
	}
	m_writer.close();

	m_reader.open(filename(), ios::binary);
	if (!m_reader.is_open()) {
		throw runtime_error("Could not open full text shard. Error: " + string(strerror(errno)));
	}

	read_file();
}

FullTextShard::~FullTextShard() {
	m_reader.close();
	delete m_buffer;
}

void FullTextShard::add(uint64_t key, uint64_t value, uint32_t score) {
	m_cache[key].emplace_back(FullTextResult(value, score));
}

void FullTextShard::sort_cache() {
	for (auto &iter : m_cache) {
		sort(iter.second.begin(), iter.second.end(), [](const FullTextResult &a, const FullTextResult &b) {
			return a.m_score > b.m_score;
		});
	}
}

vector<FullTextResult> FullTextShard::find(uint64_t key) const {

	vector<FullTextResult> cached = find_cached(key);
	vector<FullTextResult> stored = find_stored(key);

	// Merge.
	cached.insert(cached.end(), stored.begin(), stored.end());

	return cached;
}

void FullTextShard::save_file() {

	read_data_to_cache();

	m_writer.open(filename(), ios::binary | ios::trunc);
	if (!m_writer.is_open()) {
		throw runtime_error("Could not open full text shard. Error: " + string(strerror(errno)));
	}

	cout << "Writing shard: " << filename() << " with num keys: " << m_keys.size() << endl;

	m_keys.clear();
	for (auto &iter : m_cache) {
		m_keys.push_back(iter.first);
	}
	
	sort(m_keys.begin(), m_keys.end(), [](const size_t a, const size_t b) {
		return a < b;
	});

	m_num_keys = m_keys.size();

	m_writer.write((char *)&m_num_keys, 8);
	m_writer.write((char *)m_keys.data(), m_keys.size() * 8);

	vector<size_t> v_pos;
	vector<size_t> v_len;

	size_t pos = 0;
	for (uint64_t key : m_keys) {
		size_t len = m_cache[key].size() * FULL_TEXT_RECORD_LEN;
		
		v_pos.push_back(pos);
		v_len.push_back(len);

		pos += len;
	}
	
	m_writer.write((char *)v_pos.data(), m_keys.size() * 8);
	m_writer.write((char *)v_len.data(), m_keys.size() * 8);

	const size_t buffer_num_records = 1000;
	const size_t buffer_len = FULL_TEXT_RECORD_LEN * buffer_num_records;
	char buffer[buffer_len];

	// Write data.
	for (uint64_t key : m_keys) {
		size_t i = 0;
		sort(m_cache[key].begin(), m_cache[key].end(), [](const FullTextResult &a, const FullTextResult &b) {
			return a.m_score > b.m_score;
		});
		for (const FullTextResult &res : m_cache[key]) {
			memcpy(&buffer[i], &res.m_value, FULL_TEXT_KEY_LEN);
			memcpy(&buffer[i + FULL_TEXT_KEY_LEN], &res.m_score, FULL_TEXT_SCORE_LEN);
			i += FULL_TEXT_RECORD_LEN;
			if (i == buffer_len) {
				m_writer.write(buffer, buffer_len);
				i = 0;
			}
		}
		if (i) {
			m_writer.write(buffer, i);
		}
	}

	m_writer.close();
	m_cache.clear();

	read_file();
}

void FullTextShard::read_file() {

	m_keys.clear();
	m_pos.clear();
	m_len.clear();
	m_num_keys = 0;

	char buffer[64];

	m_reader.seekg(0, ios::end);
	size_t file_size = m_reader.tellg();
	if (file_size == 0) {
		m_num_keys = 0;
		m_data_start = 0;
		return;
	}

	m_reader.seekg(0, ios::beg);
	m_reader.read(buffer, 8);

	m_num_keys = *((uint64_t *)(&buffer[0]));

	if (m_num_keys > FULL_TEXT_MAX_KEYS) {
		throw runtime_error("Number of keys in file exceeeds maximum: " + to_string(m_num_keys));
	}

	char *vector_buffer = new char[m_num_keys * 8];

	// Read positions.
	m_pos.clear();
	
	m_reader.read(vector_buffer, m_num_keys * 8);
	for (size_t i = 0; i < m_num_keys; i++) {
		m_keys.push_back(*((size_t *)(&vector_buffer[i*8])));
	}

	m_reader.read(vector_buffer, m_num_keys * 8);
	for (size_t i = 0; i < m_num_keys; i++) {
		m_pos[m_keys[i]] = *((size_t *)(&vector_buffer[i*8]));
	}

	m_reader.read(vector_buffer, m_num_keys * 8);
	for (size_t i = 0; i < m_num_keys; i++) {
		m_len[m_keys[i]] = *((size_t *)(&vector_buffer[i*8]));
	}

	m_data_start = m_reader.tellg();
}

string FullTextShard::filename() const {
	return "/mnt/fti_" + m_db_name + "_" + to_string(m_shard) + ".idx";
}

size_t FullTextShard::size() const {
	return m_keys.size();
}

void FullTextShard::truncate() {
	m_cache.clear();
	m_keys.clear();
	m_num_keys = 0;

	m_writer.open(filename(), ios::trunc);
	if (!m_writer.is_open()) {
		throw runtime_error("Could not open full text shard. Error: " + string(strerror(errno)));
	}
	m_writer.close();
}

vector<FullTextResult> FullTextShard::find_cached(uint64_t key) const {
	auto cached = m_cache.find(key);
	if (cached != m_cache.end()) {
		return cached->second;
	}
	return {};
}

vector<FullTextResult> FullTextShard::find_stored(uint64_t key) const {
	auto iter = m_pos.find(key);
	if (iter == m_pos.end()) {
		return {};
	}

	vector<FullTextResult> ret;

	size_t pos = iter->second;
	size_t len = m_len.find(key)->second;

	m_reader.seekg(m_data_start + pos, ios::beg);

	size_t read_bytes = 0;
	while (read_bytes < len) {
		size_t read_len = min(m_buffer_len, len);
		m_reader.read(m_buffer, read_len);
		read_bytes += read_len;

		size_t num_records = read_len / FULL_TEXT_RECORD_SIZE;
		for (size_t i = 0; i < num_records; i++) {
			const uint64_t value = *((uint64_t *)&m_buffer[i*FULL_TEXT_RECORD_SIZE]);
			const uint32_t score = *((uint32_t *)&m_buffer[i*FULL_TEXT_RECORD_SIZE]);
			ret.emplace_back(FullTextResult(value, score));
		}
	}

	return ret;
}

void FullTextShard::read_data_to_cache() {

	// Read all the data to file.

	m_reader.seekg(m_data_start, ios::beg);

	for (uint64_t key : m_keys) {
		size_t len = m_len[key];
		size_t read_bytes = 0;
		while (read_bytes < len) {
			size_t read_len = min(m_buffer_len, len);
			m_reader.read(m_buffer, read_len);
			read_bytes += read_len;

			size_t num_records = read_len / FULL_TEXT_RECORD_SIZE;
			for (size_t i = 0; i < num_records; i++) {
				const uint64_t value = *((uint64_t *)&m_buffer[i*FULL_TEXT_RECORD_SIZE]);
				const uint32_t score = *((uint32_t *)&m_buffer[i*FULL_TEXT_RECORD_SIZE]);
				m_cache[key].emplace_back(FullTextResult(value, score));
			}
		}
	}
}
