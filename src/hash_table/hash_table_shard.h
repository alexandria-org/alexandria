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

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "hash_table.h"

namespace hash_table {

	class hash_table_shard {

		public:

			hash_table_shard(const std::string &db_name, size_t shard_id);
			~hash_table_shard();

			std::string find(uint64_t key) const;

			std::string filename_data() const;
			std::string filename_pos() const;
			size_t shard_id() const;
			size_t size() const;
			size_t file_size() const;
			void print_all_items();

		private:

			const std::string m_db_name;
			size_t m_shard_id;
			bool m_loaded;
			size_t m_size;

			const int m_significant = 12;

			// Maps keys to positions in file.
			std::unordered_map<uint64_t, std::pair<size_t, size_t>> m_pos;

			void load();
			std::string data_at_position(size_t pos) const;

	};

}
