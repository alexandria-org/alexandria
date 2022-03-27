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

#include "sub_system.h"
#include "system.h"
#include "logger/logger.h"
#include "file/tsv_file_remote.h"

using namespace boost::iostreams;
using namespace std;

namespace common {

	sub_system::sub_system() {

		LOG_INFO("download domain_info.tsv");
		file::tsv_file_remote domain_index(domain_index_filename());
		m_domain_index = new dictionary(domain_index);

		LOG_INFO("download dictionary.tsv");
		file::tsv_file_remote dict_data(dictionary_filename());
		m_dictionary = new dictionary(dict_data);

		dict_data.read_column_into(0, m_words);

		sort(m_words.begin(), m_words.end(), [](const string &a, const string &b) {
			return a < b;
		});
	}

	sub_system::~sub_system() {
		delete m_dictionary;
		delete m_domain_index;
	}

	const dictionary *sub_system::domain_index() const {
		return m_domain_index;
	}

	const dictionary *sub_system::get_dictionary() const {
		return m_dictionary;
	}

	const vector<string> sub_system::words() const {
		return m_words;
	}

}
