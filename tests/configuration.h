
#include "config.h"

BOOST_AUTO_TEST_SUITE(config)

BOOST_AUTO_TEST_CASE(read_config) {
	BOOST_CHECK_EQUAL(Config::nodes_in_cluster, 3);
}

BOOST_AUTO_TEST_SUITE_END();
