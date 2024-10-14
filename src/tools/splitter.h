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

namespace tools {

	const std::string s_suffix = "-small";
	const size_t s_num_threads = 12;

	std::vector<std::string> target_url_batches();
	std::vector<std::string> target_link_batches();

	std::vector<std::string> generate_list_with_url_files();
	std::vector<std::string> generate_list_with_link_files();
	std::vector<std::string> generate_list_with_target_url_files();
	std::vector<std::string> generate_list_with_target_link_files();

	void run_splitter();
	void run_split_urls_with_direct_links();
	void run_split_links_with_relevant_domains();
	void run_split_build_url_bloom();
	void run_split_direct_links();
	void run_split_build_direct_link_bloom();

}

