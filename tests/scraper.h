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

#include "scraper/Scraper.h"
#include <queue>
#include <vector>

BOOST_AUTO_TEST_SUITE(scraper)

BOOST_AUTO_TEST_CASE(scraper) {

	Scraper::store store;

	Scraper::scraper scraper("example.com", &store);
	scraper.push_url(URL("http://omnible.se/"));

	scraper.run();

	/*string last = store.tail();
	vector<string> cols;
	boost::algorithm::split(last, cols, boost::is_any_of("\t"));
	BOOST_CHECK_EQUAL(cols[0], "https://example.com/");
	BOOST_CHECK_EQUAL(cols[1], "Example Domain");*/
}

BOOST_AUTO_TEST_SUITE_END()
