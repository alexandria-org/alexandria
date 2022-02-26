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

#include "text/Text.h"

BOOST_AUTO_TEST_SUITE(text)

BOOST_AUTO_TEST_CASE(get_full_text_words) {
	vector<string> words = Text::get_full_text_words("C++ map");
	/*for (const string &word : words) {
		std::cout << word << std::endl;
	}*/
}

BOOST_AUTO_TEST_CASE(get_words_without_stopwords) {
	vector<string> words = Text::get_words_without_stopwords("Hej asd!asd jag, heter! !josef. cullhed 	\
		jfoidjfoai823hr9hfhwe9f8hshgohewogiqhoih");

	BOOST_CHECK_EQUAL(words.size(), 8);
	BOOST_CHECK_EQUAL(words[0], "hej");
	BOOST_CHECK_EQUAL(words[1], "asd");
	BOOST_CHECK_EQUAL(words[2], "asd");
	BOOST_CHECK_EQUAL(words[3], "jag");
	BOOST_CHECK_EQUAL(words[4], "heter");
	BOOST_CHECK_EQUAL(words[5], "josef");
	BOOST_CHECK_EQUAL(words[6], "cullhed");
	BOOST_CHECK_EQUAL(words[7], "jfoidjfoai823hr9hfhwe9f8hshgohewogiqhoih");
}

BOOST_AUTO_TEST_CASE(clean_word) {

	BOOST_CHECK_EQUAL(Text::clean_word("hej"), "hej");
	BOOST_CHECK_EQUAL(Text::clean_word("åäö"), "åäö");
	BOOST_CHECK_EQUAL(Text::clean_word("123"), "123");
	BOOST_CHECK_EQUAL(Text::clean_word("$Üç"), "");
	BOOST_CHECK_EQUAL(Text::clean_word("hejç"), "hej");
	BOOST_CHECK_EQUAL(Text::clean_word("açd"), "ad");

	BOOST_CHECK(Text::is_clean_word("hej"));
	BOOST_CHECK(Text::is_clean_word("åäö"));
	BOOST_CHECK(Text::is_clean_word("123"));
	BOOST_CHECK(!Text::is_clean_word("$Üç"));
	BOOST_CHECK(!Text::is_clean_word("hejç"));
	BOOST_CHECK(!Text::is_clean_word("açd"));

	BOOST_CHECK_EQUAL(Text::get_words_without_stopwords("hej")[0], "hej");
	BOOST_CHECK_EQUAL(Text::get_words_without_stopwords("åäö")[0], "åäö");
	BOOST_CHECK_EQUAL(Text::get_words_without_stopwords("123")[0], "123");
	BOOST_CHECK_EQUAL(Text::get_words_without_stopwords("$Üç").size(), 0);
	BOOST_CHECK_EQUAL(Text::get_words_without_stopwords("hejç").size(), 0);
	BOOST_CHECK_EQUAL(Text::get_words_without_stopwords("açd").size(), 0);

	BOOST_CHECK(Text::get_words_without_stopwords("hej josef") == vector<string>({"hej", "josef"}));
	BOOST_CHECK(Text::get_words_without_stopwords("hej, josef!") == vector<string>({"hej", "josef"}));
	BOOST_CHECK(Text::get_words_without_stopwords("hej jÜsef cullhed du är bäst") ==
		vector<string>({"hej", "cullhed", "du", "bäst"}));

	BOOST_CHECK(Text::get_words_without_stopwords("Låna! (Pengar till bilar)") ==
		vector<string>({"låna", "pengar", "bilar"}));
	BOOST_CHECK(Text::get_words_without_stopwords("Dallas Swarner | Character | zKillboard", 3) ==
		vector<string>({"dallas", "swarner", "character"}));
	BOOST_CHECK(Text::get_words_without_stopwords("Tapis Fleur des Champs Moutarde | Zen Dos", 3) ==
		vector<string>({"tapis", "fleur", "des"}));
	BOOST_CHECK(Text::get_words_without_stopwords("Gina Osorno & The Dreamers", 3) ==
		vector<string>({"gina", "osorno", "dreamers"}));

	BOOST_CHECK(Text::get_words_without_stopwords("IMG_2190 | Zhenyu (Tony) Tian") ==
		vector<string>({"zhenyu", "tony", "tian"}));
	BOOST_CHECK(Text::get_words_without_stopwords("Tills alla dör - Diamant Salihu - Bok (9789189061842) | Bokus", 3)
		== vector<string>({"tills", "dör", "diamant"}));

	BOOST_CHECK(Text::get_words_without_stopwords("Messages postés par Prechan • Forum • Zeste de Savoir", 3) ==
		vector<string>({"messages", "par", "prechan"}));
	BOOST_CHECK(Text::get_words_without_stopwords("Science SARU – 紙本分格") == vector<string>({"science", "saru"}));
	BOOST_CHECK(Text::get_words_without_stopwords("Realiteti i trishtë shqiptar përmes fotove të gazetarit gjerman që komunizmi nuk i lejoi \
		të bëheshin publike | Gazeta Malesia", 3) == vector<string>({"realiteti", "shqiptar", "fotove"}));
	BOOST_CHECK(Text::get_words_without_stopwords("York County, VA") == vector<string>({"york", "county", "va"}));
	BOOST_CHECK(Text::get_words_without_stopwords("HTML Sitemap 14 - zfreeti.com", 3) ==
		vector<string>({"html", "sitemap", "14"}));
	BOOST_CHECK(Text::get_words_without_stopwords("HTML Sitemap 14 - zfreeti.com") ==
		vector<string>({"html", "sitemap", "14"}));
	BOOST_CHECK(Text::get_words_without_stopwords("Archives.com zfreeti.com best. stream. in .the world") ==
		vector<string>({"best", "stream", "world"}));

}

BOOST_AUTO_TEST_CASE(word_freq, * boost::unit_test::tolerance(0.00001)) {
	auto freq = Text::get_word_frequency("hello my name is josef and it is good");
	BOOST_TEST(freq["hello"] == 1.0/9.0);
	BOOST_TEST(freq["is"] == 2.0/9.0);
}

BOOST_AUTO_TEST_SUITE_END()
