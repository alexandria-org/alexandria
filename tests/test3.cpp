

#include "test3.h"
#include "common.h"
#include "TsvFileS3.h"
#include "Dictionary.h"
#include <unistd.h>

#include <vector>
#include <iostream>

#include "CCUrlIndexer.h"
#include "CCFullTextIndexer.h"

using namespace std;

/*
 * Test TsvFile and TsvFileS3
 */
int test3_1(void) {
	int ok = 1;

	Aws::SDKOptions options;
	Aws::InitAPI(options);
	Aws::S3::S3Client s3_client = get_s3_client();


	TsvFileS3 s3_file(s3_client, "dictionary.tsv");

	vector<string> container1;

	s3_file.read_column_into(0, container1);

	ok = ok && container1[0] == "archives";

	TsvFileS3 test_file2(s3_client, "domain_info.tsv");

	Dictionary dict2(test_file2);

	ok = ok && dict2.find("com.rockonrr")->second.get_int(1) == 14275445;

	Aws::ShutdownAPI(options);

	return ok;
}

int test3_2(void) {
	int ok = 1;

	//CCFullTextIndexer::run_all(1);
	//CCUrlIndexer::run_all(1);

	TsvFile my_file("tests/data/tsvtest.tsv");

	ok = ok && my_file.find_first_position("aaa") == 0;
	ok = ok && my_file.find_first_position("aab") == 126;
	ok = ok && my_file.find_first_position("european") == string::npos;

	ok = ok && my_file.find_last_position("aaa") == 112;
	ok = ok && my_file.find_last_position("aab") == 126;
	ok = ok && my_file.find_last_position("european") == string::npos;

	TsvFile my_file2("tests/data/tsvtest2.tsv");

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

	Profiler profile_test("Read file to map");
	ifstream infile("/mnt/1/output_1.tsv");
	string line;
	map<string, string> index;
	size_t total_size = 0;
	while (getline(infile, line)) {
		const string word = line.substr(0, line.find("\t"));
		index[word] += line + "\n";
		total_size += line.size() + 1;
	}
	profile_test.stop();


	return ok;
}
