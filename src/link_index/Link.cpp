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

#include "Link.h"

using namespace std;

Link::Link() {

}

Link::Link(const string &standard_link_data) {
		vector<string> col_values;
		boost::algorithm::split(col_values, standard_link_data, boost::is_any_of("\t"));

		m_source_url = URL(col_values[0], col_values[1]);
		m_target_url = URL(col_values[2], col_values[3]);
		m_link_text = col_values[4].substr(0, 1000);

		m_target_host_hash = m_target_url.host_hash();
		m_source_harmonic = 0;
		m_target_harmonic = 0;
}

Link::Link(const URL &source_url, const URL &target_url, float source_harmonic, float target_harmonic)
:
	m_source_url(source_url),
	m_target_url(target_url),
	m_target_host_hash(target_url.host_hash()),
	m_source_harmonic(source_harmonic),
	m_target_harmonic(target_harmonic)
{
}

Link::~Link() {

}

float Link::url_score() const {
	return max(m_source_harmonic - m_target_harmonic, m_source_harmonic / 100.0f);
}

float Link::domain_score() const {
	return max(m_source_harmonic - m_target_harmonic, m_source_harmonic / 100.0f)/100.0;
}

