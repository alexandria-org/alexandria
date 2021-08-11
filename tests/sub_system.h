
#include "system/SubSystem.h"
#include "common/Dictionary.h"
#include "file/TsvFileS3.h"

BOOST_AUTO_TEST_SUITE(sub_system)

BOOST_AUTO_TEST_CASE(sub_system) {

	SubSystem *ss = new SubSystem();

	delete ss;
}

BOOST_AUTO_TEST_SUITE_END();
