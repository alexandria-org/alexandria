/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "index_reader.h"
#include "roaring/roaring.hh"

namespace indexer {

	template<typename data_record>
	class index {

	public:

		index(const std::string &file_name);
		index(const std::string &db_name, size_t id);
		index(const std::string &db_name, size_t id, size_t hash_table_size);
		index(index_reader *ram_reader, size_t hash_table_size);
		~index();

		std::vector<data_record> find(uint64_t key) const;
		roaring::Roaring find_bitmap(uint64_t key) const;

		/*
		 * Returns inverse document frequency (idf) for the last search.
		 * */
		float get_idf(size_t documents_with_term) const;
		size_t get_document_count() const { return m_unique_count; }

		void set_hash_table_size(size_t size) { m_hash_table_size = size; }

		void print_stats();

	private:

		mutable index_reader *m_reader;
		std::unique_ptr<index_reader_file> m_default_reader;
		
		std::string m_file_name;
		std::string m_db_name;
		size_t m_id;
		const size_t m_hash_table_size;
		size_t m_unique_count = 0;

		size_t read_key_pos(uint64_t key) const;
		void read_meta();
		std::string mountpoint() const;
		std::string filename() const;
		std::string meta_filename() const;
		
	};

	template<typename data_record>
	index<data_record>::index(const std::string &file_name)
	: m_file_name(file_name), m_hash_table_size(config::shard_hash_table_size) {
		m_default_reader = make_unique<index_reader_file>(filename());
		m_reader = (index_reader *)m_default_reader.get();
	}

	template<typename data_record>
	index<data_record>::index(const std::string &db_name, size_t id)
	: m_db_name(db_name), m_id(id), m_hash_table_size(config::shard_hash_table_size) {
		m_default_reader = make_unique<index_reader_file>(filename());
		m_reader = (index_reader *)m_default_reader.get();
	}

	template<typename data_record>
	index<data_record>::index(const std::string &db_name, size_t id, size_t hash_table_size)
	: m_db_name(db_name), m_id(id), m_hash_table_size(hash_table_size) {
		m_default_reader = make_unique<index_reader_file>(filename());
		m_reader = (index_reader *)m_default_reader.get();
	}

	template<typename data_record>
	index<data_record>::index(index_reader *ram_reader, size_t hash_table_size)
	: m_hash_table_size(hash_table_size) {
		m_reader = ram_reader;
	}

	template<typename data_record>
	index<data_record>::~index() {
	}

	template<typename data_record>
	std::vector<data_record> index<data_record>::find(uint64_t key) const {

		roaring::Roaring rr = find_bitmap(key);

		std::function<data_record(uint32_t)> id_to_rec = [this](uint32_t id) {
			data_record rec;
			m_reader->seek((m_hash_table_size + 1) * sizeof(uint64_t) + id * sizeof(data_record));
			m_reader->read((char *)&rec, sizeof(data_record));
			return rec;
		};

		std::vector<data_record> ret;
		for (uint32_t internal_id : rr) {
			ret.emplace_back(id_to_rec(internal_id));
		}

		return ret;
	}

	template<typename data_record>
	roaring::Roaring index<data_record>::find_bitmap(uint64_t key) const {
		size_t key_pos = read_key_pos(key);

		if (key_pos == SIZE_MAX) {
			return roaring::Roaring();
		}

		// Read page.
		m_reader->seek(key_pos);
		size_t num_keys;
		m_reader->read((char *)&num_keys, sizeof(size_t));

		std::unique_ptr<uint64_t[]> keys_allocator = std::make_unique<uint64_t[]>(num_keys);
		uint64_t *keys = keys_allocator.get();
		m_reader->read((char *)keys, num_keys * sizeof(uint64_t));

		size_t key_data_pos = SIZE_MAX;
		for (size_t i = 0; i < num_keys; i++) {
			if (keys[i] == key) {
				key_data_pos = i;
			}
		}

		if (key_data_pos == SIZE_MAX) {
			return roaring::Roaring();
		}

		char buffer[64];

		// Read position and length.
		m_reader->seek(key_pos + 8 + num_keys * 8 + key_data_pos * 8);
		m_reader->read(buffer, 8);
		size_t pos = *((size_t *)(&buffer[0]));

		m_reader->seek(key_pos + 8 + (num_keys * 8)*2 + key_data_pos * 8);
		m_reader->read(buffer, 8);
		size_t len = *((size_t *)(&buffer[0]));

		m_reader->seek(key_pos + 8 + (num_keys * 8)*3 + pos);

		std::unique_ptr<char[]> data_allocator = std::make_unique<char[]>(len);
		char *data = data_allocator.get();

		m_reader->read(data, len);

		return roaring::Roaring::readSafe(data, len);
	}

