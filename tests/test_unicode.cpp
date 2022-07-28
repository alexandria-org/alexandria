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
#include "parser/unicode.h"

BOOST_AUTO_TEST_SUITE(unicode)

BOOST_AUTO_TEST_CASE(unicode) {
	BOOST_CHECK_EQUAL(parser::unicode::encode("hej jag heter josef"), "hej jag heter josef");
	BOOST_CHECK_EQUAL(parser::unicode::encode("hej jag heter josef och jag tillåter utf8 åäö chars$€"),
		"hej jag heter josef och jag tillåter utf8 åäö chars$€");
	BOOST_CHECK_EQUAL(parser::unicode::encode("是美国民主党政治家，于19世纪下半叶担"), "是美国民主党政治家，于19世纪下半叶担");

	BOOST_CHECK(parser::unicode::is_valid(parser::unicode::encode("L�gg i varukorg Om produkten Specifikation Anv�ndning Våra bönor är \
		rika på protein, mineraler och fibrer. Smaken är söt och konsistensen le")));
}

BOOST_AUTO_TEST_SUITE_END()
