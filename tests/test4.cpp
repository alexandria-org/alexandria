

#include "test4.h"
#include "file/Transfer.h"
#include "parser/URL.h"
#include "text/Text.h"

using namespace std;

/*
 * Test File Transfers
 */
int test4_1(void) {
	int ok = 1;

	int error;
	{
		string result = Transfer::file_to_string("/example.txt", error);
		ok = ok && error == Transfer::OK;
		ok = ok && Text::trim(result) == "An example file";
	}

	{
		string result = Transfer::gz_file_to_string("/example.txt.gz", error);
		ok = ok && error == Transfer::OK;
		ok = ok && Text::trim(result) == "An example file";
	}

	{
		string result = Transfer::file_to_string("example.txt", error);
		ok = ok && error == Transfer::OK;
		ok = ok && Text::trim(result) == "An example file";
	}

	{
		string result = Transfer::gz_file_to_string("example.txt.gz", error);
		ok = ok && error == Transfer::OK;
		ok = ok && Text::trim(result) == "An example file";
	}

	{
		stringstream ss;
		Transfer::file_to_stream("/example.txt", ss, error);
		string result = ss.str();
		ok = ok && error == Transfer::OK;
		ok = ok && Text::trim(result) == "An example file";
	}

	{
		stringstream ss;
		Transfer::gz_file_to_stream("/example.txt.gz", ss, error);
		string result = ss.str();
		ok = ok && error == Transfer::OK;
		ok = ok && Text::trim(result) == "An example file";
	}

	return ok;
}

/*
 * Test URL parser
 * */
int test4_2(void) {
	int ok = 1;

	URL url("https://www.facebook.com/test.html");
	ok = ok && url.domain_without_tld() == "facebook";

	return ok;
}
