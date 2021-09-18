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

#include "HashTable.h"

using namespace std;

class HashTableShardBuilder {

public:

	HashTableShardBuilder(const string &db_name, size_t shard_id);
	~HashTableShardBuilder();

	bool full() const;
	void write();
	void truncate();
	void sort();
	void optimize();

	void add(uint64_t key, const string &value);

	string filename_data() const;
	string filename_pos() const;
	string filename_data_tmp() const;
	string filename_pos_tmp() const;

private:

	map<uint64_t, string> m_cache;
	const string m_db_name;
	size_t m_shard_id;
	const size_t m_cache_limit;
	map<uint64_t, size_t> m_sort_pos;

	void read_keys();

};
