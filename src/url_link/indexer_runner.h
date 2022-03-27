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
#include "full_text/full_text_index.h"
#include "full_text/full_text_indexer.h"
#include "full_text/url_to_domain.h"
#include "url_link/full_text_record.h"
#include "domain_link/full_text_record.h"
#include "full_text/full_text_record.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

namespace url_link {

	class indexer_runner {

	public:

		indexer_runner(const std::string &db_name, const std::string &domain_db_name, const std::string &hash_table_name, const std::string &domain_hash_table_name,
			const std::string &cc_batch, const SubSystem *sub_system, full_text::url_to_domain *url_to_domain);
		~indexer_runner();

		void run(const std::vector<std::string> &local_files);
		void merge();
		void sort();

	private:

		const SubSystem *m_sub_system;
		const std::string m_cc_batch;
		const std::string m_db_name;
		const std::string m_domain_db_name;
		const std::string m_hash_table_name;
		const std::string m_domain_hash_table_name;
		std::vector<std::mutex> m_hash_table_mutexes;
		std::vector<std::mutex> m_domain_hash_table_mutexes;
		std::vector<std::mutex> m_link_mutexes;
		std::vector<std::mutex> m_domain_link_mutexes;

		full_text::url_to_domain *m_url_to_domain;

		std::string run_index_thread_with_local_files(const std::vector<std::string> &local_files, int id);
		std::string run_link_index_thread(const std::vector<std::string> &warc_paths, int id);
		std::string run_merge_thread(size_t shard_id);

	};

}
