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

#pragma once

#include <iostream>
#include <vector>

namespace full_text {
	class full_text_record;
	template<typename data_record> class full_text_index;
}

class HashTable;

namespace url_link {
	struct full_text_record;
}
namespace domain_link {
	struct full_text_record;
}
namespace search_allocation {
	struct allocation;
}

namespace api {

	using full_text::full_text_index;
	using full_text::full_text_record;

	void search(const std::string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		search_allocation::allocation *allocation, std::stringstream &response_stream);

	void search(const std::string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		const full_text_index<url_link::full_text_record> &link_index,
		search_allocation::allocation *allocation, std::stringstream &response_stream);

	void search(const std::string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		const full_text_index<url_link::full_text_record> &link_index, const full_text_index<domain_link::full_text_record> &domain_link_index,
		search_allocation::allocation *allocation, std::stringstream &response_stream);

	void search_all(const std::string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		search_allocation::allocation *allocation, std::stringstream &response_stream);

	void search_all(const std::string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		const full_text_index<url_link::full_text_record> &link_index,
		search_allocation::allocation *allocation, std::stringstream &response_stream);

	void search_all(const std::string &query, HashTable &hash_table, const full_text_index<full_text_record> &index,
		const full_text_index<url_link::full_text_record> &link_index, const full_text_index<domain_link::full_text_record> &domain_link_index,
		search_allocation::allocation *allocation, std::stringstream &response_stream);

	void word_stats(const std::string &query, const full_text_index<full_text_record> &index, const full_text_index<url_link::full_text_record> &link_index,
		size_t index_size, size_t link_index_size, std::stringstream &response_stream);

	void url(const std::string &url_str, HashTable &hash_table, std::stringstream &response_stream);

	void ids(const std::string &query, const full_text_index<full_text_record> &index, search_allocation::allocation *allocation,
		std::stringstream &response_stream);

	/*
	 * Make search on remote server but with links and url index on this server.
	 * */
	void search_remote(const std::string &query, HashTable &hash_table, const full_text_index<url_link::full_text_record> &link_index,
		const full_text_index<domain_link::full_text_record> &domain_link_index, search_allocation::allocation *allocation,
		std::stringstream &response_stream);

}
