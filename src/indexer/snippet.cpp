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

#include "snippet.h"

namespace indexer {

	snippet::snippet(const std::string &domain, const std::string &url, size_t snippet_index, const std::string &text)
	: m_text(text), m_domain(domain), m_url(url), m_snippet_index(snippet_index) {
		
	}

	snippet::~snippet() {
	}

	std::vector<size_t> snippet::tokens() const {
		std::vector<std::string> words = text::get_full_text_words(m_text);
		std::vector<size_t> tokens;
		for (const std::string &word : words) {
			tokens.push_back(Hash::str(word));
		}
		return tokens;
	}

	size_t snippet::domain_hash() const {
		return Hash::str(m_domain);
	}

	size_t snippet::url_hash() const {
		return m_url.hash();
	}

	size_t snippet::snippet_hash() const {
		size_t url_hash = this->url_hash();
		size_t snippet_hash = (url_hash << 10) + m_snippet_index;
		return snippet_hash;
	}

}
