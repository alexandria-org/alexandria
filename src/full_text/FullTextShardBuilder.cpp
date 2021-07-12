
#include "FullTextShardBuilder.h"
#include "FullTextIndex.h"
#include <cstring>
#include "system/Logger.h"

FullTextShardBuilder::FullTextShardBuilder(const string &db_name, size_t shard_id)
: m_db_name(db_name), m_shard_id(shard_id) {
	m_input.push_back(new struct ShardInput[m_max_cache_size]);
	m_input_position = 0;
}

FullTextShardBuilder::~FullTextShardBuilder() {
	if (m_reader.is_open()) {
		m_reader.close();
	}
	for (struct ShardInput *input : m_input) {
		delete input;
	}
}

void FullTextShardBuilder::add(uint64_t key, uint64_t value, float score) {
	struct ShardInput *input = m_input.back();
	input[m_input_position] = ShardInput{.key = key, .value = value, .score = score};
	m_input_position++;
	if (m_input_position >= m_max_cache_size) {
		m_input.push_back(new struct ShardInput[m_max_cache_size]);
		m_input_position = 0;
	}
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
	return cache_size() > m_max_cache_size;
}

void FullTextShardBuilder::append() {
	m_writer.open(filename(), ios::binary | ios::app);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " +
			string(strerror(errno)));
	}

	while (m_input.size() > 1) {
		struct ShardInput *input = m_input.back();
		m_input.pop_back();
		m_writer.write((const char *)input, m_input_position * sizeof(struct ShardInput));
		m_input_position = m_max_cache_size;
		delete input;
	}
	m_writer.write((const char *)m_input[0], m_input_position * sizeof(struct ShardInput));
	m_input_position = 0;

	m_writer.close();
}

void FullTextShardBuilder::merge() {

	m_cache.clear();
	m_total_results.clear();

	// Read the current file.
	read_data_to_cache();

	// Read the cache into memory.
	m_reader.open(filename(), ios::binary);
	if (!m_reader.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " + string(strerror(errno)));
	}

	const size_t buffer_len = 100000;
	const size_t buffer_size = sizeof(struct ShardInput) * buffer_len;
	char *buffer = new char[buffer_size];

	m_reader.seekg(0, ios::beg);

	while (!m_reader.eof()) {

		m_reader.read(buffer, buffer_size);

		const size_t read_bytes = m_reader.gcount();
		const size_t num_records = read_bytes / sizeof(struct ShardInput);

		for (size_t i = 0; i < num_records; i++) {
			struct ShardInput *item = (struct ShardInput *)&buffer[i * sizeof(struct ShardInput)];

			m_cache[item->key].emplace_back(FullTextResult(item->value, item->score));
		}
	}
	m_reader.close();

	delete buffer;

	sort_cache();

	save_file();

	truncate_cache_files();
}

bool FullTextShardBuilder::should_merge() {

	m_writer.open(filename(), ios::binary | ios::app);
	size_t cache_file_size = m_writer.tellp();
	m_writer.close();

	return cache_file_size > m_max_cache_file_size;
}

void FullTextShardBuilder::add_adjustments(const AdjustmentList &adjustments) {

	ofstream domain_writer(domain_adjustment_filename(), ios::binary | ios::app);
	ofstream url_writer(url_adjustment_filename(), ios::binary | ios::app);

	vector<Adjustment> data = adjustments.data();

	for (const struct Adjustment &record : data) {
		if (record.type == DOMAIN_ADJUSTMENT) {
			domain_writer.write((const char *)&record, sizeof(struct Adjustment));
		}
		if (record.type == URL_ADJUSTMENT) {
			url_writer.write((const char *)&record, sizeof(struct Adjustment));
		}
	}
}

