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

#include "bloom_filter.h"
#include "algorithm/hash.h"
#include <cmath>
#include <cstring>
#include <fstream>

namespace algorithm {

	bloom_filter::bloom_filter()
	{
		m_bitmap = std::make_unique<uint64_t[]>(m_dim);
		for (size_t i = 0; i < m_dim; i++) {
			m_bitmap[i] = 0x0ull;
		}
	}

	// Dim should be a prime number..
	bloom_filter::bloom_filter(size_t dim)
	: m_dim(dim), m_bitlen(dim * 64)
	{
		m_bitmap = std::make_unique<uint64_t[]>(m_dim);
		for (size_t i = 0; i < m_dim; i++) {
			m_bitmap[i] = 0x0ull;
		}
	}

	void bloom_filter::insert(const std::string &item) {
		for (size_t i = 0; i < m_seeds.size(); i++) {
			const uint64_t hash = algorithm::hash_with_seed(item, m_seeds[i]);
			set_bit(hash);
		}
	}

	void bloom_filter::insert(uint64_t item) {
		insert(std::to_string(item));
	}

	void bloom_filter::insert_many(std::vector<uint64_t> &items) {

		std::vector<size_t> hashes;
		for (const auto &item : items) {
			const auto str_item = std::to_string(item);
			for (size_t i = 0; i < m_seeds.size(); i++) {
				const uint64_t hash = algorithm::hash_with_seed(str_item, m_seeds[i]);
				hashes.push_back(hash);
			}
		}

		std::lock_guard guard(m_mutex);
		for (const auto &hash : hashes) {
			set_bit(hash);
		}
	}

	const char * bloom_filter::data() const {
		return (char *)m_bitmap.get();
	}

	bool bloom_filter::exists(const std::string &item) const {
		for (size_t i = 0; i < m_seeds.size(); i++) {
			const uint64_t hash = algorithm::hash_with_seed(item, m_seeds[i]);
			if (!get_bit(hash)) return false;
		}
		return true;
	}

	bool bloom_filter::exists(uint64_t data) const {
		return exists(std::to_string(data));
	}

	void bloom_filter::read(char *data, size_t len) {
		memcpy((char *)m_bitmap.get(), data, len);
	}

	void bloom_filter::merge(const bloom_filter &other) {
		for (size_t i = 0; i < m_dim; i++) {
			m_bitmap[i] |= other.m_bitmap[i];
		}
	}

	double bloom_filter::saturation() {
		return 1.0;
	}

	void bloom_filter::read_file(const std::string &file_name) {
		std::ifstream infile(file_name, std::ios::binary);
		infile.read((char *)m_bitmap.get(), size());
	}

	void bloom_filter::write_file(const std::string &file_name) const {
		std::ofstream outfile(file_name, std::ios::binary | std::ios::trunc);
		outfile.write((char *)m_bitmap.get(), size());
	}

	void bloom_filter::set_bit(size_t bit) {
		const size_t x = bit % m_bitlen;
		const size_t pos = static_cast<size_t>(x / 64);
		const size_t bit_in_pos = x % 64;
		m_bitmap[pos] = m_bitmap[pos] | (0x1ull << bit_in_pos);
	}

	bool bloom_filter::get_bit(size_t bit) const {
		const size_t x = bit % m_bitlen;
		const size_t pos = static_cast<size_t>(x / 64);
		const size_t bit_in_pos = x % 64;
		return (m_bitmap[pos] & (0x1ull << bit_in_pos)) >> bit_in_pos;
	}

}
