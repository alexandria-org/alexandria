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

#include "parser/URL.h"

namespace Link {

	class Link {

	public:
		Link();
		explicit Link(const std::string &standard_link_data);
		Link(const URL &source_url, const URL &target_url, float source_harmonic, float target_harmonic);
		~Link();

		float url_score() const;
		float domain_score() const;

		const URL &source_url() const { return m_source_url; }
		const URL &target_url() const { return m_target_url; }
		const uint64_t &target_host_hash() const { return m_target_host_hash; }
		const float &source_harmonic() const { return m_source_harmonic; }
		const float &target_harmonic() const { return m_target_harmonic; }

	private:
		URL m_source_url;
		URL m_target_url;
		uint64_t m_target_host_hash;
		float m_source_harmonic;
		float m_target_harmonic;
		std::string m_link_text;
	};
}
