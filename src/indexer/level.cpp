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

using namespace std;

namespace indexer {

	std::string level_to_str(level_type lvl) {
		if (lvl == level_type::domain) return "domain";
		if (lvl == level_type::url) return "url";
		if (lvl == level_type::snippet) return "snippet";
		return "unknown";
	}

	domain_level::domain_level() {
		m_builder = std::make_shared<index_builder<domain_record>>("domain", 0);
	}

	level_type domain_level::get_type() const {
		return level_type::domain;
	}

	void domain_level::add_snippet(const snippet &s) {
		for (size_t token : s.tokens()) {
			m_builder->add(token, domain_record{.m_value = s.domain_hash()});
		}
	}

	void domain_level::merge() {
		m_builder->append();
		m_builder->merge();
	}

	level_type url_level::get_type() const {
		return level_type::url;
	}

	void url_level::add_snippet(const snippet &s) {
		size_t dom_hash = s.domain_hash();
		if (m_builders.count(dom_hash) == 0) {
			m_builders[dom_hash] = std::make_shared<index_builder<url_record>>("url", dom_hash);
		}
		for (size_t token : s.tokens()) {
			m_builders[dom_hash]->add(token, url_record{.m_value = s.url_hash()});
		}
	}

	void url_level::merge() {
		for (auto &iter : m_builders) {
			iter.second->append();
			iter.second->merge();
		}
	}

	level_type snippet_level::get_type() const {
		return level_type::snippet;
	}

	void snippet_level::add_snippet(const snippet &s) {
		size_t url_hash = s.url_hash();
		if (m_builders.count(url_hash) == 0) {
			m_builders[url_hash] = std::make_shared<index_builder<snippet_record>>("snippet", url_hash);
		}
		for (size_t token : s.tokens()) {
			m_builders[url_hash]->add(token, snippet_record{.m_value = s.snippet_hash()});
		}
	}

	void snippet_level::merge() {
		for (auto &iter : m_builders) {
			iter.second->append();
			iter.second->merge();
		}
	}

}
