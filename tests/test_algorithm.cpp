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
#include "algorithm/algorithm.h"
#include "algorithm/intersection.h"
#include "algorithm/hyper_ball.h"
#include "full_text/full_text_record.h"
#include "full_text/url_to_domain.h"
#include "search_engine/search_engine.h"

using namespace std;
using full_text::full_text_record;
using full_text::url_to_domain;

BOOST_AUTO_TEST_SUITE(test_algorithm)

BOOST_AUTO_TEST_CASE(intersection_test) {

	{
		const vector<int> result = algorithm::intersection<int>({
			{1, 2, 3},
			{2, 3},
			{2, 3, 4}
		});

		BOOST_CHECK_EQUAL(2, result.size());
		BOOST_CHECK_EQUAL(2, result[0]);
		BOOST_CHECK_EQUAL(3, result[1]);
	}

	{
		const vector<int> result = algorithm::intersection<int>({
			{1, 2, 3, 5},
			{2, 3, 5, 7},
			{2, 3, 4, 5}
		});

		BOOST_CHECK_EQUAL(3, result.size());
		BOOST_CHECK_EQUAL(2, result[0]);
		BOOST_CHECK_EQUAL(3, result[1]);
		BOOST_CHECK_EQUAL(5, result[2]);
	}

	{
		const vector<int> result = algorithm::intersection<int>({});

		BOOST_CHECK_EQUAL(0, result.size());
	}

	{
		const vector<int> result = algorithm::intersection<int>({
			{1, 2, 3, 5, 6, 7, 8},
			{9, 10},
			{1, 2, 3, 4, 5}
		});

		BOOST_CHECK_EQUAL(0, result.size());
	}

	{

		class T {
			public:
			size_t m_v;
			float m_s;

			T(size_t v, float s) : m_v(v), m_s(s) {}

			bool operator<(const T &other) const {
				return m_v < other.m_v;
			}

			bool operator==(const T &other) const {
				return m_v == other.m_v;
			}

		};
		const vector<T> result = algorithm::intersection<T>({
			{T(1, 1.0f), T(2, 1.0f), T(3, 1.0f), T(4, 1.0f)},
			{T(3, 2.0f), T(4, 2.0f), T(5, 2.0f)},
			{T(4, 3.0f), T(5, 3.0f), T(6, 3.0f), T(7, 3.0f), T(8, 3.0f)}
		}, [](T &a, const T &b) {
			return a.m_s += b.m_s;
		});

		BOOST_CHECK_EQUAL(1, result.size());
		BOOST_CHECK_EQUAL(result[0].m_v, 4);
		BOOST_CHECK_EQUAL(result[0].m_s, 6.0f);
	}
}

