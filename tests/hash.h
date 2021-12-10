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

//#include "algorithm/HyperBall.h"
#include "hash/Hash.h"

BOOST_AUTO_TEST_SUITE(hash)

BOOST_AUTO_TEST_CASE(str) {

	BOOST_CHECK_EQUAL(Hash::str("testing"), 4540905123118180926ull);
	BOOST_CHECK_EQUAL(Hash::str(""), 6142509188972423790ull);
	BOOST_CHECK_EQUAL(Hash::str("abcdefghijklmnopqrstuvxyz"), 17219978627035894604ull);
	BOOST_CHECK_EQUAL(Hash::str("123"), 10089081994332581363ull);
	BOOST_CHECK_EQUAL(Hash::str("1234"), 15651099383784684535ull);

}

BOOST_AUTO_TEST_SUITE_END()
