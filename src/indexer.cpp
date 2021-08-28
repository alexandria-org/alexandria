
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

	/*FullTextShardBuilder shard("main_index", 2166);
	shard.merge();
	return 0;*/

	cout << "FT_INDEXER_CACHE_BYTES_PER_SHARD: " << Config::ft_cached_bytes_per_shard << endl;

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

	if (arg == "truncate_link") {

		HashTableHelper::truncate("link_index");
		HashTableHelper::truncate("domain_link_index");

		//FullText::truncate_index("link_index", 8);
		//FullText::truncate_index("domain_link_index", 8);

		return 0;
	}

	if (arg == "truncate") {

		FullText::truncate_url_to_domain("main_index");
		FullText::truncate_index("main_index", 8);
		FullText::truncate_index("link_index", 8);
		FullText::truncate_index("domain_link_index", 8);

		HashTableHelper::truncate("main_index");
		HashTableHelper::truncate("link_index");
		HashTableHelper::truncate("domain_link_index");

		return 0;
	}

	return 0;
}
