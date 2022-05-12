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

#include "url_to_domain.h"
#include "logger/logger.h"
#include "indexer/merger.h"
#include "profiler/profiler.h"
#include "utils/thread_pool.hpp"
#include "URL.h"

using namespace std;

namespace full_text {

	url_to_domain::url_to_domain(const string &db_name)
	: m_db_name(db_name)
	{

	}

	url_to_domain::~url_to_domain() {
		indexer::merger::deregister_merger((size_t)this);
	}

	void url_to_domain::add_url(uint64_t url_hash, uint64_t domain_hash) {
		m_lock.lock();
		m_url_to_domain[url_hash] = domain_hash;
		m_domains[domain_hash]++;
		m_lock.unlock();
	}

	void url_to_domain::read() {
		lock_guard lock(m_lock);
		for (size_t bucket_id = 0; bucket_id < 8; bucket_id++) {
			const string file_name = "/mnt/"+(to_string(bucket_id))+"/full_text/url_to_domain_"+m_db_name+".fti";

			ifstream infile(file_name, ios::binary);
			if (infile.is_open()) {

				char buffer[8];

				do {

					infile.read(buffer, sizeof(uint64_t));
					if (infile.eof()) break;

					uint64_t url_hash = *((uint64_t *)buffer);

					infile.read(buffer, sizeof(uint64_t));
					uint64_t domain_hash = *((uint64_t *)buffer);

					m_url_to_domain[url_hash] = domain_hash;
					m_domains[domain_hash]++;

				} while (!infile.eof());

				infile.close();
			}
		}

		std::cout << "num_urls: " << m_url_to_domain.size() << std::endl;
	}

	void url_to_domain::convert() {

		/*algorithm::bloom_filter bf;
		bf.read_file("/mnt/0/url_filter.bloom");
		int fp = 0;
		for (size_t i = 0; i < 100000; i++) {
			if (bf.exists(to_string(rand()))) {
				fp++;
			}
		}

		cout << "fp: " << fp << "/" << 100000 << endl;
		return;
		*/

		algorithm::bloom_filter bf;
		bf.read_file("/mnt/0/url_filter_0.bloom");

		for (size_t i = 1; i < 24; i++) {
			algorithm::bloom_filter bf2;
			const string file_name = "/mnt/0/url_filter_"+to_string(i)+".bloom";
			cout << "reading_file: " << file_name << endl;
			bf2.read_file(file_name);

			bf.merge(bf2);
		}

		bf.write_file("/mnt/0/url_filter.bloom");

		int fp = 0;
		for (size_t i = 0; i < 100000; i++) {
			if (bf.exists(to_string(rand()))) {
				fp++;
			}
		}

		cout << "fp: " << fp << "/" << 100000 << endl;

		return;

		profiler::instance prof("run 1 million on each");
		utils::thread_pool pool(5);
		for (size_t i = 0; i < 24; i++) {
			pool.enqueue([i](){

				algorithm::bloom_filter bf;

				const string filename = "/root/urls/all-urls-" + to_string(i) + ".txt";
				ifstream infile(filename);
				string line;
				size_t idx = 0;
				while (getline(infile, line)) {
					URL url(line);
					bf.insert(url.hash_input());
					idx++;
					//if (idx > 10000000) break;
				}
				bf.commit();
				cout << "done with " << idx << ": " << filename << endl;

				bf.write_file("/mnt/0/url_filter_"+to_string(i)+".bloom");
			});
		}

		pool.run_all();
		prof.print();
	}

	void url_to_domain::write(size_t indexer_id) {\
		lock_guard lock(m_lock);
		const string file_name = "/mnt/"+(to_string(indexer_id % 8))+"/full_text/url_to_domain_"+m_db_name+".fti";

		ofstream outfile(file_name, ios::binary | ios::app);
		if (!outfile.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open url_to_domain file");
		}

		for (const auto &iter : m_url_to_domain) {
			outfile.write((const char *)&(iter.first), sizeof(uint64_t));
			outfile.write((const char *)&(iter.second), sizeof(uint64_t));
		}

		m_url_to_domain = std::unordered_map<uint64_t, uint64_t>{}; // Frees memory
		m_domains = std::unordered_map<uint64_t, size_t>{}; // Frees memory

		outfile.close();
	}

	void url_to_domain::truncate() {
		for (size_t i = 0; i < 8; i++) {
			const string file_name = "/mnt/"+(to_string(i))+"/full_text/url_to_domain_"+m_db_name+".fti";
			ofstream outfile(file_name, ios::trunc);
		}
	}

	set<uint64_t> url_to_domain::domain_set() const {
		set<uint64_t> domain_set;
		for (auto &iter : m_domains) {
			domain_set.insert(iter.first);
		}
		return domain_set;
	}

	set<uint64_t> url_to_domain::url_set() const {
		set<uint64_t> url_set;
		for (auto &iter : m_url_to_domain) {
			url_set.insert(iter.first);
		}
		return url_set;
	}
}
