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

#include <boost/test/unit_test.hpp>
#include "transfer/transfer.h"
#include "text/text.h"
#include "file/file.h"
#include "file/tsv_file_remote.h"
#include "file/tsv_file.h"
#include "file/archive.h"
#include "algorithm/hash.h"
#include "config.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(test_file)

BOOST_AUTO_TEST_CASE(transfer_test) {
	int error;
	{
		string result = transfer::file_to_string("/test-data/example.txt", error);
		BOOST_CHECK(error == transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}

	{
		string result = transfer::gz_file_to_string("/test-data/example.txt.gz", error);
		BOOST_CHECK(error == transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}

	{
		string result = transfer::file_to_string("test-data/example.txt", error);
		BOOST_CHECK(error == transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}

	{
		string result = transfer::gz_file_to_string("test-data/example.txt.gz", error);
		BOOST_CHECK(error == transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}

	{
		stringstream ss;
		transfer::file_to_stream("/test-data/example.txt", ss, error);
		string result = ss.str();
		BOOST_CHECK(error == transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}

	{
		stringstream ss;
		transfer::gz_file_to_stream("/test-data/example.txt.gz", ss, error);
		string result = ss.str();
		BOOST_CHECK(error == transfer::OK);
		BOOST_CHECK(text::trim(result) == "An example file");
	}
}

BOOST_AUTO_TEST_CASE(handle_errors) {
	int error;
	{
		string result = transfer::file_to_string("/non-existing.txt", error);
		BOOST_CHECK(error == transfer::ERROR);
	}

	{
		string result = transfer::gz_file_to_string("/non-existing.txt.gz", error);
		BOOST_CHECK(error == transfer::ERROR);
	}

	{
		stringstream ss;
		transfer::file_to_stream("/non-existing.txt", ss, error);
		BOOST_CHECK(error == transfer::ERROR);
	}

	{
		stringstream ss;
		transfer::gz_file_to_stream("/non-existing.txt.gz", ss, error);
		BOOST_CHECK(error == transfer::ERROR);
	}

	{
		vector<string> downloaded = transfer::download_gz_files_to_disk({"/non-existing.txt.gz"});
		BOOST_CHECK(downloaded.size() == 0);
	}
}

BOOST_AUTO_TEST_CASE(tsv_file_exists) {
	file::tsv_file_remote manual_paths_file("crawl-data/ALEXANDRIA-MANUAL-01/warc.paths.gz");
	vector<string> warc_paths;
	manual_paths_file.read_column_into(0, warc_paths);

	BOOST_CHECK(manual_paths_file.is_open());
	BOOST_CHECK(warc_paths.size() > 0);
	BOOST_CHECK(warc_paths[0] == "crawl-data/ALEXANDRIA-MANUAL-01/files/top_domains.txt.gz");
}

BOOST_AUTO_TEST_CASE(tsv_file_dont_exists) {
	file::tsv_file_remote manual_paths_file("non-existing-file.gz");
	BOOST_CHECK(!manual_paths_file.is_open());
}

BOOST_AUTO_TEST_CASE(local_tsv_files) {

	file::tsv_file my_file(config::test_data_path + "tsvtest.tsv");

	BOOST_CHECK_EQUAL(my_file.find_first_position("aaa"), 0);
	BOOST_CHECK_EQUAL(my_file.find_first_position("aab"), 126);
	BOOST_CHECK_EQUAL(my_file.find_first_position("european"), string::npos);

	BOOST_CHECK_EQUAL(my_file.find_last_position("aaa"), 112);
	BOOST_CHECK_EQUAL(my_file.find_last_position("aab"), 126);
	BOOST_CHECK_EQUAL(my_file.find_last_position("european"), string::npos);

	file::tsv_file my_file2(config::test_data_path + "tsvtest2.tsv");

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
		size_t content_len = transfer::head_content_length("http://127.0.0.1/test-data/automobileszone.com", error);
		BOOST_CHECK_EQUAL(error, transfer::OK);
		BOOST_CHECK_EQUAL(content_len, 8084);
	}

	{
		int error;
		size_t content_len = transfer::head_content_length("http://127.0.0.1/test-data/automobileszone.com-not-here", error);
		BOOST_CHECK_EQUAL(error, transfer::ERROR);
		BOOST_CHECK_EQUAL(content_len, 0);
	}

}

BOOST_AUTO_TEST_CASE(test_upload) {
	// This amazon bucket is gone.
	/*{
		int error;
		string buffer;
		transfer::url_to_string("http://alexandria-test-data.s3.amazonaws.com/multipart_test", buffer, error);
		BOOST_CHECK_EQUAL(error, transfer::OK);

		error = transfer::upload_file("multipart_test", buffer);
		BOOST_CHECK_EQUAL(error, transfer::OK);
	}*/
}

BOOST_AUTO_TEST_CASE(test_upload_gz) {
	// This amazon bucket is gone.
	/*{
		int error;
		string buffer;
		transfer::url_to_string("http://alexandria-test-data.s3.amazonaws.com/multipart_test", buffer, error);
		BOOST_CHECK_EQUAL(error, transfer::OK);

		error = transfer::upload_gz_file("multipart_test.gz", buffer);
		BOOST_CHECK_EQUAL(error, transfer::OK);

		// Download it again as gz file and see if we get the same result.
		
		const string result_back = transfer::gz_file_to_string("multipart_test.gz", error);
		BOOST_CHECK_EQUAL(error, transfer::OK);

		BOOST_CHECK_EQUAL(result_back.size(), buffer.size());
		BOOST_CHECK_EQUAL(algorithm::hash(result_back), algorithm::hash(buffer));
	}*/
}

/*
 * Test the tsv_file::read_column_into function that is used a lot.
 * */
BOOST_AUTO_TEST_CASE(test_tsv_file) {

	{
		file::tsv_file tsv(config::test_data_path + "tsvtest3.tsv");
		vector<string> vec;
		tsv.read_column_into(0, vec, 2, 3);

		BOOST_CHECK(vec.size() == 2);
		BOOST_CHECK(vec[0] == "line4");
		BOOST_CHECK(vec[1] == "line5");
	}

	{
		file::tsv_file tsv(config::test_data_path + "tsvtest3.tsv");
		set<string> data;
		tsv.read_column_into(0, data, 2, 3);

		BOOST_CHECK(data.size() == 2);
		BOOST_CHECK(data.count("line4") == 1);
		BOOST_CHECK(data.count("line5") == 1);
	}

	{
		file::tsv_file tsv(config::test_data_path + "tsvtest3.tsv");
		vector<string> vec;
		tsv.read_column_into(0, vec, 100, 3);

		BOOST_CHECK(vec.size() == 3);
		BOOST_CHECK(vec[0] == "line4");
		BOOST_CHECK(vec[1] == "line5");
		BOOST_CHECK(vec[2] == "line6");
	}

	{
		file::tsv_file tsv(config::test_data_path + "tsvtest3.tsv");
		set<string> data;
		tsv.read_column_into(0, data, 100, 3);

		BOOST_CHECK(data.size() == 3);
		BOOST_CHECK(data.count("line4") == 1);
		BOOST_CHECK(data.count("line5") == 1);
		BOOST_CHECK(data.count("line6") == 1);
	}

	{
		file::tsv_file tsv(config::test_data_path + "tsvtest3.tsv");
		vector<string> vec;
		tsv.read_column_into(0, vec, 3, 0);

		BOOST_CHECK(vec.size() == 3);
		BOOST_CHECK(vec[0] == "line1");
		BOOST_CHECK(vec[1] == "line2");
		BOOST_CHECK(vec[2] == "line3");
	}

	{
		file::tsv_file tsv(config::test_data_path + "tsvtest3.tsv");
		set<string> data;
		tsv.read_column_into(0, data, 3, 0);

		BOOST_CHECK(data.size() == 3);
		BOOST_CHECK(data.count("line1") == 1);
		BOOST_CHECK(data.count("line2") == 1);
		BOOST_CHECK(data.count("line3") == 1);
	}
}

/*
 * Test the file::archive simple tarball
 * */
BOOST_AUTO_TEST_CASE(test_archive) {

	{
		file::archive tar("test_dir.tar");

		file::create_directory("test_dir1");

		std::ofstream file1("test_dir1/file1.txt");
		file1 << "hello world 1";
		file1.close();

		std::ofstream file2("test_dir1/file2.txt");
		file2 << "hello world 2";
		file2.close();

		std::ofstream file3("test_dir1/file3.txt");
		file3 << "hello world 3";
		file3.close();

		tar.read_dir("test_dir1");
	}

	{
		file::archive tar("test_dir.tar");

		file::create_directory("test_dir2");
		tar.untar("test_dir2");

		BOOST_CHECK_EQUAL(file::cat("test_dir2/file1.txt"), "hello world 1");
		BOOST_CHECK_EQUAL(file::cat("test_dir2/file2.txt"), "hello world 2");
		BOOST_CHECK_EQUAL(file::cat("test_dir2/file3.txt"), "hello world 3");

	}
	file::delete_directory("test_dir1");
	file::delete_directory("test_dir2");
	file::delete_file("test_dir.tar");
}

BOOST_AUTO_TEST_CASE(test_archive2) {

	{
		file::archive tar("test_dir.tar");

		file::create_directory("test_dir1");

		// Create 500 files.
		for (size_t i = 1; i <= 500; i++) {
			std::ofstream file1("test_dir1/file" + std::to_string(i) + ".txt");
			for (size_t j = 0; j < i; j++) {
				file1 << "hello world " << j << std::endl;
			}
		}

		tar.read_dir("test_dir1");
	}

	{
		file::archive tar("test_dir.tar");

		file::create_directory("test_dir2");
		tar.untar("test_dir2");

		// Check 500 files.
		for (size_t i = 1; i <= 500; i++) {
			std::ifstream file1("test_dir2/file" + std::to_string(i) + ".txt");
			std::string line;
			size_t j = 0;
			while (std::getline(file1, line)) {
				BOOST_CHECK_EQUAL(line, "hello world " + std::to_string(j));
				j++;
			}
			BOOST_CHECK_EQUAL(j, i);
		}

	}
	file::delete_directory("test_dir1");
	file::delete_directory("test_dir2");
	file::delete_file("test_dir.tar");
}

BOOST_AUTO_TEST_CASE(test_rename_file) {
	file::create_directory("/tmp/alexandria_test_98237593257");
	file::create_directory("/tmp/alexandria_test_98237593257/testdir");
	file::rename("/tmp/alexandria_test_98237593257/testdir", "/tmp/alexandria_test_98237593257/testdir2");
	BOOST_CHECK(file::file_exists("/tmp/alexandria_test_98237593257/testdir2"));
	BOOST_CHECK(!file::file_exists("/tmp/alexandria_test_98237593257/testdir"));
	file::delete_directory("/tmp/alexandria_test_98237593257");
	BOOST_CHECK(!file::file_exists("/tmp/alexandria_test_98237593257/testdir"));
	BOOST_CHECK(!file::file_exists("/tmp/alexandria_test_98237593257/testdir2"));
	BOOST_CHECK(!file::file_exists("/tmp/alexandria_test_98237593257"));

	file::create_directory("/tmp/alexandria_test_98237593257/testdir");
	BOOST_CHECK(file::file_exists("/tmp/alexandria_test_98237593257"));
	file::delete_directory("/tmp/alexandria_test_98237593257");
	BOOST_CHECK(!file::file_exists("/tmp/alexandria_test_98237593257"));
}

BOOST_AUTO_TEST_SUITE_END()