BOOST_AUTO_TEST_CASE(incremental_partitions) {

	{
		vector<vector<int>> res = algorithm::incremental_partitions({5}, 64);
		BOOST_CHECK_EQUAL(res.size(), 5);
	}
	{
		vector<vector<int>> res = algorithm::incremental_partitions({6}, 64);
		BOOST_CHECK_EQUAL(res.size(), 6);
	}
	{
		vector<vector<int>> res = algorithm::incremental_partitions({3}, 64);
		BOOST_CHECK_EQUAL(res.size(), 3);
		BOOST_CHECK(res[0] == vector<int>{0});
		BOOST_CHECK(res[1] == vector<int>{1});
		BOOST_CHECK(res[2] == vector<int>{2});
	}

	{
		vector<vector<int>> res = algorithm::incremental_partitions({2, 2}, 64);
		BOOST_CHECK_EQUAL(res.size(), 4);
		BOOST_CHECK((res[0] == vector<int>{0, 0}));
		BOOST_CHECK((res[1] == vector<int>{1, 0}));
		BOOST_CHECK((res[2] == vector<int>{0, 1}));
		BOOST_CHECK((res[3] == vector<int>{1, 1}));
	}
	{
		vector<vector<int>> res = algorithm::incremental_partitions({3, 3}, 64);
		BOOST_CHECK_EQUAL(res.size(), 9);
		BOOST_CHECK((res[0] == vector<int>{0, 0}));
		BOOST_CHECK((res[1] == vector<int>{1, 0}));
		BOOST_CHECK((res[2] == vector<int>{0, 1}));
		BOOST_CHECK((res[3] == vector<int>{1, 1}));
		BOOST_CHECK((res[4] == vector<int>{2, 0}));
		BOOST_CHECK((res[5] == vector<int>{0, 2}));
		BOOST_CHECK((res[6] == vector<int>{2, 1}));
		BOOST_CHECK((res[7] == vector<int>{1, 2}));
		BOOST_CHECK((res[8] == vector<int>{2, 2}));
	}
	{
		vector<vector<int>> res = algorithm::incremental_partitions({3, 3}, 5);
		BOOST_CHECK_EQUAL(res.size(), 5);
		BOOST_CHECK((res[0] == vector<int>{0, 0}));
		BOOST_CHECK((res[1] == vector<int>{1, 0}));
		BOOST_CHECK((res[2] == vector<int>{0, 1}));
		BOOST_CHECK((res[3] == vector<int>{1, 1}));
		BOOST_CHECK((res[4] == vector<int>{2, 0}));
	}
	{
		vector<vector<int>> res = algorithm::incremental_partitions({3, 3, 3}, 64);
		BOOST_CHECK_EQUAL(res.size(), 27);
		BOOST_CHECK((res[0] == vector<int>{0, 0, 0}));
		BOOST_CHECK((res[1] == vector<int>{1, 0, 0}));
		BOOST_CHECK((res[2] == vector<int>{0, 1, 0}));
		BOOST_CHECK((res[3] == vector<int>{0, 0, 1}));
		BOOST_CHECK((res[4] == vector<int>{1, 1, 0}));
		BOOST_CHECK((res[5] == vector<int>{1, 0, 1}));
		BOOST_CHECK((res[6] == vector<int>{0, 1, 1}));
		BOOST_CHECK((res[7] == vector<int>{2, 0, 0}));
		BOOST_CHECK((res[8] == vector<int>{0, 2, 0}));
		BOOST_CHECK((res[9] == vector<int>{0, 0, 2}));
		BOOST_CHECK((res[10] == vector<int>{1, 1, 1}));
		BOOST_CHECK((res[11] == vector<int>{2, 1, 0}));
		BOOST_CHECK((res[12] == vector<int>{2, 0, 1}));
		BOOST_CHECK((res[13] == vector<int>{1, 2, 0}));
		BOOST_CHECK((res[14] == vector<int>{1, 0, 2}));
		BOOST_CHECK((res[15] == vector<int>{0, 2, 1}));
	}
	{
		vector<vector<int>> res = algorithm::incremental_partitions({2, 3}, 64);
		BOOST_CHECK_EQUAL(res.size(), 6);
		BOOST_CHECK((res[0] == vector<int>{0, 0}));
		BOOST_CHECK((res[1] == vector<int>{1, 0}));
		BOOST_CHECK((res[2] == vector<int>{0, 1}));
		BOOST_CHECK((res[3] == vector<int>{1, 1}));
		BOOST_CHECK((res[4] == vector<int>{0, 2}));
		BOOST_CHECK((res[5] == vector<int>{1, 2}));
	}

}

BOOST_AUTO_TEST_CASE(binary_search) {
	{
		full_text_record *records = new full_text_record[10];
		records[0].m_value = 1;
		records[1].m_value = 2;
		records[2].m_value = 3;
		records[3].m_value = 4;
		records[4].m_value = 10;
		records[5].m_value = 11;
		records[6].m_value = 12;
		records[7].m_value = 13;
		records[8].m_value = 13;
		records[9].m_value = 100;
		BOOST_CHECK(search_engine::lower_bound(records, 0, 10, 13) == 7);
		BOOST_CHECK(search_engine::lower_bound(records, 0, 10, 10) == 4);
		BOOST_CHECK(search_engine::lower_bound(records, 0, 10, 100) == 9);
		BOOST_CHECK(search_engine::lower_bound(records, 0, 10, 101) == 10);

		BOOST_CHECK(search_engine::lower_bound(records, 0, 10, 5) == 4);

		BOOST_CHECK(search_engine::lower_bound(records, 5, 10, 11) == 5);
		BOOST_CHECK(search_engine::lower_bound(records, 9, 10, 100) == 9);
		BOOST_CHECK(search_engine::lower_bound(records, 10, 10, 100) == 10);
		BOOST_CHECK(search_engine::lower_bound(records, 9, 10, 101) == 10);
	}
}

