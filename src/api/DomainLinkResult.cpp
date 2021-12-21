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

#include "DomainLinkResult.h"
#include "text/Text.h"
#include "domain_link/FullTextRecord.h"

using namespace std;

DomainLinkResult::DomainLinkResult(const string &tsv_data, const DomainLink::FullTextRecord &res)
: m_score(res.m_score), m_link_hash(res.m_value) {

	string source_host;
	string source_path;
	string target_host;
	string target_path;

	size_t pos_start = 0;
	size_t pos_end = 0;
	size_t col_num = 0;
	while (pos_end != string::npos) {
		pos_end = tsv_data.find('\t', pos_start);
		const size_t len = pos_end - pos_start;
		if (col_num == 0) {
			source_host = tsv_data.substr(pos_start, len);
		}
		if (col_num == 1) {
			source_path = tsv_data.substr(pos_start, len);
		}
		if (col_num == 2) {
			target_host = tsv_data.substr(pos_start, len);
		}
		if (col_num == 3) {
			target_path = tsv_data.substr(pos_start, len);
		}
		if (col_num == 4) {
			m_link_text = tsv_data.substr(pos_start, len);
		}

		pos_start = pos_end + 1;
		col_num++;
	}

	m_source_url = URL(source_host, source_path);
	m_target_url = URL(target_host, target_path);

}

DomainLinkResult::~DomainLinkResult() {

}

