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

#include "html_link.h"

using namespace std;

namespace parser {

	html_link::html_link(const string &host, const string &path, const string &target_host, const string &target_path, bool nofollow,
		const string &text) :
		m_host(host),
		m_path(path),
		m_target_host(target_host),
		m_target_path(target_path),
		m_nofollow(nofollow),
		m_text(text)
	{
		
	}

	html_link::html_link(const string &host, const string &path, const string &target_host, const string &target_path, bool nofollow) :
		m_host(host),
		m_path(path),
		m_target_host(target_host),
		m_target_path(target_path),
		m_nofollow(nofollow)
	{
		
	}

	html_link::~html_link() {}

}
