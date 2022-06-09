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

namespace indexer {

	#pragma pack(4)
	class domain_link_record {
		public:
		uint64_t m_value;
		float m_score;
		uint64_t m_source_domain;
		uint64_t m_target_domain;

		domain_link_record() : m_value(0), m_score(0.0f) {};
		domain_link_record(uint64_t value) : m_value(value), m_score(0.0f) {};
		domain_link_record(uint64_t value, float score) : m_value(value), m_score(score) {};
		domain_link_record(uint64_t value, float score, uint64_t target_domain)
				: m_value(value), m_score(score), m_target_domain(target_domain) {};

		bool operator==(const domain_link_record &b) const {
			return m_value == b.m_value;
		}

		bool operator<(const domain_link_record &b) const {
			return m_value < b.m_value;
		}

		domain_link_record &operator+=(const domain_link_record &b) {
			return *this;
		}

		/*
		 * Will be applied to records before truncating. Top records will be kept.
		 * */
		struct truncate_order {
			inline bool operator() (const domain_link_record &a, const domain_link_record &b) {
				return a.m_score > b.m_score;
			}
		};

		/*
		 * Will be applied before storing on disk. This is the order the records will be returned in.
		 * */
		struct storage_order {
			inline bool operator() (const domain_link_record &a, const domain_link_record &b) {
				return a.m_target_domain < b.m_target_domain;
			}
		};

		bool storage_equal(const domain_link_record &a) const {
			return m_target_domain == a.m_target_domain;
		}

	};
	#pragma pack()
}
