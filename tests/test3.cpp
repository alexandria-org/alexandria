

#include "test3.h"
#include "common/common.h"
#include "file/TsvFileS3.h"
#include "common/Dictionary.h"
#include <unistd.h>

#include <vector>
#include <iostream>

#include "index/CCUrlIndex.h"
#include "index/CCIndexRunner.h"
#include "index/CCIndexMerger.h"

using namespace std;

/*
 * Test TsvFile and TsvFileS3
 */
int test3_1(void) {
	int ok = 1;

	SubSystem *ss = new SubSystem();

	TsvFileS3 s3_file(ss->s3_client(), "dictionary.tsv");

	vector<string> container1;

	s3_file.read_column_into(0, container1);

	ok = ok && container1[0] == "archives";

	TsvFileS3 test_file2(ss->s3_client(), "domain_info.tsv");

	Dictionary dict2(test_file2);

	ok = ok && dict2.find("com.rockonrr")->second.get_int(1) == 14275445;

	delete ss;

	return ok;
}

int test3_2(void) {
	int ok = 1;

	CCIndexRunner<CCUrlIndex> indexer("CC-MAIN-2021-10");
	//indexer.run_all(1);
	//CCFullTextIndexer::run_all(1);
	//CCUrlIndexer::run_all(1);

	TsvFile my_file("../tests/data/tsvtest.tsv");

	ok = ok && my_file.find_first_position("aaa") == 0;
	ok = ok && my_file.find_first_position("aab") == 126;
	ok = ok && my_file.find_first_position("european") == string::npos;

	ok = ok && my_file.find_last_position("aaa") == 112;
	ok = ok && my_file.find_last_position("aab") == 126;
	ok = ok && my_file.find_last_position("european") == string::npos;

	TsvFile my_file2("../tests/data/tsvtest2.tsv");

	ok = ok && my_file2.find_first_position("aaa") == 0;
	ok = ok && my_file2.find_first_position("aab") > 0;
	ok = ok && my_file2.find_first_position("european") == string::npos;

	ok = ok && my_file2.find_last_position("aaa") > 0 && my_file2.find_last_position("aaa") < my_file2.size();
	ok = ok && my_file2.find_last_position("aab") > 0 && my_file2.find_last_position("aab") < my_file2.size();
	ok = ok && my_file2.find_last_position("aac") > 0 && my_file2.find_last_position("aac") == my_file2.size() - 115;
	ok = ok && my_file2.find_last_position("european") == string::npos;

	ok = ok && my_file2.find_next_position("aaa") == my_file2.find_first_position("aab");
	ok = ok && my_file2.find_next_position("aab") == my_file2.find_first_position("aac");
	ok = ok && my_file2.find_next_position("aabb") == my_file2.find_first_position("aac");
	ok = ok && my_file2.find_next_position("aac") == my_file2.size();

	Profiler profile1("Test profiler");

	return ok;
}

int test3_3(void) {
	int ok = 1;

	CCIndexMerger merger("CC-MAIN-2021-17", "main");
	
	string file1 =
		"test1\tmerge1\t1\n"
		"test2\tmerge2\t2";
	string file2 =
		"test2\tmerge2\t2\n"
		"test3\tmerge3\t3";
	string file23 =
		"test3\tmerge3\t3\n"
		"test2\tmerge2\t2\n"
		"test1\tmerge1\t1";
	ok = ok && merger.merge_file(file1, file2, {1}, 2, 10) == file23;

	file1 =
		"test1\tmerge\tcol1\t999999990\tsome long metadata\n"
		"test1\tmerge\tcol2\t999999991\tsome long metadata\n"
		"test1\tmerge\tcol3\t999999992\tsome long metadata\n"
		"test1\tmerge\tcol4\t999999993\tsome long metadata\n"
		"test1\tmerge\tcol5\t999999994\tsome long metadata";
	file2 =
		"test1\tmerge\tcol1\t999999999\tsome long metadata\n"
		"test1\tmerge\tcol2\t999999998\tsome long metadata\n"
		"test1\tmerge\tcol3\t999999997\tsome long metadata\n"
		"test1\tmerge\tcol6\t999999996\tsome long metadata\n"
		"test1\tmerge\tcol7\t999999995\tsome long metadata";
	file23 =
		"test1\tmerge\tcol6\t999999996\tsome long metadata\n"
		"test1\tmerge\tcol7\t999999995\tsome long metadata\n"
		"test1\tmerge\tcol5\t999999994\tsome long metadata\n"
		"test1\tmerge\tcol4\t999999993\tsome long metadata\n"
		"test1\tmerge\tcol3\t999999992\tsome long metadata\n"
		"test1\tmerge\tcol2\t999999991\tsome long metadata\n"
		"test1\tmerge\tcol1\t999999990\tsome long metadata";
	ok = ok && merger.merge_file(file1, file2, {1,2}, 3, 10) == file23;

	file1 =
		"test1\tmerge\tcol1\t999999990\tsome long metadata\n"
		"test1\tmerge\tcol2\t999999991\tsome long metadata\n"
		"test1\tmerge\tcol3\t999999992\tsome long metadata\n"
		"test1\tmerge\tcol4\t999999993\tsome long metadata\n"
		"test1\tmerge\tcol5\t999999994\tsome long metadata";
	file2 =
		"test1\tmerge_asd\tcol1\t999999999\tsome long metadata\n"
		"test1\tmerge\tcol2\t999999998\tsome long metadata\n"
		"test1\tmerge\tcol3\t999999997\tsome long metadata\n"
		"test1\tmerge\tcol6\t999999996\tsome long metadata\n"
		"test1\tmerge\tcol7\t999999995\tsome long metadata";
	file23 =
		"test1\tmerge_asd\tcol1\t999999999\tsome long metadata\n"
		"test1\tmerge\tcol6\t999999996\tsome long metadata\n"
		"test1\tmerge\tcol7\t999999995\tsome long metadata\n"
		"test1\tmerge\tcol5\t999999994\tsome long metadata\n"
		"test1\tmerge\tcol4\t999999993\tsome long metadata\n"
		"test1\tmerge\tcol3\t999999992\tsome long metadata\n"
		"test1\tmerge\tcol2\t999999991\tsome long metadata\n"
		"test1\tmerge\tcol1\t999999990\tsome long metadata";
	ok = ok && merger.merge_file(file1, file2, {1,2}, 3, 10) == file23;

	return ok;
}

int test3_4(void) {
	int ok = 1;

	SubSystem *sub_system = new SubSystem();

	sub_system->upload_from_string("alexandria-index", "example.txt", "Hej hopp");
	ok = ok && sub_system->download_to_string("alexandria-index", "example.txt") == "Hej hopp";

	return ok;
}