void FullTextShardBuilder::merge_adjustments(const FullTextIndexer *indexer) {

	if (m_shard_id == 5772) {
		cout << "asd" << endl;
	}

	// Read everything to cache.
	read_data_to_cache();

	// Allocate buffer.
	const size_t buffer_size = 10000;
	char *buffer = new char[buffer_size * sizeof(struct Adjustment)];

	// Apply url adjustments.
	{
		ifstream reader(url_adjustment_filename(), ios::binary);
		while (!reader.eof()) {
			reader.read(buffer, buffer_size);
			const size_t read_bytes = reader.gcount();

			for (size_t i = 0; i < read_bytes; i += sizeof(struct Adjustment)) {
				struct Adjustment *adjustment = (struct Adjustment *)(&buffer[i]);
				auto end = m_cache[adjustment->word_hash].end();
				auto iter = lower_bound(m_cache[adjustment->word_hash].begin(), end,
					adjustment->key_hash);
				if (iter == end || (*iter).m_value != adjustment->key_hash) {
					// Add the item.
					cout << "adding url to shard: " << target_filename() << endl;
					m_cache[adjustment->word_hash].push_back(FullTextResult(adjustment->key_hash,
						adjustment->domain_harmonic + adjustment->score));
					sort(m_cache[adjustment->word_hash].begin(), m_cache[adjustment->word_hash].end());
				} else {
					cout << "updating url to shard: " << target_filename() << endl;
					(*iter).m_score += adjustment->score;
				}
			}
		}
	}

	// Apply domain adjustments.
	{
		const unordered_map<uint64_t, uint64_t> *url_to_domain = indexer->url_to_domain();
		ifstream reader(domain_adjustment_filename(), ios::binary);
		while (!reader.eof()) {
			reader.read(buffer, buffer_size);
			const size_t read_bytes = reader.gcount();

			for (size_t i = 0; i < read_bytes; i += sizeof(struct Adjustment)) {
				struct Adjustment *adjustment = (struct Adjustment *)(&buffer[i]);

				for (auto &iter : m_cache[adjustment->word_hash]) {
					auto find_iter = url_to_domain->find(iter.m_value);
					if (find_iter != url_to_domain->end() && find_iter->second == adjustment->key_hash) {
						iter.m_score += adjustment->score;
						cout << "updating domain to shard: " << target_filename() << endl;
					}
				}
			}
		}
	}

	delete buffer;

	sort_cache();
	save_file();
	truncate_cache_files();
}

void FullTextShardBuilder::read_data_to_cache() {

	m_cache.clear();
	m_total_results.clear();

	ifstream reader(target_filename(), ios::binary);

	if (!reader.is_open()) return;

	reader.seekg(0, ios::end);
	const size_t file_size = reader.tellg();

	if (file_size == 0) return;

	char buffer[64];

	reader.seekg(0, ios::beg);
	reader.read(buffer, 8);

	uint64_t num_keys = *((uint64_t *)(&buffer[0]));

	if (num_keys > FULL_TEXT_MAX_KEYS) {
		throw error("Number of keys in file exceeeds maximum: file: " + filename() + " num: " + to_string(num_keys));
	}

	char *vector_buffer = new char[num_keys * 8];

	// Read the keys.
	reader.read(vector_buffer, num_keys * 8);
	vector<uint64_t> keys;
	for (size_t i = 0; i < num_keys; i++) {
		keys.push_back(*((uint64_t *)(&vector_buffer[i*8])));
	}

	// Read the positions.
	reader.read(vector_buffer, num_keys * 8);
	vector<size_t> positions;
	for (size_t i = 0; i < num_keys; i++) {
		positions.push_back(*((size_t *)(&vector_buffer[i*8])));
	}

	// Read the lengths.
	reader.read(vector_buffer, num_keys * 8);
	vector<size_t> lens;
	size_t data_size = 0;
	for (size_t i = 0; i < num_keys; i++) {
		size_t len = *((size_t *)(&vector_buffer[i*8]));
		lens.push_back(len);
		data_size += len;
	}

	// Read the totals.
	reader.read(vector_buffer, num_keys * 8);
	for (size_t i = 0; i < num_keys; i++) {
		size_t total = *((size_t *)(&vector_buffer[i*8]));
		m_total_results[keys[i]] = total;
	}
	delete vector_buffer;

	if (data_size == 0) return;

	m_buffer = new char[m_buffer_len];

	// Read the data.
	size_t total_read_data = 0;
	size_t key_id = 0;
	size_t key_data_len = lens[key_id];
	while (total_read_data < data_size) {
		reader.read(m_buffer, m_buffer_len);
		const size_t read_len = reader.gcount();
		total_read_data += read_len;

		size_t num_records = read_len / FULL_TEXT_RECORD_LEN;
		for (size_t i = 0; i < num_records; i++) {
			if (key_data_len == 0) {
				key_id++;
				key_data_len = lens[key_id];
			}
			
			const uint64_t value = *((uint64_t *)&m_buffer[i*FULL_TEXT_RECORD_LEN]);
			const float score = *((float *)&m_buffer[i*FULL_TEXT_RECORD_LEN + FULL_TEXT_KEY_LEN]);
			m_cache[keys[key_id]].emplace_back(FullTextResult(value, score));

			key_data_len--;
		}
	}

	delete m_buffer;

}

