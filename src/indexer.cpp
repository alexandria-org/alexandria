
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

#include "full_text/FullTextIndex.h"
#include "full_text/FullTextIndexer.h"
#include "full_text/FullTextIndexerRunner.h"
#include "full_text/FullTextResult.h"

#include "link_index/LinkIndex.h"
#include "link_index/LinkIndexer.h"
#include "link_index/LinkIndexerRunner.h"
#include "link_index/LinkResult.h"

using namespace std;
using namespace Aws::Utils::Json;

namespace io = boost::iostreams;

int main(int argc, const char **argv) {

	/*FullTextShardBuilder shard("main_index", 2166);
	shard.merge();
	return 0;*/

	cout << "FT_INDEXER_CACHE_BYTES_PER_SHARD: " << FT_INDEXER_CACHE_BYTES_PER_SHARD << endl;

	if (argc == 1) {
		FullTextIndexerRunner indexer("main_index", "CC-MAIN-2021-17");
		indexer.run();
		Profiler::print_memory_status();
		return 0;
	}

	const string arg(argv[1]);

	if (arg == "link") {
		if (argc != 4) {
			cerr << "Missing offset and limit arguments." << endl;	
			return 1;
		}

		size_t offset = stoi(argv[2]);
		size_t limit = stoi(argv[3]);

		cout << "Running with offset: " << offset << " and limit: " << limit << endl;

		//LinkIndexerRunner indexer("link_index", "CC-MAIN-2021-17", "main_index");
		//LinkIndexerRunner indexer("link_index", "CC-MAIN-2021-10", "main_index");
		LinkIndexerRunner indexer("link_index", "CC-MAIN-2021-04", "main_index");
		indexer.run(offset, limit);
		Profiler::print_memory_status();
		return 0;
	}

	return 0;
}
