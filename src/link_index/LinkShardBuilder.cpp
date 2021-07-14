
#include "LinkShardBuilder.h"
#include "LinkIndex.h"
#include <cstring>
#include "system/Logger.h"

LinkShardBuilder::LinkShardBuilder(const string &db_name, size_t shard_id)
: m_db_name(db_name), m_shard_id(shard_id) {
	m_input.push_back(new struct LinkShardInput[m_max_cache_size]);
	m_input_position = 0;
}

LinkShardBuilder::~LinkShardBuilder() {
	if (m_reader.is_open()) {
		m_reader.close();
	}
	for (struct LinkShardInput *input : m_input) {
		delete input;
	}
}

void LinkShardBuilder::add(uint64_t word_key, uint64_t link_hash, uint64_t source, uint64_t target,
	uint64_t source_domain, uint64_t target_domain, float score) {

	struct LinkShardInput *input = m_input.back();

	input[m_input_position] = LinkShardInput{
		.key = word_key,
		.link_hash = link_hash,
		.source_hash = source,
		.target = target,
		.source_domain = source_domain,
		.target_domain = target_domain,
		.score = score
	};
	m_input_position++;
	if (m_input_position >= m_max_cache_size) {
		m_input.push_back(new struct LinkShardInput[m_max_cache_size]);
		m_input_position = 0;
	}
}

void LinkShardBuilder::sort_cache() {
	for (auto &iter : m_cache) {
		// Make elements unique.
		sort(iter.second.begin(), iter.second.end(), [](const LinkResult &a, const LinkResult &b) {
			return a.m_link_hash < b.m_link_hash;
		});
		auto last = unique(iter.second.begin(), iter.second.end(), [](const LinkResult &a, const LinkResult &b) {
			return a.m_link_hash == b.m_link_hash;
		});
		iter.second.erase(last, iter.second.end());

		m_total_results[iter.first] = iter.second.size();

		// Cap at m_max_results
		if (iter.second.size() > m_max_results) {
			sort(iter.second.begin(), iter.second.end(), [](const LinkResult &a, const LinkResult &b) {
				return a.m_score > b.m_score;
			});
			iter.second.resize(m_max_results);

			// Order by value again.
			sort(iter.second.begin(), iter.second.end(), [](const LinkResult &a, const LinkResult &b) {
				return a.m_link_hash < b.m_link_hash;
			});
		}
	}
}

bool LinkShardBuilder::full() const {
	return cache_size() > LI_INDEXER_MAX_CACHE_SIZE;
}

void LinkShardBuilder::append() {
	m_writer.open(filename(), ios::binary | ios::app);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " +
			string(strerror(errno)));
	}

	while (m_input.size() > 1) {
		struct LinkShardInput *input = m_input.back();
		m_input.pop_back();
		m_writer.write((const char *)input, m_input_position * sizeof(struct LinkShardInput));
		m_input_position = m_max_cache_size;
		delete input;
	}
	m_writer.write((const char *)m_input[0], m_input_position * sizeof(struct LinkShardInput));
	m_input_position = 0;

	m_writer.close();
}

void LinkShardBuilder::merge() {

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
	const size_t buffer_size = sizeof(struct LinkShardInput) * buffer_len;
	char *buffer = new char[buffer_size];

	m_reader.seekg(0, ios::beg);

	while (!m_reader.eof()) {

		m_reader.read(buffer, buffer_size);

		const size_t read_bytes = m_reader.gcount();
		const size_t num_records = read_bytes / sizeof(struct LinkShardInput);

		for (size_t i = 0; i < num_records; i++) {
			struct LinkShardInput *item = (struct LinkShardInput *)&buffer[i * sizeof(struct LinkShardInput)];

			m_cache[item->key].emplace_back(*item);
		}
	}
	m_reader.close();

	delete buffer;

	sort_cache();

	save_file();

	truncate_cache_files();
}

bool LinkShardBuilder::should_merge() {

	m_writer.open(filename(), ios::binary | ios::app);
	size_t cache_file_size = m_writer.tellp();
	m_writer.close();

	return cache_file_size > m_max_cache_file_size;
}

void LinkShardBuilder::read_data_to_cache() {

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

		size_t num_records = read_len / sizeof(struct LinkShardInput);
		for (size_t i = 0; i < num_records; i++) {
			if (key_data_len == 0) {
				key_id++;
				key_data_len = lens[key_id];
			}
			
			const struct LinkShardInput *item = ((struct LinkShardInput *)&m_buffer[i*sizeof(struct LinkShardInput)]);
			m_cache[keys[key_id]].emplace_back(*item);

			key_data_len--;
		}
	}

	delete m_buffer;

}

void LinkShardBuilder::save_file() {

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
		size_t len = m_cache[key].size() * sizeof(struct LinkShardInput);
		
		v_pos.push_back(pos);
		v_len.push_back(len);
		v_tot.push_back(m_total_results[key]);

		pos += len + sizeof(size_t);
	}
	
	m_writer.write((char *)v_pos.data(), keys.size() * 8);
	m_writer.write((char *)v_len.data(), keys.size() * 8);
	m_writer.write((char *)v_tot.data(), keys.size() * 8);

	const size_t buffer_num_records = 1000;
	const size_t buffer_len = sizeof(struct LinkShardInput) * buffer_num_records;
	char buffer[buffer_len];

	// Write data.
	hash<string> hasher;
	for (uint64_t key : keys) {
		size_t i = 0;

		for (const struct LinkShardInput &res : m_cache[key]) {
			memcpy(&buffer[i], &res, sizeof(struct LinkShardInput));
			i += sizeof(struct LinkShardInput);
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

string LinkShardBuilder::filename() const {
	return "/mnt/"+(to_string(m_shard_id % 8))+"/output/precache_" + to_string(m_shard_id) + ".fti";
}

string LinkShardBuilder::target_filename() const {
	return "/mnt/"+to_string(m_shard_id % 8)+"/full_text/fti_" + m_db_name + "_" + to_string(m_shard_id) + ".idx";
}

void LinkShardBuilder::truncate() {

	m_cache.clear();

	m_writer.open(filename(), ios::trunc);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard. Error: " + string(strerror(errno)));
	}
	m_writer.close();

	ofstream target_writer(target_filename(), ios::trunc);
	target_writer.close();
}

/*
	Deletes all data from caches.
*/
void LinkShardBuilder::truncate_cache_files() {

	m_cache.clear();

	m_writer.open(filename(), ios::trunc);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard. Error: " + string(strerror(errno)));
	}
	m_writer.close();

}

size_t LinkShardBuilder::disk_size() const {

	m_reader.open(filename(), ios::binary);
	if (!m_reader.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " + string(strerror(errno)));
	}

	m_reader.seekg(0, ios::end);
	size_t file_size = m_reader.tellg();
	m_reader.close();
	return file_size;
}

size_t LinkShardBuilder::cache_size() const {
	return m_input_position + (m_input.size() - 1) * m_max_cache_size;
}

