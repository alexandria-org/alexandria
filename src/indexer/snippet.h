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

#include "level.h"
#include "text/Text.h"
#include "parser/URL.h"

namespace indexer {

	template<typename data_record>
	class snippet {

	public:

		snippet(const std::string &domain, const std::string &url, const std::string &text);
		~snippet();

        std::vector<size_t> tokens() const;
        size_t key(level_type lvl) const;

	private:

		std::string m_text;
		std::string m_domain;
		URL m_url;
		
		
	};

	template<typename data_record>
	snippet<data_record>::snippet(const std::string &domain, const std::string &url, const std::string &text)
	: m_text(text), m_domain(domain), m_url(url) {
		
	}

	template<typename data_record>
	snippet<data_record>::~snippet() {
	}

    template<typename data_record>
	std::vector<size_t> snippet<data_record>::tokens() const {
		std::vector<std::string> words = Text::get_full_text_words(m_text);
		std::vector<size_t> tokens;
		for (const std::string &word : words) {
			tokens.push_back(Hash::str(word));
		}
		return tokens;
	}

	template<typename data_record>
	size_t snippet<data_record>::key(level_type lvl) const {
		if (lvl == level::domain) return Hash::str(m_domain);
		if (lvl == level::url) return m_url.hash();
		if (lvl == level::snippet) return m_url.hash();

		return 0;
	}

}
