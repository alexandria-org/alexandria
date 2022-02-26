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
#include "index.h"

namespace indexer {

	class snippet;
	enum class level_type { domain = 101, url = 102, snippet = 103};

	std::string level_to_str(level_type lvl);

	class generic_record {

		public:
		uint64_t m_value;
		float m_score;

		generic_record() : m_value(0), m_score(0.0f) {};
		generic_record(uint64_t value) : m_value(value), m_score(0.0f) {};
		generic_record(uint64_t value, float score) : m_value(value), m_score(score) {};

		bool operator==(const generic_record &b) const {
			return m_value == b.m_value;
		}

		bool operator<(const generic_record &b) const {
			return m_value < b.m_value;
		}

		generic_record operator+(const generic_record &b) const {
			generic_record sum;
			sum.m_value = m_value;
			sum.m_score = m_score + b.m_score;
			return sum;
		}

		generic_record &operator+=(const generic_record &b) {
			m_score += b.m_score;
			return *this;
		}

	};

	class level {
		public:
		virtual level_type get_type() const = 0;
		virtual void add_snippet(const snippet &s) = 0;
		virtual void add_document(size_t id, const std::string &doc) = 0;
		virtual void add_index_file(const std::string &local_path) = 0;
		virtual void merge() = 0;
		virtual std::vector<generic_record> find(const std::string &query, const std::vector<size_t> &keys) = 0;

		protected:
		template<typename data_record>
		std::vector<generic_record> intersection(const std::vector<std::vector<data_record>> &input) const;

		template<typename data_record>
		void sort_and_get_top_results(std::vector<data_record> &input, size_t num_results) const;
	};

	#pragma pack(4)
	class domain_record : public generic_record {

		public:
		float m_count = 1;

		domain_record operator+(const domain_record &b) const {
			domain_record sum;
			sum.m_value = m_value;
			sum.m_score = m_score + b.m_score;
			sum.m_count = m_count + b.m_count;
			return sum;
		}

	};

	class domain_level: public level {
		private:
		std::shared_ptr<index_builder<domain_record>> m_builder;
		public:
		domain_level();
		level_type get_type() const;
		void add_snippet(const snippet &s);
		void add_document(size_t id, const std::string &doc);
		void add_index_file(const std::string &local_path);
		void merge();
		std::vector<generic_record> find(const std::string &query, const std::vector<size_t> &keys);
	};

	class url_record : public generic_record {

	};

	class url_level: public level {
		private:
		std::map<size_t, std::shared_ptr<index_builder<url_record>>> m_builders;
		public:
		level_type get_type() const;
		void add_snippet(const snippet &s);
		void add_document(size_t id, const std::string &doc);
		void add_index_file(const std::string &local_path);
		void merge();
		std::vector<generic_record> find(const std::string &query, const std::vector<size_t> &keys);
	};

	struct snippet_record : public generic_record {

	};

	class snippet_level: public level {
		private:
		std::map<size_t, std::shared_ptr<index_builder<snippet_record>>> m_builders;
		public:
		level_type get_type() const;
		void add_snippet(const snippet &s);
		void add_document(size_t id, const std::string &doc);
		void add_index_file(const std::string &local_path);
		void merge();
		std::vector<generic_record> find(const std::string &query, const std::vector<size_t> &keys);
	};
}
