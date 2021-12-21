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

#include "LinkResult.h"
#include "text/Text.h"

using namespace std;

LinkResult::LinkResult(const string &tsv_data, const Link::FullTextRecord &res)
: m_score(res.m_score), m_link_hash(res.m_value) {
	size_t pos_start = 0;
	size_t pos_end = 0;

	pos_end = tsv_data.find(" links to ", pos_start);
	size_t len = pos_end - pos_start;
	m_source_url = URL(tsv_data.substr(pos_start, len));
	pos_start = pos_end + string(" links to ").size();

	pos_end = tsv_data.find(" with link text: ", pos_start);
	len = pos_end - pos_start;
	m_target_url = URL(tsv_data.substr(pos_start, len));
	pos_start = pos_end + string(" with link text: ").size();

	m_link_text = tsv_data.substr(pos_start);

}

LinkResult::~LinkResult() {

}

