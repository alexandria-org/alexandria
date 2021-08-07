
#define BOOST_TEST_MODULE file_test
#include <boost/test/unit_test.hpp>
#include "file/Transfer.h"
#include "text/Text.h"

BOOST_AUTO_TEST_CASE(transfer_test) {
	int error;
	{
		string result = Transfer::file_to_string("/example.txt", error);
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(Text::trim(result) == "An example file");
	}

	{
		string result = Transfer::gz_file_to_string("/example.txt.gz", error);
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(Text::trim(result) == "An example file");
	}

	{
		string result = Transfer::file_to_string("example.txt", error);
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(Text::trim(result) == "An example file");
	}

	{
		string result = Transfer::gz_file_to_string("example.txt.gz", error);
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(Text::trim(result) == "An example file");
	}

	{
		stringstream ss;
		Transfer::file_to_stream("/example.txt", ss, error);
		string result = ss.str();
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(Text::trim(result) == "An example file");
	}

	{
		stringstream ss;
		Transfer::gz_file_to_stream("/example.txt.gz", ss, error);
		string result = ss.str();
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(Text::trim(result) == "An example file");
	}
}

