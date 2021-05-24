
#include <iterator>
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#include <iostream>
#include <memory>
#include <unistd.h>

#include "CCFullTextIndexer.h"
#include "CCUrlIndex.h"
#include "CCIndexRunner.h"
#include "CCIndexMerger.h"
#include "CCLinkIndex.h"

using namespace std;
using namespace Aws::Utils::Json;

namespace io = boost::iostreams;

int main(int argc, const char **argv) {

	//CCFullTextIndexer::run_all();
	//CCUrlIndexer::run_all();
	//CCIndexRunner<CCUrlIndex> indexer("CC-MAIN-2021-04");
	
	//CCIndexRunner<CCLinkIndex> indexer("CC-MAIN-2021-04");
	//indexer.run_all();

	CCIndexMerger merger("CC-MAIN-2021-17", "main");
	merger.run_all();

	return 0;
}
