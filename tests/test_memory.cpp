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
#include "memory/memory.h"
#include "memory/debugger.h"
#include "indexer/index_builder.h"
#include "indexer/basic_index_builder.h"
#include "indexer/url_level.h"
#include "indexer/domain_link_record.h"

BOOST_AUTO_TEST_SUITE(test_memory)

BOOST_AUTO_TEST_CASE(test_memory) {
	memory::update();

	BOOST_CHECK(memory::get_available_memory() > 0);
	BOOST_CHECK(memory::get_total_memory() > 0);

	const size_t used1 = memory::allocated_memory();

	const size_t memlen = 1000000;
	char *some_mem = new char[memlen];
	for (size_t i = 0; i < memlen; i++) {
		some_mem[i] = 1;
	}
	memory::update();

	const size_t used2 = memory::allocated_memory();
	delete[] some_mem;
	const size_t used3 = memory::allocated_memory();

	std::cout << "used1: " << used1 << std::endl;
	std::cout << "used2: " << used2 << std::endl;
	std::cout << "used3: " << used3 << std::endl;
	BOOST_CHECK(used1 + 1000000 == used2);
	BOOST_CHECK(used1 == used3);
}

/*
 * Test memory consumtion during merge, should end with same amount.
 * */
BOOST_AUTO_TEST_CASE(test_indexer_memory) {
	memory::update();

	indexer::create_db_directories("domain_link_index");

	BOOST_CHECK(memory::get_available_memory() > 0);
	BOOST_CHECK(memory::get_total_memory() > 0);

	size_t memuse1, memuse2, memuse3, memuse4;
	memuse1 = memory::allocated_memory();

	{
		indexer::basic_index_builder<indexer::domain_link_record> idx("domain_link_index", 97ull);

		memuse2 = memory::allocated_memory();
		idx.append();
		idx.merge();
		memuse3 = memory::allocated_memory();
	}

	memuse4 = memory::allocated_memory();

	BOOST_CHECK(memuse1 == memuse4);
	BOOST_CHECK(memuse2 == memuse3);

}

BOOST_AUTO_TEST_SUITE_END()
