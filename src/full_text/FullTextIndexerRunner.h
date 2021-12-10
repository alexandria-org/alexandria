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
#include <mutex>
#include "system/SubSystem.h"
#include "system/ThreadPool.h"
#include "hash_table/HashTable.h"
#include "full_text/FullTextIndex.h"
#include "FullTextRecord.h"

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;

class FullTextIndexerRunner {

public:

	FullTextIndexerRunner(const string &db_name, const string &hash_table_name, const string &cc_batch, const SubSystem *sub_system);
	FullTextIndexerRunner(const string &db_name, const string &hash_table_name, const string &cc_batch);
	FullTextIndexerRunner(const string &db_name, const string &hash_table_name, const SubSystem *sub_system);
	~FullTextIndexerRunner();

	void run(size_t partition, size_t max_partitions);
	void run(const vector<string> local_files, size_t partition);
	void run_link();
	void merge(size_t partition);
	void sort(size_t partition);
	void truncate_cache(size_t partition);

private:

	const string m_cc_batch;
	const string m_db_name;
	const string m_hash_table_name;
	const SubSystem *m_sub_system;

	mutex m_hash_table_mutexes[Config::ht_num_shards];
	mutex m_full_text_mutexes[Config::ft_num_shards];
	mutex m_write_url_to_domain_mutex;

	bool m_run_merge_large;
	bool m_did_allocate_sub_system;

	string run_index_thread_with_local_files(const vector<string> &local_files, int id, size_t partition);
	string run_merge_thread(size_t shard_id, size_t partition);

};
