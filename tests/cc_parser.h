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
#include "parser/URL.h"

BOOST_AUTO_TEST_SUITE(cc_parser)

BOOST_AUTO_TEST_CASE(download_warc) {

	Logger::start_logger_thread();

	string buffer;
	Warc::multipart_download("http://alexandria-test-data.s3.amazonaws.com/multipart_test", [&buffer](const string &data) {
		buffer.append(data);
	});

	BOOST_CHECK_EQUAL(buffer.size(), 15728640);
	BOOST_CHECK_EQUAL(Hash::str(buffer), 1803966798292769636ull);

	Logger::join_logger_thread();
}

BOOST_AUTO_TEST_CASE(parse_cc_batch) {
	ifstream infile(Config::test_data_path + "bokus_test.warc.gz", ios::binary);

	Warc::Parser pp;
	pp.parse_stream(infile);

	{
		stringstream ss(pp.result());
		string line;
		bool found_url = false;
		while (getline(ss, line)) {
			vector<string> cols;
			boost::algorithm::split(cols, line, boost::is_any_of("\t"));

			if (cols[0] == "https://www.bokus.com/recension/670934") {
				BOOST_CHECK(cols[1].substr(0, 26) == "Mycket intressant läsning");
				BOOST_CHECK(cols[2].substr(0, 25) == "Recension av Lena Klippvi");
				BOOST_CHECK(cols[3].substr(0, 25) == "Mycket intressant läsnin");
				BOOST_CHECK(cols[4].substr(0, 120) == "Recenserad produkt Los Angeles's Original Farmers Market Häftad (Trade Paper) Mycket intressant läsning om hur Farmers");
				BOOST_CHECK(cols[5] == "2021-07-31T20:08:45Z");
				BOOST_CHECK(cols[6] == "213.187.205.190");
				found_url = true;
			}
		}
		BOOST_CHECK(found_url);
	}

	{
		stringstream ss(pp.link_result());
		string line;
		int links_found = 0;
		while (getline(ss, line)) {
			vector<string> cols;
			boost::algorithm::split(cols, line, boost::is_any_of("\t"));

			if (links_found == 0) {
				BOOST_CHECK(cols[0] == "bokus.com");
				BOOST_CHECK(cols[1] == "/recension/670934");
				BOOST_CHECK(cols[2] == "help.bokus.com");
				BOOST_CHECK(cols[3] == "/");
				BOOST_CHECK(cols[4] == "Vanliga frågor & svar");
			}
			links_found++;
		}
		BOOST_CHECK_EQUAL(links_found, 8);
	}

	/*{
		const char *internal_links = pp.internal_link_result().c_str();
		{
			const uint64_t hash1 = *((uint64_t *)&internal_links[0]);
			const uint64_t hash2 = *((uint64_t *)&internal_links[8]);
			BOOST_CHECK_EQUAL(hash1, URL("https://www.bokus.com/recension/670934").hash());
			BOOST_CHECK_EQUAL(hash2, URL("https://www.bokus.com/cgi-bin/logout_user_info.cgi").hash());
		}
		{
			const uint64_t hash1 = *((uint64_t *)&internal_links[16]);
			const uint64_t hash2 = *((uint64_t *)&internal_links[24]);
			BOOST_CHECK_EQUAL(hash1, URL("https://www.bokus.com/recension/670934").hash());
			BOOST_CHECK_EQUAL(hash2, URL("https://www.bokus.com/cgi-bin/log_in_real.cgi").hash());
		}
	}*/
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

BOOST_AUTO_TEST_CASE(parse_cc_batch_301) {

	Warc::Parser pp;
	ifstream infile(Config::test_data_path + "long_warc.gz", ios::binary);
	pp.parse_stream(infile);

}

BOOST_AUTO_TEST_SUITE_END()