void FullTextShardBuilder::save_file() {

	vector<uint64_t> keys;

	const string filename = target_filename();

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
	vector<size_t> v_tot;

	size_t pos = 0;
	for (uint64_t key : keys) {

		// Store position and length
		size_t len = m_cache[key].size() * FULL_TEXT_RECORD_LEN;
		
		v_pos.push_back(pos);
		v_len.push_back(len);
		v_tot.push_back(m_total_results[key]);

		pos += len;
	}
	
	m_writer.write((char *)v_pos.data(), keys.size() * 8);
	m_writer.write((char *)v_len.data(), keys.size() * 8);
	m_writer.write((char *)v_tot.data(), keys.size() * 8);

	const size_t buffer_num_records = 1000;
	const size_t buffer_len = FULL_TEXT_RECORD_LEN * buffer_num_records;
	char buffer[buffer_len];

	// Write data.
	hash<string> hasher;
	for (uint64_t key : keys) {
		size_t i = 0;

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
	return "/mnt/" + (to_string(m_shard_id % 8)) + "/output/precache_" + m_db_name + "_" + to_string(m_shard_id) +
		".fti";
}

string FullTextShardBuilder::target_filename() const {
	return "/mnt/" + to_string(m_shard_id % 8) + "/full_text/fti_" + m_db_name + "_" + to_string(m_shard_id) + ".idx";
}

string FullTextShardBuilder::domain_adjustment_filename() const {
	return "/mnt/" + to_string(m_shard_id % 8) + "/full_text/fti_" + m_db_name + "_" + to_string(m_shard_id) + ".adj_dom";
}

string FullTextShardBuilder::url_adjustment_filename() const {
	return "/mnt/" + to_string(m_shard_id % 8) + "/full_text/fti_" + m_db_name + "_" + to_string(m_shard_id) + ".adj_url";
}

/*
	Deletes ALL data from this shard.
*/
void FullTextShardBuilder::truncate() {

	m_cache.clear();

	m_writer.open(filename(), ios::trunc);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard. Error: " + string(strerror(errno)));
	}
	m_writer.close();

	ofstream domain_writer(domain_adjustment_filename(), ios::trunc);
	domain_writer.close();

	ofstream url_writer(url_adjustment_filename(), ios::trunc);
	url_writer.close();

	ofstream target_writer(target_filename(), ios::trunc);
	target_writer.close();
}

/*
	Deletes all data from caches.
*/
void FullTextShardBuilder::truncate_cache_files() {

	m_cache.clear();

	m_writer.open(filename(), ios::trunc);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard. Error: " + string(strerror(errno)));
	}
	m_writer.close();

	ofstream domain_writer(domain_adjustment_filename(), ios::trunc);
	domain_writer.close();

	ofstream url_writer(url_adjustment_filename(), ios::trunc);
	url_writer.close();
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
	return m_input_position + (m_input.size() - 1) * m_max_cache_size;
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
			float score = *((float *)(&buffer[0]));

			if (key == for_key) {
				num_found++;
			}
		}
		if (m_reader.eof()) break;
	}
	m_reader.close();

	return num_found;
}

