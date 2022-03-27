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
#include "URL.h"

namespace parser {

	class html_link {

		public:
			html_link(const std::string &host, const std::string &path, const std::string &target_host, const std::string &target_path, bool nofollow,
				const std::string &text);
			html_link(const std::string &host, const std::string &path, const std::string &target_host, const std::string &target_path, bool nofollow);
			~html_link();

			URL source_url() const { return URL(m_host, m_path); };
			URL target_url() const { return URL(m_target_host, m_target_path); };
			std::string host() const { return m_host; };
			std::string path() const { return m_path; };
			std::string target_host() const { return m_target_host; };
			std::string target_path() const { return m_target_path; };
			bool nofollow() const { return m_nofollow; };
			std::string text() const {return m_text; };

		private:
			std::string m_host;
			std::string m_path;
			std::string m_target_host;
			std::string m_target_path;
			bool m_nofollow;
			std::string m_text;

	};

}