BOOST_AUTO_TEST_CASE(harmonic_centrality) {
	{
		set<pair<uint32_t, uint32_t>> e = {std::make_pair(0, 1), std::make_pair(1, 2), std::make_pair(2, 0)};
		vector<double> h = algorithm::harmonic_centrality(3, e, 6);
		BOOST_CHECK(h.size() == 3);
		BOOST_CHECK((h == vector<double>{1.5, 1.5, 1.5}));
	}

	{
		set<pair<uint32_t, uint32_t>> e = {
			std::make_pair(0, 1),
			std::make_pair(1, 2),
			std::make_pair(2, 0),
			std::make_pair(2, 3),
			std::make_pair(3, 4),
			std::make_pair(3, 5),
			std::make_pair(4, 2),
			std::make_pair(5, 4),
		};
		vector<double> h = algorithm::harmonic_centrality(7, e, 6);
		BOOST_CHECK(h.size() == 7);
		BOOST_CHECK_CLOSE(h[0], 8.0/3.0, 0.000001);
		BOOST_CHECK_CLOSE(h[1], 7.0/3.0, 0.000001);
		BOOST_CHECK_CLOSE(h[2], 7.0/2.0, 0.000001);
		BOOST_CHECK_EQUAL(h[6], 0.0);
	}

	{
		set<pair<uint32_t, uint32_t>> e = {
			std::make_pair(0, 1),
			std::make_pair(1, 2),
			std::make_pair(2, 1),
			std::make_pair(3, 1),
			std::make_pair(4, 1),
			std::make_pair(5, 1),
			std::make_pair(6, 1),
			std::make_pair(7, 1),
		};
		vector<double> h = algorithm::harmonic_centrality(8, e, 6);
		BOOST_CHECK(h.size() == 8);
		BOOST_CHECK_CLOSE(h[1], 7, 0.000001);
	}
}

BOOST_AUTO_TEST_CASE(harmonic_centrality_threaded) {
	{
		set<pair<uint32_t, uint32_t>> e = {std::make_pair(0, 1), std::make_pair(1, 2), std::make_pair(2, 0)};
		vector<double> h = algorithm::harmonic_centrality_threaded(3, e, 6, 3);
		BOOST_CHECK(h.size() == 3);
		BOOST_CHECK((h == vector<double>{1.5, 1.5, 1.5}));
	}

	{
		set<pair<uint32_t, uint32_t>> e = {
			std::make_pair(0, 1),
			std::make_pair(1, 2),
			std::make_pair(2, 0),
			std::make_pair(2, 3),
			std::make_pair(3, 4),
			std::make_pair(3, 5),
			std::make_pair(4, 2),
			std::make_pair(5, 4),
		};
		vector<double> h = algorithm::harmonic_centrality_threaded(7, e, 6, 2);
		BOOST_CHECK(h.size() == 7);
		BOOST_CHECK_CLOSE(h[0], 8.0/3.0, 0.000001);
		BOOST_CHECK_CLOSE(h[1], 7.0/3.0, 0.000001);
		BOOST_CHECK_CLOSE(h[2], 7.0/2.0, 0.000001);
		BOOST_CHECK_EQUAL(h[6], 0.0);
	}

	{
		set<pair<uint32_t, uint32_t>> e = {
			std::make_pair(0, 1),
			std::make_pair(1, 2),
			std::make_pair(2, 1),
			std::make_pair(3, 1),
			std::make_pair(4, 1),
			std::make_pair(5, 1),
			std::make_pair(6, 1),
			std::make_pair(7, 1),
		};
		vector<double> h = algorithm::harmonic_centrality_threaded(8, e, 6, 1);
		BOOST_CHECK(h.size() == 8);
		BOOST_CHECK_CLOSE(h[1], 7, 0.000001);
	}
}

BOOST_AUTO_TEST_SUITE_END()
