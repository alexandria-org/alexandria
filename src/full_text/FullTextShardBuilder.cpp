
#include "FullTextShardBuilder.h"
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
		sort(iter.second.begin(), iter.second.end(), [](const FullTextResult &a, const FullTextResult &b) {
			return a.m_score > b.m_score;
		});
	}
}

void FullTextShardBuilder::append_precache() {
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

void FullTextShardBuilder::merge_precache() {
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

	truncate();

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

