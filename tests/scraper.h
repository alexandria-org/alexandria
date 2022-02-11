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

#include "scraper/scraper.h"
#include <queue>
#include <vector>

BOOST_AUTO_TEST_SUITE(scraper)

BOOST_AUTO_TEST_CASE(scraper) {

	Scraper::store store;

	Scraper::scraper scraper("omnible.se", &store);
	scraper.set_timeout(0);
	scraper.push_url(URL("http://omnible.se/"));
	scraper.push_url(URL("http://omnible.se/10126597891759986715"));
	scraper.push_url(URL("http://omnible.se/10123997891267016458"));
	scraper.push_url(URL("http://omnible.se/gtin/9789180230865"));
	scraper.push_url(URL("http://omnible.se/10123697814011564169"));
	scraper.push_url(URL("https://www.omnible.se/notfound"));
	scraper.push_url(URL("https://www.omnible.se/gtin/9789177714958"));

	scraper.run();

	string last = store.tail();
	vector<string> cols;
	boost::algorithm::split(cols, last, boost::is_any_of("\t"));
	BOOST_CHECK_EQUAL(cols[0], "https://www.omnible.se/10123697814011564169");
	BOOST_CHECK_EQUAL(cols[1], "Den sista gåvan av Abdulrazak Gurnah - recensioner & prisjämförelse - Omnible");
}

BOOST_AUTO_TEST_CASE(scraper_multithreaded) {

	return;

	vector<string> urls = {
		/*"http://omnible.se/",
		"http://omnible.se/10126597891759986715",
		"http://omnible.se/10123997891267016458",
		"https://spelagratis.nu/",
		"https://spelagratis.nu/super_mario_world.html",
		"http://omnible.se/gtin/9789180230865",
		"http://omnible.se/10123697814011564169",
		"https://spelagratis.nu/dirt_bike.html"*/
		"http://optout.aboutads.info/",
		"http://tabernus.com/",
		"http://tabernus.com/test",
		"http://apnews.excite.com/article/20071031/D8SKBRKO0.html",
		"http://thebetter.wiki/en/Jeb_Magruder",
		"https://www.thebetter.wiki/en/testing"
	};

	Scraper::run_scraper_on_urls(urls);
}

BOOST_AUTO_TEST_SUITE_END()
