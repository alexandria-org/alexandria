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
#include <memory>
#include <map>
#include <vector>
#include "snippet.h"
#include "index_builder.h"

namespace indexer {

	class snippet;
	enum class level_type { domain = 101, url = 102, snippet = 103};

	std::string level_to_str(level_type lvl);

	class level {
		public:
		virtual level_type get_type() const = 0;
		virtual void add_snippet(const snippet &s) = 0;
		virtual void merge() = 0;
	};

	struct domain_record {

		uint64_t m_value;
		float m_score;

	};

	class domain_level: public level {
		private:
		std::shared_ptr<index_builder<domain_record>> m_builder;
		public:
		domain_level();
		level_type get_type() const;
		void add_snippet(const snippet &s);
		void merge();
	};

	struct url_record {

		uint64_t m_value;
		float m_score;

	};

	class url_level: public level {
		private:
		std::map<size_t, std::shared_ptr<index_builder<url_record>>> m_builders;
		public:
		level_type get_type() const;
		void add_snippet(const snippet &s);
		void merge();
	};

	struct snippet_record {

		uint64_t m_value;
		float m_score;

	};

	class snippet_level: public level {
		private:
		std::map<size_t, std::shared_ptr<index_builder<snippet_record>>> m_builders;
		public:
		level_type get_type() const;
		void add_snippet(const snippet &s);
		void merge();
	};
}
