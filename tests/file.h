/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "transfer/Transfer.h"
#include "text/text.h"
#include "file/TsvFileRemote.h"
#include "hash/Hash.h"

BOOST_AUTO_TEST_SUITE(file)

BOOST_AUTO_TEST_CASE(transfer_test) {
	int error;
	{
		string result = Transfer::file_to_string("/test-data/example.txt", error);
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}

	{
		string result = Transfer::gz_file_to_string("/test-data/example.txt.gz", error);
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}

	{
		string result = Transfer::file_to_string("test-data/example.txt", error);
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}

	{
		string result = Transfer::gz_file_to_string("test-data/example.txt.gz", error);
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}

	{
		stringstream ss;
		Transfer::file_to_stream("/test-data/example.txt", ss, error);
		string result = ss.str();
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}

	{
		stringstream ss;
		Transfer::gz_file_to_stream("/test-data/example.txt.gz", ss, error);
		string result = ss.str();
		BOOST_CHECK(error == Transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}
}

BOOST_AUTO_TEST_CASE(handle_errors) {
	int error;
	{
		string result = Transfer::file_to_string("/non-existing.txt", error);
		BOOST_CHECK(error == Transfer::ERROR);
	}

	{
		string result = Transfer::gz_file_to_string("/non-existing.txt.gz", error);
		BOOST_CHECK(error == Transfer::ERROR);
	}

	{
		stringstream ss;
		Transfer::file_to_stream("/non-existing.txt", ss, error);
		BOOST_CHECK(error == Transfer::ERROR);
	}

	{
		stringstream ss;
		Transfer::gz_file_to_stream("/non-existing.txt.gz", ss, error);
		BOOST_CHECK(error == Transfer::ERROR);
	}

	{
		vector<string> downloaded = Transfer::download_gz_files_to_disk({"/non-existing.txt.gz"});
		BOOST_CHECK(downloaded.size() == 0);
	}
}

BOOST_AUTO_TEST_CASE(tsv_file_exists) {
	File::TsvFileRemote manual_paths_file("crawl-data/ALEXANDRIA-MANUAL-01/warc.paths.gz");
	vector<string> warc_paths;
	manual_paths_file.read_column_into(0, warc_paths);

	BOOST_CHECK(manual_paths_file.is_open());
	BOOST_CHECK(warc_paths.size() > 0);
	BOOST_CHECK(warc_paths[0] == "crawl-data/ALEXANDRIA-MANUAL-01/files/top_domains.txt.gz");
}

BOOST_AUTO_TEST_CASE(tsv_file_dont_exists) {
	File::TsvFileRemote manual_paths_file("non-existing-file.gz");
	BOOST_CHECK(!manual_paths_file.is_open());
}

BOOST_AUTO_TEST_CASE(local_tsv_files) {

	File::TsvFile my_file(Config::test_data_path + "tsvtest.tsv");

	BOOST_CHECK_EQUAL(my_file.find_first_position("aaa"), 0);
	BOOST_CHECK_EQUAL(my_file.find_first_position("aab"), 126);
	BOOST_CHECK_EQUAL(my_file.find_first_position("european"), string::npos);

	BOOST_CHECK_EQUAL(my_file.find_last_position("aaa"), 112);
	BOOST_CHECK_EQUAL(my_file.find_last_position("aab"), 126);
	BOOST_CHECK_EQUAL(my_file.find_last_position("european"), string::npos);

	File::TsvFile my_file2(Config::test_data_path + "tsvtest2.tsv");

	BOOST_CHECK_EQUAL(my_file2.find_first_position("aaa"), 0);
	BOOST_CHECK(my_file2.find_first_position("aab") > 0);
	BOOST_CHECK_EQUAL(my_file2.find_first_position("european"), string::npos);

	BOOST_CHECK(my_file2.find_last_position("aaa") > 0 && my_file2.find_last_position("aaa") < my_file2.size());
	BOOST_CHECK(my_file2.find_last_position("aab") > 0 && my_file2.find_last_position("aab") < my_file2.size());
	BOOST_CHECK(my_file2.find_last_position("aac") > 0 && my_file2.find_last_position("aac") == my_file2.size() - 115);
	BOOST_CHECK(my_file2.find_last_position("european") == string::npos);

	BOOST_CHECK_EQUAL(my_file2.find_next_position("aaa"), my_file2.find_first_position("aab"));
	BOOST_CHECK_EQUAL(my_file2.find_next_position("aab"), my_file2.find_first_position("aac"));
	BOOST_CHECK_EQUAL(my_file2.find_next_position("aabb"), my_file2.find_first_position("aac"));
	BOOST_CHECK_EQUAL(my_file2.find_next_position("aac"), my_file2.size());
}

BOOST_AUTO_TEST_CASE(head_content_len) {

	{
		int error;
		size_t content_len = Transfer::head_content_length("http://127.0.0.1/test-data/automobileszone.com", error);
		BOOST_CHECK_EQUAL(error, Transfer::OK);
		BOOST_CHECK_EQUAL(content_len, 8084);
	}

	{
		int error;
		size_t content_len = Transfer::head_content_length("http://localhost/test-data/10272145489625484395-1002.gz", error);
		BOOST_CHECK_EQUAL(error, Transfer::OK);
		BOOST_CHECK_EQUAL(content_len, 59535774);
	}

	{
		int error;
		size_t content_len = Transfer::head_content_length("http://127.0.0.1/test-data/automobileszone.com-not-here", error);
		BOOST_CHECK_EQUAL(error, Transfer::ERROR);
		BOOST_CHECK_EQUAL(content_len, 0);
	}

}

BOOST_AUTO_TEST_CASE(test_upload) {

	{
		int error;
		string buffer;
		Transfer::url_to_string("http://alexandria-test-data.s3.amazonaws.com/multipart_test", buffer, error);
		BOOST_CHECK_EQUAL(error, Transfer::OK);

		error = Transfer::upload_file("multipart_test", buffer);
		BOOST_CHECK_EQUAL(error, Transfer::OK);
	}
}

BOOST_AUTO_TEST_CASE(test_upload_gz) {

	{
		int error;
		string buffer;
		Transfer::url_to_string("http://alexandria-test-data.s3.amazonaws.com/multipart_test", buffer, error);
		BOOST_CHECK_EQUAL(error, Transfer::OK);

		error = Transfer::upload_gz_file("multipart_test.gz", buffer);
		BOOST_CHECK_EQUAL(error, Transfer::OK);

		// Download it again as gz file and see if we get the same result.
		
		const string result_back = Transfer::gz_file_to_string("multipart_test.gz", error);
		BOOST_CHECK_EQUAL(error, Transfer::OK);

		BOOST_CHECK_EQUAL(result_back.size(), buffer.size());
		BOOST_CHECK_EQUAL(Hash::str(result_back), Hash::str(buffer));
	}
}

BOOST_AUTO_TEST_SUITE_END()
