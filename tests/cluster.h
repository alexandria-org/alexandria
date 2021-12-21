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
#include "cluster/Document.h"
#include "transfer/Transfer.h"
#include "hash/Hash.h"
#include "system/Logger.h"

BOOST_AUTO_TEST_SUITE(cluster)

BOOST_AUTO_TEST_CASE(cluster) {
	Logger::start_logger_thread();
	{
		int error;
		stringstream ss;
		Transfer::gz_file_to_stream("/test-data/10272145489625484395-1002.gz", ss, error);
		BOOST_CHECK(error == Transfer::OK);

		Cluster::Corpus corpus;
		Cluster::Documents documents;
		Cluster::read_corpus(corpus, documents, ss);

		const size_t key = Hash::str("aftonbladet.se");
		BOOST_CHECK(documents.count(key) == 1);
		BOOST_CHECK(documents[key].size() > 0);

		//Cluster::print_document(corpus, documents[key]);
	}
	Logger::join_logger_thread();
}

BOOST_AUTO_TEST_SUITE_END()
