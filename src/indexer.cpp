
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

	ifstream infile("/root/CC-MAIN-20210423161713-20210423191713-00094");

	if (infile.is_open()) {

		string data;
		size_t sum_compressed = 0;
		size_t sum_uncompressed = 0;
		size_t sum_decompress_time = 0;
		size_t sum = 0;
		while (getline(infile, data)) {

			stringstream ss(data);

			filtering_istream compress_stream;
			compress_stream.push(gzip_compressor(gzip_params(gzip::best_compression)));
			compress_stream.push(ss);

			stringstream res;
			res << compress_stream.rdbuf();

			string compressed = res.str();

			if (compressed.size() > 1500) {
				cout << data << endl;
				cout << "uncompressed size: " << data.size() << endl;
				cout << "compressed size: " << compressed.size() << endl;
			}

			Profiler decompress_profile("Uncompress time");
			stringstream ss2(compressed);

			filtering_istream decompress_stream;
			decompress_stream.push(gzip_compressor());
			decompress_stream.push(ss2);

			stringstream res2;
			res2 << decompress_stream.rdbuf();

			string decompressed = res2.str();

			sum_decompress_time += decompress_profile.get_micro();
			decompress_profile.stop();

			sum_compressed += compressed.size();
			sum_uncompressed += data.size();

			sum++;
		}

		cout << "average compressed: " << ((double)sum_compressed/sum) << endl;
		cout << "average uncompressed: " << ((double)sum_uncompressed/sum) << endl;
		cout << "average time to decompress: " << ((double)sum_decompress_time/sum) << " microseconds" << endl;

	}

	return 0;


	/*FullTextIndexerRunner indexer("CC-MAIN-2021-17");
	//indexer.index_text("http://aciedd.org/fixing-solar-panels/	Fixing Solar Panels ‚Äì blog	blog		Menu Home Search for: Posted in General Fixing Solar Panels Author: Holly Montgomery Published Date: December 24, 2020 Leave a Comment on Fixing Solar Panels Complement your renewable power project with Perfection fashionable solar panel assistance structures. If you live in an region that receives a lot of snow in the winter, becoming able to easily sweep the snow off of your solar panels is a major comfort. If your solar panel contractor advises you that horizontal solar panels are the greatest selection for your solar wants, you do not need to have a particular inverter. The Solar PV panels are then clamped to the rails, keeping the panels really close to the roof to decrease wind loading. For 1 point, solar panels require to face either south or west to get direct sunlight. Once you have bought your solar panel you will need to have to determine on a safe fixing method, our extensive variety of permanent and non permane");
	indexer.index_warc_path("/crawl-data/CC-MAIN-2021-17/segments/1618039596883.98/warc/CC-MAIN-20210423161713-20210423191713-00094.gz");
	indexer.merge();

	hash<string> hasher;
	const uint64_t hashed_url = hasher("http://aciedd.org/fixing-solar-panels/");

	FullTextIndex fti1("main_index");
	vector<FullTextResult> result2 = fti1.search_word("an");
	cout << "hashed_url: " << hashed_url << endl;
	for (auto res : result2) {
		if (res.m_value == hashed_url) {
			cout << "Found: " << res.m_value << endl;
		}
	}
	return 0;*/

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
