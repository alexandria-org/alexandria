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
#include "composite_index_builder.h"
#include "sharded_index_builder.h"
#include "sharded_index.h"
#include "index.h"
#include "generic_record.h"
#include "roaring/roaring.hh"

namespace indexer {

	class snippet;
	enum class level_type { domain = 101, url = 102, snippet = 103};

	std::string level_to_str(level_type lvl);

	/*
	This is the returned record from the index_tree. It contains more data than the stored record.
	*/
	class return_record : public generic_record {

		public:
		uint64_t m_url_hash;
		size_t m_num_url_links = 0;
		size_t m_num_domain_links = 0;

	};

	class link_record : public generic_record {
		public:
		uint64_t m_source_domain;
		uint64_t m_target_hash;

		link_record() : generic_record() {};
		link_record(uint64_t value) : generic_record(value) {};
		link_record(uint64_t value, float score) : generic_record(value, score) {};

		struct storage_order {
			inline bool operator() (const link_record &a, const link_record &b) {
				return a.m_target_hash < b.m_target_hash;
			}
		};

	};

	class domain_link_record : public generic_record {
		public:
		uint64_t m_source_domain;
		uint64_t m_target_domain;

		domain_link_record() : generic_record() {};
		domain_link_record(uint64_t value) : generic_record(value) {};
		domain_link_record(uint64_t value, float score) : generic_record(value, score) {};

		struct storage_order {
			inline bool operator() (const domain_link_record &a, const domain_link_record &b) {
				return a.m_target_domain < b.m_target_domain;
			}
		};

	};

	class level {
		public:
		virtual level_type get_type() const = 0;
		virtual void add_snippet(const snippet &s) = 0;
		virtual void add_document(size_t id, const std::string &doc) = 0;
		virtual void add_index_file(const std::string &local_path,
			std::function<void(uint64_t, const std::string &)> add_data,
			std::function<void(uint64_t, uint64_t)> add_url) = 0;
		virtual void merge() = 0;
		virtual void calculate_scores() = 0;
		virtual void clean_up() = 0;
		virtual std::vector<return_record> find(const std::string &query, const std::vector<size_t> &keys,
			const std::vector<link_record> &links, const std::vector<domain_link_record> &domain_links) = 0;

		protected:
		template<typename data_record>
		roaring::Roaring intersection(const std::vector<roaring::Roaring> &input) const;
		template<typename data_record>
		std::vector<return_record> intersection(const std::vector<std::vector<data_record>> &input) const;

		template<typename data_record>
		std::vector<return_record> summed_union(const std::vector<std::vector<data_record>> &input) const;

		template<typename data_record>
		void sort_and_get_top_results(std::vector<data_record> &input, size_t num_results) const;

		mutex m_lock;
	};

	class domain_record: public generic_record {

		public:
		domain_record() : generic_record() {};
		domain_record(uint64_t value) : generic_record(value) {};
		domain_record(uint64_t value, float score) : generic_record(value, score) {};

	};

	class domain_level: public level {
		private:
		std::unique_ptr<sharded_index_builder<domain_record>> m_builder;
		std::unique_ptr<sharded_index<domain_record>> m_search_index;
		public:
		domain_level();
		level_type get_type() const;
		void add_snippet(const snippet &s);
		void add_document(size_t id, const std::string &doc);
		void add_index_file(const std::string &local_path,
			std::function<void(uint64_t, const std::string &)> add_data,
			std::function<void(uint64_t, uint64_t)> add_url);
		void merge();
		void calculate_scores();
		void clean_up();
		std::vector<return_record> find(const std::string &query, const std::vector<size_t> &keys,
			const std::vector<link_record> &links, const std::vector<domain_link_record> &domain_links);
		size_t apply_domain_links(const std::vector<domain_link_record> &links, std::vector<return_record> &results);
	};

	class url_record : public generic_record {

		public:

		url_record() : generic_record() {};
		url_record(uint64_t value) : generic_record(value) {};
		url_record(uint64_t value, float score) : generic_record(value, score) {};

	};

	class url_level: public level {
		private:
		std::shared_ptr<composite_index_builder<url_record>> m_builder;
		public:
		url_level();
		level_type get_type() const;
		void add_snippet(const snippet &s);
		void add_document(size_t id, const std::string &doc);
		void add_index_file(const std::string &local_path,
			std::function<void(uint64_t, const std::string &)> add_data,
			std::function<void(uint64_t, uint64_t)> add_url);
		void merge();
		void calculate_scores() {};
		void clean_up();
		std::vector<return_record> find(const std::string &query, const std::vector<size_t> &keys,
			const std::vector<link_record> &links, const std::vector<domain_link_record> &domain_links);
		size_t apply_url_links(const std::vector<link_record> &links, std::vector<return_record> &results);
	};

	struct snippet_record : public generic_record {

		public:
		snippet_record() : generic_record() {};
		snippet_record(uint64_t value) : generic_record(value) {};
		snippet_record(uint64_t value, float score) : generic_record(value, score) {};

	};

	class snippet_level: public level {
		private:
		std::shared_ptr<composite_index_builder<snippet_record>> m_builder;
		public:
		snippet_level();
		level_type get_type() const;
		void add_snippet(const snippet &s);
		void add_document(size_t id, const std::string &doc);
		void add_index_file(const std::string &local_path,
			std::function<void(uint64_t, const std::string &)> add_data,
			std::function<void(uint64_t, uint64_t)> add_url);
		void merge();
		void calculate_scores() {};
		void clean_up();
		std::vector<return_record> find(const std::string &query, const std::vector<size_t> &keys,
			const std::vector<link_record> &links, const std::vector<domain_link_record> &domain_links);
	};
}
