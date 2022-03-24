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

#include "domain_stats.h"
#include <iostream>
#include "common/Dictionary.h"
#include "file/TsvFileRemote.h"
#include "logger/logger.h"
#include "system/System.h"

using namespace std;

namespace domain_stats {

	Dictionary domain_data;

	void download_domain_stats() {
		LOG_INFO("download domain_info.tsv");
		File::TsvFileRemote domain_info_tsv(System::domain_index_filename());
		LOG_INFO("parsing.....");
		domain_data.load_tsv(domain_info_tsv);
	}

	float harmonic_centrality(const URL &url) {
		return harmonic_centrality(url.host_reverse());
	}

	float harmonic_centrality(const std::string &reverse_host) {

		const auto iter = domain_data.find(reverse_host);

		float harmonic = 0.0f;
		if (iter != domain_data.end()) {
			const DictionaryRow row = iter->second;
			harmonic = row.get_float(1);
		}

		return harmonic;
	}
}
