
#include "FullTextShardBuilder.h"
#include "FullTextIndex.h"
#include <cstring>
#include "system/Logger.h"

FullTextShardBuilder::FullTextShardBuilder(const string &file_name)
: m_filename(file_name) {
	m_buffer = new char[m_buffer_len];
}

FullTextShardBuilder::~FullTextShardBuilder() {
	if (m_reader.is_open()) {
		m_reader.close();
	}
	delete m_buffer;
}

void FullTextShardBuilder::add(uint64_t key, uint64_t value, uint32_t score) {
	m_cache[key].emplace_back(FullTextResult(value, score));
}

void FullTextShardBuilder::sort_cache() {
	for (auto &iter : m_cache) {
		// Make elements unique.
		sort(iter.second.begin(), iter.second.end(), [](const FullTextResult &a, const FullTextResult &b) {
			return a.m_value < b.m_value;
		});
		auto last = unique(iter.second.begin(), iter.second.end());
		iter.second.erase(last, iter.second.end());

		m_total_results[iter.first] = iter.second.size();

		// Cap at m_max_results
		if (iter.second.size() > m_max_results) {
			sort(iter.second.begin(), iter.second.end(), [](const FullTextResult &a, const FullTextResult &b) {
				return a.m_score > b.m_score;
			});
			iter.second.resize(m_max_results);

			// Order by value again.
			sort(iter.second.begin(), iter.second.end(), [](const FullTextResult &a, const FullTextResult &b) {
				return a.m_value < b.m_value;
			});
		}

	}
}

bool FullTextShardBuilder::full() const {
	return cache_size() > FT_INDEXER_MAX_CACHE_SIZE;
}

void FullTextShardBuilder::append() {
	m_writer.open(filename(), ios::binary | ios::app);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " +
			string(strerror(errno)));
	}

	for (const auto &iter : m_cache) {
		size_t len = iter.second.size();
		m_writer.write((const char *)&iter.first, FULL_TEXT_KEY_LEN);
		m_writer.write((const char *)&len, sizeof(size_t));
		for (const FullTextResult &res : iter.second) {
			m_writer.write((const char *)&res.m_value, FULL_TEXT_KEY_LEN);
			m_writer.write((const char *)&res.m_score, FULL_TEXT_SCORE_LEN);
		}
	}

	m_writer.close();

	m_cache.clear();
}

void FullTextShardBuilder::merge(const string &db_name, size_t shard_id) {
	m_cache.clear();
	m_total_results.clear();
	// Read the whole cache.
	m_reader.open(filename(), ios::binary);
	if (!m_reader.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " + string(strerror(errno)));
	}

	char buffer[64];

	m_reader.seekg(0, ios::end);
	size_t file_size = m_reader.tellg();
	m_reader.seekg(0, ios::beg);

	while (!m_reader.eof()) {
		m_reader.read(buffer, FULL_TEXT_KEY_LEN);
		uint64_t key = *((uint64_t *)(&buffer[0]));

		if (m_reader.eof()) {
			break;
		}

		m_reader.read(buffer, sizeof(size_t));
		size_t len = *((size_t *)(&buffer[0]));

		for (size_t i = 0; i < len; i++) {
			m_reader.read(buffer, FULL_TEXT_KEY_LEN);
			uint64_t value = *((uint64_t *)(&buffer[0]));

			m_reader.read(buffer, FULL_TEXT_SCORE_LEN);
			uint32_t score = *((uint32_t *)(&buffer[0]));

			m_cache[key].emplace_back(FullTextResult(value, score));
		}
		if (m_reader.eof()) break;
	}
	m_reader.close();

	sort_cache();

	save_file(db_name, shard_id);

	truncate();
}

