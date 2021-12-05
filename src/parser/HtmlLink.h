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

#include <string>
#include "parser/URL.h"

using namespace std;

class HtmlLink {

public:
	HtmlLink(const string &host, const string &path, const string &target_host, const string &target_path, bool nofollow,
		const string &text);
	HtmlLink(const string &host, const string &path, const string &target_host, const string &target_path, bool nofollow);
	~HtmlLink();

	URL source_url() const { return URL(m_host, m_path); };
	URL target_url() const { return URL(m_target_host, m_target_path); };
	string host() const { return m_host; };
	string path() const { return m_path; };
	string target_host() const { return m_target_host; };
	string target_path() const { return m_target_path; };
	bool nofollow() const { return m_nofollow; };
	string text() const {return m_text; };

private:
	string m_host;
	string m_path;
	string m_target_host;
	string m_target_path;
	bool m_nofollow;
	string m_text;

};
