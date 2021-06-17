
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
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextIndexerRunner.h"
#include "full_text/FullTextResult.h"
#include "system/Profiler.h"

using namespace std;
using namespace Aws::Utils::Json;

namespace io = boost::iostreams;

int main(int argc, const char **argv) {

	/*FullTextIndexerRunner indexer("CC-MAIN-2021-17");
	indexer.run();
	return 0;*/

	//SubSystem *sub_system = new SubSystem();

	HashTable hash_table("main_index");
	FullTextIndex fti("main_index");
	//fti.download(sub_system);
	//hash_table.download(sub_system);

	//delete sub_system;
	//return 0;

	string query = "";
	while (query != "quit") {
		cout << "query> ";
		getline(cin, query);

		if (query == "quit") break;

		Profiler profiler2("Total");
		Profiler profiler3("FTI");
		vector<FullTextResult> result2 = fti.search_phrase(query);
		profiler3.stop();

		Profiler profiler4("HT");
		size_t idx = 0;
		for (FullTextResult &res : result2) {
			cout << "found ID: " << res.m_value << endl;
			cout << "found url: " << hash_table.find(res.m_value) << endl;
			idx++;
			if (idx >= 10) break;
		}
		profiler4.stop();
		profiler2.stop();
		cout << "Found a total of: " << result2.size() << " urls and fetched " << idx << " of them" << endl;
	}

	return 0;

	/*ofstream outfile("/mnt/example.data", ios::binary | ios::trunc);

	const size_t num_elems = 10000000;
	char *buffer = new char[num_elems * 12];
	for (size_t i = 0; i < num_elems; i++) {
		uint64_t value = rand();
		uint32_t score = rand() % 10;
		memcpy(&buffer[i*12], &value, sizeof(value));
		memcpy(&buffer[i*12 + 8], &score, sizeof(score));
	}
	outfile.write(buffer, num_elems * 12);
	outfile.close();

	Profiler read_file("Read file");

	ifstream infile("/mnt/example.data", ios::binary);
	infile.seekg(0, ios::beg);
	infile.read(buffer, num_elems * 12);
	infile.close();
	read_file.stop();*/



	//CCFullTextIndexer::run_all();
	//CCUrlIndexer::run_all();
	//CCIndexRunner<CCUrlIndex> indexer("CC-MAIN-2021-04");
	
	//CCIndexRunner<CCLinkIndex> indexer("CC-MAIN-2021-04");
	//indexer.run_all();

	//FullTextIndexerRunner indexer("CC-MAIN-2021-17");
	//indexer.run();

	//CCIndexMerger merger("CC-MAIN-2021-17", "main");
	//merger.run_all();

	return 0;
}
