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
		~index();

		std::vector<data_record> find(uint64_t key) const;

	private:

		std::string m_db_name;
		size_t m_id;

		size_t read_key_pos(std::ifstream &reader, uint64_t key) const;
		std::string mountpoint() const;
		std::string filename() const;
		std::string key_filename() const;
		
	};

	template<typename data_record>
	index<data_record>::index(const std::string &db_name, size_t id)
	: m_db_name(db_name), m_id(id) {
	}

	template<typename data_record>
	index<data_record>::~index() {
	}

	template<typename data_record>
	std::vector<data_record> index<data_record>::find(uint64_t key) const {

		std::ifstream reader(filename(), std::ios::binary);

		size_t key_pos = read_key_pos(reader, key);

		if (key_pos == SIZE_MAX) {
			return {};
		}

		// Read page.
		reader.seekg(key_pos);
		size_t num_keys;
		reader.read((char *)&num_keys, sizeof(size_t));

		uint64_t *keys = new uint64_t[num_keys];
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
		//size_t total_num_results = *((size_t *)(&buffer[0]));

		reader.seekg(key_pos + 8 + (num_keys * 8)*4 + pos, std::ios::beg);

		size_t num_records = len / sizeof(data_record);

		std::vector<data_record> ret(num_records);
		reader.read((char *)ret.data(), sizeof(data_record) * num_records);

		return ret;
	}

	/*
	 * Reads the exact position of the key, returns SIZE_MAX if the key was not found.
	 * */
	template<typename data_record>
	size_t index<data_record>::read_key_pos(std::ifstream &reader, uint64_t key) const {

		const size_t hash_pos = key % Config::shard_hash_table_size;

		std::ifstream key_reader(key_filename(), std::ios::binary);

		key_reader.seekg(hash_pos * sizeof(size_t));

		size_t pos;
		key_reader.read((char *)&pos, sizeof(size_t));

		return pos;
	}

	template<typename data_record>
	std::string index<data_record>::mountpoint() const {
		return std::to_string(m_id % 8);
	}

	template<typename data_record>
	std::string index<data_record>::filename() const {
		return "/mnt/" + mountpoint() + "/full_text/" + m_db_name + "_" + std::to_string(m_id) + ".data";
	}

	template<typename data_record>
	std::string index<data_record>::key_filename() const {
		return "/mnt/" + mountpoint() + "/full_text/" + m_db_name + "_" + std::to_string(m_id) + ".keys";
	}

}
