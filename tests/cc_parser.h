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

#include "config.h"
#include "parser/Warc.h"

BOOST_AUTO_TEST_SUITE(cc_parser)

BOOST_AUTO_TEST_CASE(parse_cc_batch) {
	ifstream infile(Config::test_data_path + "bokus_test.warc.gz", ios::binary);

	Warc::Parser pp;
	pp.parse_stream(infile);

	stringstream ss(pp.result());
	string line;
	bool found_url = false;
	while (getline(ss, line)) {
		vector<string> cols;
		boost::algorithm::split(cols, line, boost::is_any_of("\t"));

		if (cols[0] == "https://www.bokus.com/recension/670934") {
			BOOST_CHECK(cols[4].substr(0, 120) == "Recenserad produkt Los Angeles's Original Farmers Market Häftad (Trade Paper) Mycket intressant läsning om hur Farmers");
			found_url = true;
		}
	}

	BOOST_CHECK(found_url);
}

BOOST_AUTO_TEST_CASE(parse_cc_batch_multistream) {

	string response;
	{
		Warc::Parser pp;
		ifstream infile(Config::test_data_path + "warc_test.gz", ios::binary);
		pp.parse_stream(infile);

		response = pp.result();
	}

	vector<string> files = {
		Config::test_data_path + "warc_test.gz.aa",
		Config::test_data_path + "warc_test.gz.ab",
		Config::test_data_path + "warc_test.gz.ac",
		Config::test_data_path + "warc_test.gz.ad",
		Config::test_data_path + "warc_test.gz.ae",
		Config::test_data_path + "warc_test.gz.af",
		Config::test_data_path + "warc_test.gz.ag",
		Config::test_data_path + "warc_test.gz.ah",
		Config::test_data_path + "warc_test.gz.ai",
		Config::test_data_path + "warc_test.gz.aj"
	};

	Warc::Parser pp;

	for (const string &filename : files) {
		ifstream infile(filename, ios::binary);
		pp.parse_stream(infile);
	}

	BOOST_CHECK_EQUAL(pp.result().size(), response.size());
}

BOOST_AUTO_TEST_SUITE_END()
