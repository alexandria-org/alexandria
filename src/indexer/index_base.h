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

#include <vector>
#include "config.h"
#include "logger/logger.h"
#include "roaring/roaring.hh"

namespace indexer {

	template<typename data_record>
	class index_base {

		public:

		index_base();
		explicit index_base(size_t hash_table_size);

		void set_hash_table_size(size_t size) { m_hash_table_size = size; }

		protected:

			size_t m_hash_table_size;
			mutable std::recursive_mutex m_lock;

			bool read_page_into(std::istream &reader, std::map<uint64_t, std::vector<data_record>> &into) const;
			bool read_bitmap_page_into(std::istream &reader, std::map<uint64_t, roaring::Roaring> &into) const;
			size_t hash_table_byte_size() const { return m_hash_table_size * sizeof(size_t); }
	};

	template<typename data_record>
	index_base<data_record>::index_base()
	: m_hash_table_size(config::shard_hash_table_size)
	{}

	template<typename data_record>
	index_base<data_record>::index_base(size_t hash_table_size)
	: m_hash_table_size(hash_table_size)
	{}

	template<typename data_record>
	bool index_base<data_record>::read_page_into(std::istream &reader, std::map<uint64_t, std::vector<data_record>> &into) const {

		uint64_t num_keys;
		reader.read((char *)&num_keys, sizeof(uint64_t));
		if (reader.eof()) return false;

		std::unique_ptr<char[]> vector_buffer_allocator;
		try {
			vector_buffer_allocator = std::make_unique<char[]>(num_keys * sizeof(uint64_t));
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << num_keys << " keys" << std::endl;
			return false;
		}

		char *vector_buffer = vector_buffer_allocator.get();

		// Read the keys.
		reader.read(vector_buffer, num_keys * sizeof(uint64_t));
		std::vector<uint64_t> keys;
		for (size_t i = 0; i < num_keys; i++) {
			keys.push_back(*((uint64_t *)(&vector_buffer[i*8])));
		}

		// Read the positions.
		reader.read(vector_buffer, num_keys * 8);
		std::vector<size_t> positions;
		for (size_t i = 0; i < num_keys; i++) {
			positions.push_back(*((size_t *)(&vector_buffer[i*8])));
		}

		// Read the lengths.
		reader.read(vector_buffer, num_keys * 8);
		std::vector<size_t> lens;
		size_t max_len = 0;
		size_t data_size = 0;
		for (size_t i = 0; i < num_keys; i++) {
			size_t len = *((size_t *)(&vector_buffer[i*8]));
			if (len > max_len) max_len = len;
			lens.push_back(len);
			data_size += len;
		}

		if (data_size == 0) return true;

		std::unique_ptr<char[]> buffer_allocator;
		try {
			buffer_allocator = std::make_unique<char[]>(max_len);
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << max_len << " bytes" << std::endl;
			return false;
		}
		char *buffer = buffer_allocator.get();

		// Read the records.
		for (size_t i = 0; i < num_keys; i++) {
			const size_t len = lens[i];
			reader.read(buffer, len);
			const size_t read_len = reader.gcount();
			if (read_len != len) {
				LOG_INFO("Data stopped before end. Ignoring shard");
				return false;
			}

			const data_record *records = (data_record *)buffer;
			const size_t num_records = len / sizeof(data_record);

			for (size_t j = 0; j < num_records; j++) {
				into[keys[i]].push_back(records[j]);
			}
		}

		return true;
	}

	template<typename data_record>
	bool index_base<data_record>::read_bitmap_page_into(std::istream &reader, std::map<uint64_t, roaring::Roaring> &into) const {

		uint64_t num_keys;
		reader.read((char *)&num_keys, sizeof(uint64_t));
		if (reader.eof()) return false;

		std::unique_ptr<char[]> vector_buffer_allocator;
		try {
			vector_buffer_allocator = std::make_unique<char[]>(num_keys * sizeof(uint64_t));
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << num_keys << " keys" << std::endl;
			return false;
		}

		char *vector_buffer = vector_buffer_allocator.get();

		// Read the keys.
		reader.read(vector_buffer, num_keys * sizeof(uint64_t));
		std::vector<uint64_t> keys;
		for (size_t i = 0; i < num_keys; i++) {
			keys.push_back(*((uint64_t *)(&vector_buffer[i*8])));
		}

		// Read the positions.
		reader.read(vector_buffer, num_keys * 8);
		std::vector<size_t> positions;
		for (size_t i = 0; i < num_keys; i++) {
			positions.push_back(*((size_t *)(&vector_buffer[i*8])));
		}

		// Read the lengths.
		reader.read(vector_buffer, num_keys * 8);
		std::vector<size_t> lens;
		size_t max_len = 0;
		size_t data_size = 0;
		for (size_t i = 0; i < num_keys; i++) {
			size_t len = *((size_t *)(&vector_buffer[i*8]));
			if (len > max_len) max_len = len;
			lens.push_back(len);
			data_size += len;
		}

		if (data_size == 0) return true;

		std::unique_ptr<char[]> buffer_allocator;
		try {
			buffer_allocator = std::make_unique<char[]>(max_len);
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << max_len << " bytes" << std::endl;
			throw exception;
		}
		char *buffer = buffer_allocator.get();

		// Read the bitmap data.
		for (size_t i = 0; i < num_keys; i++) {
			const size_t len = lens[i];
			reader.read(buffer, len);
			const size_t read_len = reader.gcount();
			if (read_len != len) {
				LOG_INFO("Data stopped before end. Ignoring shard ");
				throw std::runtime_error("Data stopped before end. File is corrupt.");
			}

			into[keys[i]] = roaring::Roaring::readSafe(buffer, len);
		}

		return true;
	}

}
