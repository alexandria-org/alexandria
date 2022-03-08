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

namespace indexer {

	template<typename data_record>
	class index {

	public:

		index(const std::string &db_name, size_t id);
		index(const std::string &db_name, size_t id, size_t hash_table_size);
		~index();

		std::vector<data_record> find(uint64_t key) const;
		std::vector<data_record> find(uint64_t key, size_t &total_found) const;

		/*
		 * Returns inverse document frequency (idf) for the last search.
		 * */
		float get_idf(size_t documents_with_term) const;
		size_t get_document_count() const { return m_unique_count; }

	private:

		std::string m_db_name;
		size_t m_id;
		const size_t m_hash_table_size;
		size_t m_unique_count = 0;

		size_t read_key_pos(uint64_t key) const;
		void read_meta();
		std::string mountpoint() const;
		std::string filename() const;
		std::string key_filename() const;
		std::string meta_filename() const;
		
	};

	template<typename data_record>
	index<data_record>::index(const std::string &db_name, size_t id)
	: m_db_name(db_name), m_id(id), m_hash_table_size(Config::shard_hash_table_size) {
		read_meta();
	}

	template<typename data_record>
	index<data_record>::index(const std::string &db_name, size_t id, size_t hash_table_size)
	: m_db_name(db_name), m_id(id), m_hash_table_size(hash_table_size) {
		read_meta();
	}

	template<typename data_record>
	index<data_record>::~index() {
	}

	template<typename data_record>
	std::vector<data_record> index<data_record>::find(uint64_t key) const {
		size_t total;
		return find(key, total);
	}

	template<typename data_record>
	std::vector<data_record> index<data_record>::find(uint64_t key, size_t &total_found) const {

		size_t key_pos = read_key_pos(key);

		if (key_pos == SIZE_MAX) {
			return {};
		}

		// Read page.
		std::ifstream reader(filename(), std::ios::binary);
		reader.seekg(key_pos);
		size_t num_keys;
		reader.read((char *)&num_keys, sizeof(size_t));

		std::unique_ptr<uint64_t[]> keys_allocator = std::make_unique<uint64_t[]>(num_keys);
		uint64_t *keys = keys_allocator.get();
		reader.read((char *)keys, num_keys * sizeof(uint64_t));

		size_t key_data_pos = SIZE_MAX;
		for (size_t i = 0; i < num_keys; i++) {
			if (keys[i] == key) {
				key_data_pos = i;
			}
		}

		if (key_data_pos == SIZE_MAX) {
			return {};
		}

		char buffer[64];

		// Read position and length.
		reader.seekg(key_pos + 8 + num_keys * 8 + key_data_pos * 8, std::ios::beg);
		reader.read(buffer, 8);
		size_t pos = *((size_t *)(&buffer[0]));

		reader.seekg(key_pos + 8 + (num_keys * 8)*2 + key_data_pos * 8, std::ios::beg);
		reader.read(buffer, 8);
		size_t len = *((size_t *)(&buffer[0]));

		reader.seekg(key_pos + 8 + (num_keys * 8)*3 + key_data_pos * 8, std::ios::beg);
		reader.read(buffer, 8);
		total_found = *((size_t *)(&buffer[0]));

		reader.seekg(key_pos + 8 + (num_keys * 8)*4 + pos, std::ios::beg);

		size_t num_records = len / sizeof(data_record);

		std::vector<data_record> ret(num_records);
		reader.read((char *)ret.data(), sizeof(data_record) * num_records);

		return ret;
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

		std::ifstream key_reader(key_filename(), std::ios::binary);

		key_reader.seekg(hash_pos * sizeof(size_t));

		size_t pos;
		key_reader.read((char *)&pos, sizeof(size_t));

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
		return "/mnt/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) + ".data";
	}

	template<typename data_record>
	std::string index<data_record>::key_filename() const {
		return "/mnt/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) + ".keys";
	}

	template<typename data_record>
	std::string index<data_record>::meta_filename() const {
		return "/mnt/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) + ".meta";
	}

}
