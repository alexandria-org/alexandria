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

#include "full_text/FullTextShardBuilder.h"
#include "full_text/FullTextShard.h"

BOOST_AUTO_TEST_SUITE(shard_builder)

BOOST_AUTO_TEST_CASE(shard_builder) {

	FullTextShardBuilder<FullTextRecord> builder("single_db_test", 10);

	builder.truncate();
	builder.truncate_cache_files();

	{
		FullTextRecord record = {
			.m_value = 1111ull,
			.m_score = 0.1f,
			.m_domain_hash = 2222ull
		};

		builder.add(123456ull, record);
		builder.append();
	}

	{
		FullTextRecord record = {
			.m_value = 111ull,
			.m_score = 0.2f,
			.m_domain_hash = 222ull
		};

		builder.add(123457ull, record);
		builder.append();
	}

	builder.merge();

	{
		FullTextRecord record = {
			.m_value = 112ull,
			.m_score = 0.2f,
			.m_domain_hash = 222ull
		};

		builder.add(123457ull, record);
	}

	{
		FullTextRecord record = {
			.m_value = 113ull,
			.m_score = 0.2f,
			.m_domain_hash = 222ull
		};

		builder.add(123457ull + Config::shard_hash_table_size, record);
		builder.append();
	}

	builder.merge();

	FullTextShard<FullTextRecord> shard("single_db_test", 10);

	FullTextResultSet<FullTextRecord> result_set(Config::ft_max_results_per_section * Config::ft_max_sections);
	shard.find(123456ull, &result_set);

	BOOST_CHECK_EQUAL(result_set.size(), 1);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_value, 1111ull);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_score, 0.1f);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_domain_hash, 2222ull);

	result_set.close_sections();

	shard.find(123457ull, &result_set);

	BOOST_CHECK_EQUAL(result_set.size(), 2);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_value, 111ull);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_score, 0.2f);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_domain_hash, 222ull);

	BOOST_CHECK_EQUAL(result_set.data_pointer()[1].m_value, 112ull);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[1].m_score, 0.2f);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[1].m_domain_hash, 222ull);

	result_set.close_sections();

	shard.find(123457ull + Config::shard_hash_table_size, &result_set);

	BOOST_CHECK_EQUAL(result_set.size(), 1);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_value, 113ull);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_score, 0.2f);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_domain_hash, 222ull);
}

BOOST_AUTO_TEST_CASE(shard_builder2) {

	FullTextShardBuilder<FullTextRecord> builder("single_db_test", 10);

	builder.truncate();
	builder.truncate_cache_files();

	{
		FullTextRecord record = {
			.m_value = 1111ull,
			.m_score = 0.1f,
			.m_domain_hash = 2222ull
		};

		builder.add(123456ull, record);
	}
	{
		FullTextRecord record = {
			.m_value = 1112ull,
			.m_score = 0.2f,
			.m_domain_hash = 2223ull
		};

		builder.add(123456ull + Config::shard_hash_table_size, record);
	}

	builder.append();
	builder.merge();

	FullTextShard<FullTextRecord> shard("single_db_test", 10);

	FullTextResultSet<FullTextRecord> result_set(Config::ft_max_results_per_section * Config::ft_max_sections);
	shard.find(123456ull, &result_set);

	BOOST_CHECK_EQUAL(result_set.size(), 1);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_value, 1111ull);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_score, 0.1f);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_domain_hash, 2222ull);

	result_set.close_sections();

	shard.find(123456ull + Config::shard_hash_table_size, &result_set);

	BOOST_CHECK_EQUAL(result_set.size(), 1);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_value, 1112ull);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_score, 0.2f);
	BOOST_CHECK_EQUAL(result_set.data_pointer()[0].m_domain_hash, 2223ull);

}

BOOST_AUTO_TEST_SUITE_END()
