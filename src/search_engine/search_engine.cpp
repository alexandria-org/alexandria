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

#include "search_engine.h"
#include "text/text.h"
#include "sort/Sort.h"
#include "system/Profiler.h"
#include <cmath>

using namespace std;
using namespace full_text;

namespace search_engine {

	void reset_search_metric(struct search_metric &metric) {
		metric.m_total_found = 0;
		metric.m_total_url_links_found = 0;
		metric.m_total_domain_links_found = 0;
		metric.m_links_handled = 0;
		metric.m_link_domain_matches = 0;
		metric.m_link_url_matches = 0;
	}

	vector<full_text_record> search_deduplicate(search_allocation::storage<full_text_record> *storage,
		const full_text_index<full_text_record> &index, const vector<url_link::full_text_record> &links,
		const vector<domain_link::full_text_record> &domain_links, const string &query, size_t limit, struct search_metric &metric) {

		vector<full_text_record> complete_result = search_wrapper(storage, index, links, domain_links, query, Config::pre_result_limit, metric);

		vector<full_text_record> deduped_result = deduplicate_result_vector<full_text_record>(complete_result, limit);

		return deduped_result;
	}

}
