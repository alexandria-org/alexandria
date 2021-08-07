

#include "test4.h"
#include "file/Transfer.h"
#include "file/TsvFileRemote.h"
#include "parser/URL.h"
#include "text/Text.h"
#include "full_text/FullText.h"

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

/*
 * Test some FullText routines
 * */
int test4_3(void) {
	int ok = 1;

	{
		auto partition = FullText::make_partition_from_files({"file1", "file2", "file3", "file4"}, 1, 3);
		ok = ok && partition.size() == 1;
		ok = ok && partition[0] == "file2";
	}

	{
		auto partition = FullText::make_partition_from_files({"file1", "file2", "file3", "file4"}, 0, 3);
		ok = ok && partition.size() == 2;
		ok = ok && partition[0] == "file1";
		ok = ok && partition[1] == "file4";
	}

	return ok;
}

/*
 * Test tsv files
 * */
int test4_4(void) {
	int ok = 1;

	{
		TsvFileRemote manual_paths_file("crawl-data/ALEXANDRIA-MANUAL-01/warc.paths.gz");
		vector<string> warc_paths;
		manual_paths_file.read_column_into(0, warc_paths);

		ok = ok && manual_paths_file.is_open();
		ok = ok && warc_paths.size() > 0;
		ok = ok && warc_paths[0] == "/crawl-data/ALEXANDRIA-MANUAL-01/files/top_domains.txt.gz";
	}

	{
		TsvFileRemote manual_paths_file("non-existing-file.gz");
		ok = ok && !manual_paths_file.is_open();
	}

	return ok;
}
