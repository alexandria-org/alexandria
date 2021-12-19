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
#include "parser/URL.h"
#include "full_text/FullTextRecord.h"

class ResultWithSnippet {

public:
	ResultWithSnippet(const std::string &tsv_data, const FullTextRecord &res);
	~ResultWithSnippet();

	const URL &url() const { return m_url; };
	const std::string &title() const { return m_title; };
	const std::string &snippet() const { return m_snippet; };
	const float &score() const { return m_score; };
	const uint64_t &domain_hash() const { return m_domain_hash; };

private:

	URL m_url;
	std::string m_title;
	std::string m_meta;
	std::string m_snippet;
	float m_score;
	uint64_t m_domain_hash;

	std::string make_snippet(const std::string &text) const;

};
