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

#include "system.h"
#include <thread>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace common {

	bool is_dev() {
		if (getenv("ALEXANDRIA_LIVE") != NULL && std::stoi(getenv("ALEXANDRIA_LIVE")) > 0) {
			return false;
		}
		return true;
	}

	std::string domain_index_filename() {
		if (is_dev()) {
			return "/dev_files/domain_info.tsv";
		}
		return "/files/domain_info.tsv";
	}

	std::string dictionary_filename() {
		if (is_dev()) {
			return "/dev_files/dictionary.tsv";
		}
		return "/files/dictionary.tsv";
	}

	std::string uuid() {
		// Create a random UUID
		boost::uuids::uuid uuid = boost::uuids::random_generator()();
		// Convert UUID to string and return
		return boost::uuids::to_string(uuid);
	}

}
