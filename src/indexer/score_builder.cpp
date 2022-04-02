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

#include "score_builder.h"
#include <iostream>
#include <map>

namespace indexer {

	score_builder::score_builder(size_t num_documents, const std::map<uint64_t, size_t> *document_sizes)
	: m_num_documents(num_documents), m_document_sizes(document_sizes)
	{
		calculate_avg_document_size();
	}
		
	float score_builder::score() const {
		return 0.0f;
	}

	size_t score_builder::document_size(uint64_t doc_id) const {
		if (m_document_sizes->count(doc_id)) {
			return m_document_sizes->at(doc_id);
		}
		return 0;
	}

	void score_builder::calculate_avg_document_size() {
		m_avg_document_size = 0.0f;
		if (m_document_sizes->size()) {
			size_t sum = 0;
			for (const auto &iter : *m_document_sizes) {
				sum += iter.second;
			}
			m_avg_document_size = (float)sum / m_document_sizes->size();
		}
	}

}
