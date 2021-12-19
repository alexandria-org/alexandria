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

#include "ResultWithSnippet.h"
#include "text/Text.h"

using namespace std;

ResultWithSnippet::ResultWithSnippet(const string &tsv_data, const FullTextRecord &res)
: m_score(res.m_score), m_domain_hash(res.m_domain_hash) {
	size_t pos_start = 0;
	size_t pos_end = 0;
	size_t col_num = 0;
	while (pos_end != string::npos) {
		pos_end = tsv_data.find('\t', pos_start);
		const size_t len = pos_end - pos_start;
		if (col_num == 0) {
			m_url = URL(tsv_data.substr(pos_start, len));
		}
		if (col_num == 1) {
			m_title = tsv_data.substr(pos_start, len);
		}
		if (col_num == 3) {
			m_meta = tsv_data.substr(pos_start, len);
		}
		if (col_num == 4) {
			m_snippet = make_snippet(tsv_data.substr(pos_start, len));
			if (m_snippet.size() == 0) {
				m_snippet = make_snippet(m_meta);
			}
		}

		pos_start = pos_end + 1;
		col_num++;
	}
}

ResultWithSnippet::~ResultWithSnippet() {

}

string ResultWithSnippet::make_snippet(const string &text) const {
	string response = text.substr(0, 140);
	Text::trim(response);
	if (response.size() >= 140) response += "...";
	return response;
}

