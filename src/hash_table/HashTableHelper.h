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
#include "hash_table/HashTable.h"
#include "hash_table/HashTableShardBuilder.h"

namespace HashTableHelper {

	void truncate(const std::string &hash_table_name);
	std::vector<HashTableShardBuilder *> create_shard_builders(const std::string &hash_table_name);
	void delete_shard_builders(std::vector<HashTableShardBuilder *> &shards);
	void add_data(std::vector<HashTableShardBuilder *> &shards, uint64_t key, const std::string &value);
	void write(std::vector<HashTableShardBuilder *> &shards);
	void sort(std::vector<HashTableShardBuilder *> &shards);
	void optimize(std::vector<HashTableShardBuilder *> &shards);

}
