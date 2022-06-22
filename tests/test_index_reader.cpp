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
#include "indexer/index_builder.h"
#include "indexer/index.h"
#include "indexer/generic_record.h"
#include "indexer/level.h"
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include "URL.h"
#include "text/text.h"
#include "profiler/profiler.h"
#include "roaring/roaring.hh"

BOOST_AUTO_TEST_SUITE(test_index_reader)

BOOST_AUTO_TEST_CASE(test_index_reader) {

	{
		indexer::index_builder<indexer::generic_record> idx("test_db", 0, 1000);

		idx.truncate();

		idx.add(100, indexer::generic_record(1000));
		idx.add(100, indexer::generic_record(1001));
		idx.add(100, indexer::generic_record(1002));

		idx.append();
		idx.merge();

	}

	{
		ifstream reader("/mnt/0/full_text/test_db/0.data", ios::binary);
		reader.seekg(0, ios::end);
		size_t file_size = reader.tellg();
		reader.seekg(0, ios::beg);
		char *buffer = new char[file_size];
		reader.read(buffer, file_size);

		std::string file_data(buffer, file_size);

		std::istringstream ram_reader(file_data);

		indexer::index<indexer::generic_record> idx(&ram_reader, 1000);

		vector<indexer::generic_record> res = idx.find(100);

		BOOST_REQUIRE(res.size() == 3);
		BOOST_CHECK(res[0].m_value == 1000);
		BOOST_CHECK(res[1].m_value == 1001);
		BOOST_CHECK(res[2].m_value == 1002);

		delete buffer;
	}

}

BOOST_AUTO_TEST_CASE(test_index_reader_2) {

	/*
	{
		indexer::index_builder<indexer::url_record> idx("restaurantbusinessonline.com");
		idx.set_hash_table_size(1000);

		idx.truncate();

		const vector<size_t> cols = {1, 2, 3, 4};

		vector<string> files;

		boost::filesystem::path p ("./output");
		boost::filesystem::directory_iterator end_itr;

		for (boost::filesystem::directory_iterator itr(p); itr != end_itr; ++itr) {
			// If it's not a directory, list it. If you want to list directories too, just remove this check.
			if (boost::filesystem::is_regular_file(itr->path())) {
				// assign current file name to current_file and echo it out to the console.
				string current_file = itr->path().string();
				files.push_back(current_file);
			}
		}

		size_t num_added = 0;
		size_t num_bytes_added = 0;

		for (const string &local_path : files) {

			ifstream infile(local_path, ios::in);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				vector<string> col_values;
				boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

				URL url(col_values[0]);

				if (url.host() != "doodlecraftblog.com") continue;

				num_added++;

				uint64_t url_hash = url.hash();

				for (size_t col : cols) {
					vector<string> words = text::get_full_text_words(col_values[col]);
					for (const string &word : words) {
						num_bytes_added += word.size();
						idx.add(::algorithm::hash(word), ::indexer::url_record(url_hash));
					}
				}
			}
		}

		num_added++;

		cout << "ADDED " << num_added << " URLS" << endl;
		cout << num_bytes_added << " bytes" << endl;

		idx.append();
		idx.merge();

	}

	{
		logger::verbose(true);
		profiler::instance prof("load index file to ram");
		ifstream reader("restaurantbusinessonline.com.data", ios::binary);
		reader.seekg(0, ios::end);
		size_t file_size = reader.tellg();
		reader.seekg(0, ios::beg);
		char *buffer = new char[file_size];
		reader.read(buffer, file_size);
		prof.stop();

		indexer::index_reader_ram ram_reader(buffer, file_size);

		indexer::index<indexer::generic_record> idx((indexer::index_reader *)&ram_reader, 1000);

		cout << "file_size: " << file_size << endl;
		idx.print_stats();

		vector<indexer::generic_record> res = idx.find(::algorithm::hash("helicopter"));

		BOOST_REQUIRE(res.size() > 0);

		delete buffer;
	}*/

}

BOOST_AUTO_TEST_SUITE_END()
