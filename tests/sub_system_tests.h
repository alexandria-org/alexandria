
#include "system/SubSystem.h"
#include "common/Dictionary.h"
#include "file/TsvFileS3.h"

BOOST_AUTO_TEST_SUITE(sub_system_tests)

BOOST_AUTO_TEST_CASE(sub_system) {

	SubSystem *ss = new SubSystem();

	TsvFileS3 s3_file(ss->s3_client(), "dictionary.tsv");

	vector<string> container1;

	s3_file.read_column_into(0, container1);

	BOOST_CHECK_EQUAL(container1[0], "archives");

	TsvFileS3 test_file2(ss->s3_client(), "domain_info.tsv");

	Dictionary dict2(test_file2);

	BOOST_CHECK_EQUAL(dict2.find("com.rockonrr")->second.get_int(1), 14275445);

	delete ss;
}

BOOST_AUTO_TEST_SUITE_END();
