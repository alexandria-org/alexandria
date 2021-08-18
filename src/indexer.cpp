
#include <iterator>
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#include <iostream>
#include <memory>
#include <unistd.h>

#include "index/CCUrlIndex.h"
#include "index/CCIndexRunner.h"
#include "index/CCIndexMerger.h"
#include "index/CCLinkIndex.h"
#include "system/Profiler.h"

#include "full_text/FullText.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextIndexer.h"
#include "full_text/FullTextIndexerRunner.h"
#include "full_text/FullTextResult.h"

#include "link_index/LinkIndex.h"
#include "link_index/LinkIndexer.h"
#include "link_index/LinkIndexerRunner.h"

using namespace std;
using namespace Aws::Utils::Json;

namespace io = boost::iostreams;

int main(int argc, const char **argv) {

	/*FullTextShardBuilder shard("main_index", 2166);
	shard.merge();
	return 0;*/

	cout << "FT_INDEXER_CACHE_BYTES_PER_SHARD: " << FT_INDEXER_CACHE_BYTES_PER_SHARD << endl;

	if (argc == 1) {

		HashTable hash_table("main_index");
		hash_table.truncate();

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			FullTextIndexerRunner indexer("main_index_" + to_string(partition_num), "main_index", "CC-MAIN-2021-31", sub_system);
			indexer.run(partition_num, 8);
		}
		Profiler::print_memory_status();
		return 0;
	}

	const string arg(argv[1]);

	if (arg == "link") {

		const string cc_batch(argv[2]);

		LogInfo("Running link indexer on " + cc_batch);

		LogInfo("Reading UrlToDomain map");
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			LinkIndexerRunner indexer("link_index_" + to_string(partition_num), "domain_link_index_" + to_string(partition_num),
				"link_index", "domain_link_index", cc_batch, sub_system, url_to_domain);
			indexer.run(partition_num, 8);
		}
		Profiler::print_memory_status();

		return 0;
	}

	if (arg == "truncate_link") {
		FullText::truncate_index("link_index", 8);
		HashTable hash_table("link_index");

		hash_table.truncate();

		return 0;
	}

	return 0;
}
