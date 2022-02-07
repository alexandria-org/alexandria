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
#include "json.hpp"

namespace UrlStore {

	const size_t update_has_https     = 0B00000001;
	const size_t update_has_www       = 0B00000010;

	class DomainData {
		public:
			DomainData();
			explicit DomainData(const std::string &str);
			DomainData(const char *cstr, size_t len);
			~DomainData();

			std::string m_domain;
			size_t m_has_https = 0;
			size_t m_has_www = 0;

			void apply_update(const DomainData &data, size_t update_bitmask);

			std::string to_str() const;
			std::string private_key() const;
			std::string public_key() const;
			nlohmann::ordered_json to_json() const;

			// Operators.
			friend std::ostream &operator<<(std::ostream &os, DomainData const &data);

			static std::string public_key_to_private_key(const std::string &public_key) {
				return public_key;
			}

			static const std::string uri;
	};

}