void FullTextShardBuilder::apply_adjustment(const string &db_name, size_t shard_id,
		const unordered_map<uint64_t, uint64_t> &url_domain_map, FullTextAdjustment &adjustments) {

	hash<string> thasher;

	Profiler pf("FullTextShardBuilder::adjust_score_for_domain_link");

	ifstream reader(target_filename(db_name, shard_id), ios::binary);
	ofstream writer(target_filename(db_name, shard_id), ios::in | ios::out | ios::binary);

	char buffer[64];

	reader.seekg(0, ios::beg);
	reader.read(buffer, 8);

	uint64_t num_keys = *((uint64_t *)(&buffer[0]));

	char *vector_buffer = new char[num_keys * 8];

	// Read the keys.
	reader.read(vector_buffer, num_keys * 8);
	vector<uint64_t> keys;
	for (size_t i = 0; i < num_keys; i++) {
		keys.push_back(*((uint64_t *)(&vector_buffer[i*8])));
	}

	vector<size_t> positions;
	size_t pos_start = reader.tellg();
	reader.read(vector_buffer, num_keys * 8);
	for (size_t i = 0; i < num_keys; i++) {
		positions.push_back(*((size_t *)(&vector_buffer[i*8])));
	}

	vector<size_t> lens;
	size_t len_start = reader.tellg();
	reader.read(vector_buffer, num_keys * 8);
	for (size_t i = 0; i < num_keys; i++) {
		lens.push_back(*((size_t *)(&vector_buffer[i*8])));
	}
	delete vector_buffer;

	size_t data_start = len_start + num_keys * 8;

	reader.seekg(data_start, ios::beg);

	size_t domhash = thasher("users.lawschoolnumbers.com");

	Profiler pf2("FullTextShardBuilder::adjust_score_for_domain_link final loop");

	auto host_adjustments = adjustments.host_adjustments();
	
	for (size_t key_num = 0; key_num < keys.size(); key_num++) {
		const uint64_t key = keys[key_num];
		const size_t pos = positions[key_num];
		const size_t len = lens[key_num];

		if (host_adjustments.count(key) > 0) {

			// We have adjustments for this key.
			reader.seekg(data_start + pos, ios::beg);

			// Read total number of results.
			size_t total_num_results;
			reader.read((char *)&total_num_results, sizeof(size_t));

			const size_t num_records = len / FULL_TEXT_RECORD_LEN;

			size_t read_bytes = 0;
			while (read_bytes < len) {
				size_t read_len = min(m_buffer_len, len);
				size_t read_start = reader.tellg();
				reader.read(m_buffer, read_len);
				read_bytes += read_len;

				size_t num_records_read = read_len / FULL_TEXT_RECORD_LEN;
				bool did_change = false;
				for (size_t i = 0; i < num_records_read; i++) {
					const uint64_t url_hash = *((uint64_t *)&m_buffer[i*FULL_TEXT_RECORD_LEN]);
					const uint32_t score = *((uint32_t *)&m_buffer[i*FULL_TEXT_RECORD_LEN + FULL_TEXT_KEY_LEN]);

					auto url_domain_iter = url_domain_map.find(url_hash);
					if (url_domain_iter == url_domain_map.end()) {
						throw error("Could not find url in url_domain_map");
					}

					auto find_iter = host_adjustments.find(key);
					uint32_t new_score = score;
					for (const auto &iter : find_iter->second) {
						if (url_domain_iter->second == iter.first) {
							// Adjust score.
							new_score += 1000*iter.second.size();
						}
					}
					if (score != new_score) {
						did_change = true;
						memcpy(&m_buffer[i*FULL_TEXT_RECORD_LEN + FULL_TEXT_KEY_LEN], &new_score, sizeof(uint32_t));
					}
				}

				if (did_change) {
					writer.seekp(read_start, ios::beg);
					writer.write(m_buffer, read_len);
				}
			}
		}
	}

	writer.close();
}

void FullTextShardBuilder::save_file(const string &db_name, size_t shard_id) {

	vector<uint64_t> keys;

	const string filename = target_filename(db_name, shard_id);

	m_writer.open(filename, ios::binary | ios::trunc);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard. Error: " + string(strerror(errno)));
	}

	keys.clear();
	for (auto &iter : m_cache) {
		keys.push_back(iter.first);
	}
	
	sort(keys.begin(), keys.end(), [](const size_t a, const size_t b) {
		return a < b;
	});

	size_t num_keys = keys.size();

	m_writer.write((char *)&num_keys, 8);
	m_writer.write((char *)keys.data(), keys.size() * 8);

	vector<size_t> v_pos;
	vector<size_t> v_len;

	size_t pos = 0;
	for (uint64_t key : keys) {

		// Store position and length
		size_t len = m_cache[key].size() * FULL_TEXT_RECORD_LEN;
		
		v_pos.push_back(pos);
		v_len.push_back(len);

		pos += len + sizeof(size_t);
	}
	
	m_writer.write((char *)v_pos.data(), keys.size() * 8);
	m_writer.write((char *)v_len.data(), keys.size() * 8);

	const size_t buffer_num_records = 1000;
	const size_t buffer_len = FULL_TEXT_RECORD_LEN * buffer_num_records;
	char buffer[buffer_len];

	// Write data.
	hash<string> hasher;
	for (uint64_t key : keys) {
		size_t i = 0;

		m_writer.write((char *)&(m_total_results[key]), sizeof(size_t));

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
}

string FullTextShardBuilder::filename() const {
	return m_filename;
}

string FullTextShardBuilder::target_filename(const string &db_name, size_t shard_id) const {
	return "/mnt/"+to_string(shard_id % 8)+"/full_text/fti_" + db_name + "_" + to_string(shard_id) + ".idx";
}

void FullTextShardBuilder::truncate() {

	m_cache.clear();

	m_writer.open(filename(), ios::trunc);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard. Error: " + string(strerror(errno)));
	}
	m_writer.close();
}

size_t FullTextShardBuilder::disk_size() const {

	m_reader.open(filename(), ios::binary);
	if (!m_reader.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " + string(strerror(errno)));
	}

	m_reader.seekg(0, ios::end);
	size_t file_size = m_reader.tellg();
	m_reader.close();
	return file_size;
}

size_t FullTextShardBuilder::cache_size() const {
	return m_cache.size();
}

size_t FullTextShardBuilder::count_keys(uint64_t for_key) const {
	m_reader.open(filename(), ios::binary);
	if (!m_reader.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " + string(strerror(errno)));
	}

	char buffer[64];

	m_reader.seekg(0, ios::end);
	size_t file_size = m_reader.tellg();
	m_reader.seekg(0, ios::beg);

	size_t num_found = 0;
	while (!m_reader.eof()) {
		m_reader.read(buffer, FULL_TEXT_KEY_LEN);
		uint64_t key = *((uint64_t *)(&buffer[0]));

		if (m_reader.eof()) {
			break;
		}

		m_reader.read(buffer, sizeof(size_t));
		size_t len = *((size_t *)(&buffer[0]));

		for (size_t i = 0; i < len; i++) {
			m_reader.read(buffer, FULL_TEXT_KEY_LEN);
			uint64_t value = *((uint64_t *)(&buffer[0]));

			m_reader.read(buffer, FULL_TEXT_SCORE_LEN);
			uint32_t score = *((uint32_t *)(&buffer[0]));

			if (key == for_key) {
				num_found++;
			}
		}
		if (m_reader.eof()) break;
	}
	m_reader.close();

	return num_found;
}

