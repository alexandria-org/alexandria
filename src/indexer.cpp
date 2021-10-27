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

#include <iterator>
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#include <iostream>
#include <memory>
#include <unistd.h>

#include "config.h"
#include "system/Profiler.h"

#include "hash_table/HashTableHelper.h"

#include "full_text/FullText.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextIndexer.h"
#include "full_text/FullTextIndexerRunner.h"

#include "link_index/LinkIndexer.h"
#include "link_index/LinkIndexerRunner.h"

using namespace std;
using namespace Aws::Utils::Json;

namespace io = boost::iostreams;

int main(int argc, const char **argv) {

	if (argc == 1) {

		FullText::index_all_batches("main_index", "main_index");
		Profiler::print_memory_status();

		return 0;
	}

	const string arg(argv[1]);

	if (arg == "link") {

		FullText::index_all_link_batches("link_index", "domain_link_index", "link_index", "domain_link_index");
		Profiler::print_memory_status();

		return 0;
	}

	if (arg == "optimize") {

		vector<HashTableShardBuilder *> shards = HashTableHelper::create_shard_builders("main_index");
		HashTableHelper::optimize(shards);

		return 0;
	}

	if (arg == "truncate_link") {

		HashTableHelper::truncate("link_index");
		HashTableHelper::truncate("domain_link_index");

		FullText::truncate_index("link_index");
		FullText::truncate_index("domain_link_index");

		return 0;
	}

	if (arg == "truncate") {

		FullText::truncate_url_to_domain("main_index");
		FullText::truncate_index("main_index");
		FullText::truncate_index("link_index");
		FullText::truncate_index("domain_link_index");

		HashTableHelper::truncate("main_index");
		HashTableHelper::truncate("link_index");
		HashTableHelper::truncate("domain_link_index");

		return 0;
	}

	return 0;
}
