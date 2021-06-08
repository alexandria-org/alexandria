
#include "FullTextShardBuilder.h"
#include <cstring>
#include "system/Logger.h"

FullTextShardBuilder::FullTextShardBuilder(const string &file_name)
: m_filename(file_name) {
}

FullTextShardBuilder::~FullTextShardBuilder() {
	if (m_reader.is_open()) {
		m_reader.close();
	}
}

void FullTextShardBuilder::add(uint64_t key, uint64_t value, uint32_t score) {
	m_cache[key].emplace_back(FullTextResult(value, score));
}

void FullTextShardBuilder::sort_cache() {
	for (auto &iter : m_cache) {
		sort(iter.second.begin(), iter.second.end(), [](const FullTextResult &a, const FullTextResult &b) {
			return a.m_score > b.m_score;
		});
		size_t max_items = min(m_max_results, iter.second.size());
		vector<FullTextResult> results(iter.second.begin(), iter.second.begin() + max_items);

		sort(results.begin(), results.end(), [](const FullTextResult &a, const FullTextResult &b) {
			return a.m_value < b.m_value;
		});
		m_cache[iter.first] = results;
	}
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

void FullTextShardBuilder::save_file(const string &db_name, size_t shard_id) {

	vector<uint64_t> keys;

	const string filename = "/mnt/fti_" + db_name + "_" + to_string(shard_id) + ".idx";

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

		// Make elements unique.
		sort(m_cache[key].begin(), m_cache[key].end(), [](const FullTextResult &a, const FullTextResult &b) {
			return a.m_value < b.m_value;
		});
		auto last = unique(m_cache[key].begin(), m_cache[key].end());
		m_cache[key].erase(last, m_cache[key].end());

		// Store position and length
		size_t len = m_cache[key].size() * FULL_TEXT_RECORD_LEN;
		
		v_pos.push_back(pos);
		v_len.push_back(len);

		pos += len;
	}
	
	m_writer.write((char *)v_pos.data(), keys.size() * 8);
	m_writer.write((char *)v_len.data(), keys.size() * 8);

	const size_t buffer_num_records = 1000;
	const size_t buffer_len = FULL_TEXT_RECORD_LEN * buffer_num_records;
	char buffer[buffer_len];

	// Write data.
	for (uint64_t key : keys) {
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
}

string FullTextShardBuilder::filename() const {
	return m_filename;
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

