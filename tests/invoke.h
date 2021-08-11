
#include "aws/Lambda.h"

BOOST_AUTO_TEST_SUITE(invoke)

BOOST_AUTO_TEST_CASE(invoke_lambda) {
	SubSystem *sub_system = new SubSystem();
	const string key = "crawl-data/CC-MAIN-2021-04/segments/1610703495901.0/warc/CC-MAIN-20210115134101-20210115164101-00000.warc.gz";
	size_t return_code = Lambda::invoke(sub_system, "cc-parser", "{\"s3bucket\":\"commoncrawl\", \"s3key\": \""+key+"\"}");

	BOOST_CHECK_EQUAL(return_code, 202);

	delete sub_system;
}

BOOST_AUTO_TEST_SUITE_END();
