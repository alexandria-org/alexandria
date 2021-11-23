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

#include "algorithm/HyperBall.h"

BOOST_AUTO_TEST_SUITE(hyper_ball)

BOOST_AUTO_TEST_CASE(harmonic_centrality_hyper_ball) {

	{
		set<pair<uint32_t, uint32_t>> e = {
			make_pair(0, 1),
			make_pair(1, 2),
			make_pair(2, 0),
			make_pair(2, 3),
			make_pair(3, 4),
			make_pair(3, 5),
			make_pair(4, 2),
			make_pair(5, 4),
		};
		const size_t n = 1000;
		vector<uint32_t> *edge_map = Algorithm::set_to_edge_map(n, e);
		vector<double> h = Algorithm::hyper_ball(n, edge_map);
		delete [] edge_map;
		BOOST_CHECK(h.size() == n);
		BOOST_CHECK_CLOSE(h[0], 8.0/3.0, 0.000001);
		BOOST_CHECK_CLOSE(h[1], 7.0/3.0, 0.000001);
		BOOST_CHECK_CLOSE(h[2], 7.0/2.0, 0.000001);
		BOOST_CHECK_EQUAL(h[6], 0.0);
	}

}

BOOST_AUTO_TEST_CASE(harmonic_centrality_hyper_ball2) {

	{
		set<pair<uint32_t, uint32_t>> e = {
			make_pair(0, 1),
			make_pair(1, 5),
			make_pair(2, 5),
			make_pair(3, 2),
			make_pair(6, 2),
			make_pair(7, 3),
			make_pair(10, 7),
			make_pair(7, 9),
			make_pair(9, 3),
			make_pair(9, 6),
			make_pair(8, 9),
			make_pair(4, 8),
		};
		const size_t n = 1000;
		vector<uint32_t> *edge_map = Algorithm::set_to_edge_map(n, e);
		vector<double> h = Algorithm::hyper_ball(n, edge_map);
		delete [] edge_map;
		BOOST_CHECK(h.size() == n);
		BOOST_CHECK_CLOSE(h[5], 4.86666666667, 0.000001);
		BOOST_CHECK_CLOSE(h[8], 1.0, 0.000001);
		BOOST_CHECK_CLOSE(h[2], 3.91666666667, 0.000001);
	}

}

BOOST_AUTO_TEST_CASE(harmonic_centrality_hyper_ball3) {

	{
		set<pair<uint32_t, uint32_t>> e = {
			make_pair(0, 11),
			make_pair(1, 0),
			make_pair(2, 1),
			make_pair(3, 2),
			make_pair(3, 8),
			make_pair(4, 7),
			make_pair(5, 7),
			make_pair(6, 7),
			make_pair(7, 8),
			make_pair(10, 12),
			make_pair(11, 1),
			make_pair(11, 10),
			make_pair(12, 25),
			make_pair(13, 9),
			make_pair(13, 14),
			make_pair(14, 9),
			make_pair(14, 8),
			make_pair(14, 15),
			make_pair(15, 7),
			make_pair(19, 15),
			make_pair(20, 21),
			make_pair(21, 16),
			make_pair(21, 17),
			make_pair(21, 18),
			make_pair(21, 22),
			make_pair(22, 23),
			make_pair(23, 19),
			make_pair(24, 20),
			make_pair(24, 21),
			make_pair(24, 25),
			make_pair(25, 24),
		};
		const size_t n = 1000;
		vector<uint32_t> *edge_map = Algorithm::set_to_edge_map(n, e);
		vector<double> h = Algorithm::hyper_ball(n, edge_map);
		delete [] edge_map;
		BOOST_CHECK(h.size() == n);
		BOOST_CHECK_CLOSE(h[0], 2.33333333333, 0.000001);
		BOOST_CHECK_CLOSE(h[7], 7.25156232656, 0.000001);
	}

}

BOOST_AUTO_TEST_SUITE_END();

