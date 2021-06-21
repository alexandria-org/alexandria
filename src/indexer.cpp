
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
#include "full_text/FullTextIndexer.h"
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
	/*hash_table.download(sub_system);
	fti.download(sub_system);

	delete sub_system;
	return 0;*/

	/*SubSystem *sub_system = new SubSystem();
	hash<string> hasher;
	const uint64_t hashed_url = hasher("http://aciedd.org/fixing-solar-panels/");
	FullTextIndexer indexer(1, sub_system);
	vector<string> words = indexer.get_full_text_words("If you live in an region that receives");
	for (auto word : words) {
		vector<FullTextResult> result2 = fti.search_word(word);
		bool did_find = false;
		for (auto res : result2) {
			if (res.m_value == hashed_url) {
				did_find = true;
				break;
			}
		}
		if (did_find) {
			cout << "found url for: " << word << endl;
		} else {
			cout << "NOT found url for: " << word << endl;
		}
	}
	return 0;*/

	string query = "";
	while (query != "quit") {
		cout << "query> ";
		getline(cin, query);

		if (query == "quit") break;
		if (query == "") continue;

		Profiler profiler2("Total");
		Profiler profiler3("FTI");
		size_t total;
		vector<FullTextResult> result2 = fti.search_phrase(query, 1000, total);
		profiler3.stop();

		Profiler profiler4("HT");
		size_t idx = 0;
		for (FullTextResult &res : result2) {
			string url = hash_table.find(res.m_value);
			cout << "found ID: " << res.m_value << " score (" << res.m_score << ")" << endl;
			cout << "found url: " << url << endl;
			idx++;
		}
		profiler4.stop();
		profiler2.stop();
		cout << "Found a total of: " << total << " urls and fetched " << idx << " of them" << endl;
		cout << "Top 10 URLs:" << endl;
		idx = 0;
		for (FullTextResult &res : result2) {
			string url = hash_table.find(res.m_value);
			cout << "[" << res.m_score << "] " << url << endl;
			if (idx >= 10) break;
			idx++;
		}
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