	template<typename data_record>
	float index<data_record>::get_idf(size_t documents_with_term) const {
		if (documents_with_term) {
			const size_t documents_in_corpus = m_unique_count;
			float idf = log((float)documents_in_corpus / documents_with_term);
			return idf;
		}

		return 0.0f;
	}

	/*
	 * Reads the exact position of the key, returns SIZE_MAX if the key was not found.
	 * */
	template<typename data_record>
	size_t index<data_record>::read_key_pos(uint64_t key) const {

		if (m_hash_table_size == 0) return 0;

		const size_t hash_pos = key % m_hash_table_size;

		m_reader->seek(hash_pos * sizeof(size_t));

		size_t pos;
		m_reader->read((char *)&pos, sizeof(size_t));

		return pos;
	}

	/*
	 * Reads the count of unique recprds from the count file and puts it in the m_unique_count member.
	 * */
	template<typename data_record>
	void index<data_record>::read_meta() {
		struct meta {
			size_t unique_count;
		};

		meta m;

		std::ifstream meta_reader(meta_filename(), std::ios::binary);

		if (meta_reader.is_open()) {
			meta_reader.read((char *)(&m), sizeof(meta));
		}

		m_unique_count = m.unique_count;
	}

	template<typename data_record>
	std::string index<data_record>::mountpoint() const {
		return std::to_string(m_id % 8);
	}

	template<typename data_record>
	std::string index<data_record>::filename() const {
		if (m_file_name != "") return m_file_name + ".data";
		return "/mnt/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) + ".data";
	}

	template<typename data_record>
	std::string index<data_record>::meta_filename() const {
		if (m_file_name != "") return m_file_name + ".meta";
		return "/mnt/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) + ".meta";
	}

	template<typename data_record>
	void index<data_record>::print_stats() {

		size_t total_num_keys = 0;
		size_t total_num_records = 0;
		size_t total_roaring_size = 0;
		size_t total_record_size = 0;
		size_t total_file_size = m_reader->size();
		size_t total_cardinality = 0;
		size_t total_page_header_size = 0;

		m_reader->seek(m_hash_table_size * 8);
		m_reader->read((char *)&total_num_records, sizeof(size_t));

		total_record_size = total_num_records * sizeof(data_record);

		for (size_t page = 0; page < m_hash_table_size; page++) {
			size_t key_pos = read_key_pos(page);

			if (key_pos == SIZE_MAX) {
				continue;
			}

			// Read page.
			m_reader->seek(key_pos);
			size_t num_keys;
			m_reader->read((char *)&num_keys, sizeof(size_t));

			total_num_keys += num_keys;

			std::unique_ptr<uint64_t[]> keys_allocator = std::make_unique<uint64_t[]>(num_keys);
			uint64_t *keys = keys_allocator.get();
			m_reader->read((char *)keys, num_keys * sizeof(uint64_t));
			total_page_header_size += num_keys * sizeof(uint64_t) * 3;

			for (size_t i = 0; i < num_keys; i++) {
				size_t key_data_pos = i;
				
				char buffer[64];

				// Read position and length.
				m_reader->seek(key_pos + 8 + num_keys * 8 + key_data_pos * 8);
				m_reader->read(buffer, 8);
				size_t pos = *((size_t *)(&buffer[0]));

				m_reader->seek(key_pos + 8 + (num_keys * 8)*2 + key_data_pos * 8);
				m_reader->read(buffer, 8);
				size_t len = *((size_t *)(&buffer[0]));

				m_reader->seek(key_pos + 8 + (num_keys * 8)*3 + pos);

				std::unique_ptr<char[]> data_allocator = std::make_unique<char[]>(len);
				char *data = data_allocator.get();

				m_reader->read(data, len);

				roaring::Roaring rr = roaring::Roaring::readSafe(data, len);

				total_cardinality += rr.cardinality();
				total_roaring_size += len;
			}
		}

		cout << "total_num_keys: " << total_num_keys << endl;
		cout << "total_num_records: " << total_num_records << endl;
		cout << "record size: " << total_record_size << " (" << 100*((float)total_record_size / total_file_size) << "%)" << endl;
		cout << "page header size: " << total_page_header_size << " (" << 100*((float)total_page_header_size / total_file_size) << "%)" << endl;
		cout << "roaring size: " << total_roaring_size << " (" << 100*((float)total_roaring_size / total_file_size) << "%)" << endl;
		cout << "mean length for key: " << total_roaring_size / total_num_keys << endl;
		cout << "mean cardinality for key: " << total_cardinality / total_num_keys << endl;
	}

}
