

#include "test3.h"
#include "common.h"
#include "TsvFileS3.h"
#include "Dictionary.h"
#include <unistd.h>

#include <vector>
#include <iostream>

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
