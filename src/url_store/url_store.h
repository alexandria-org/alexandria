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
#include <vector>
#include <deque>
#include <future>
#include <thread>
#include <boost/filesystem.hpp>
#include "config.h"
#include "algorithm/hash.h"
#include "transfer/transfer.h"
#include "profiler/profiler.h"
#include "logger/logger.h"
#include "file/file.h"
#include "json.hpp"
#include "key_value_store.h"
#include "URL.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

#include "url_data.h"
#include "domain_data.h"
#include "robots_data.h"

namespace url_store {

	using std::string;
	using std::vector;

	const int OK = 0;
	const int ERROR = 1;

	template <typename store_data>
	class url_store_batch {
		public:
			url_store_batch();
			~url_store_batch();

			void set(const store_data &data);

			std::vector<leveldb::WriteBatch> m_batches;
	};

	template <typename store_data>
	class url_store {
		public:
			url_store();
			~url_store();

			// Key value interface
			void set(const store_data &data);
			store_data get(const string &public_key);

			// Bulk inserts.
			void write_batch(url_store_batch<store_data> &batch);

			// Pending inserts.
			bool has_pending_insert();
			std::string next_pending_insert();
			void add_pending_insert(const std::string &file);

		private:
			std::vector<key_value_store *> m_shards;
			std::deque<std::string> m_pending_inserts;

	};

	struct all_stores {
		url_store<url_data> url;
		url_store<domain_data> domain;
		url_store<robots_data> robots;
	};

	template <typename store_data>
	url_store_batch<store_data>::url_store_batch()
	: m_batches(config::url_store_shards)
	{
		
	}

	template <typename store_data>
	url_store_batch<store_data>::~url_store_batch() {

	}

	template <typename store_data>
	void url_store_batch<store_data>::set(const store_data &data) {
		const size_t shard = algorithm::hash(data.private_key()) % config::url_store_shards;
		m_batches[shard].Put(data.private_key(), data.to_str());
	}

	template <typename store_data>
	url_store<store_data>::url_store()  {
		const string &db_prefix = store_data::uri;
		for (size_t i = 0; i < config::url_store_shards; i++) {
			boost::filesystem::create_directories("/mnt/" + std::to_string(i % 8) + "/store/"+db_prefix+"/url_store_" + std::to_string(i));
			m_shards.push_back(new key_value_store("/mnt/" + std::to_string(i % 8) + "/store/"+db_prefix+"/url_store_" + std::to_string(i)));
		}
	}

	template <typename store_data>
	url_store<store_data>::~url_store() {
		for (key_value_store *shard : m_shards) {
			delete shard;
		}
	}

	template <typename store_data>
	void url_store<store_data>::set(const store_data &data) {
		const size_t shard = algorithm::hash(data.private_key()) % config::url_store_shards;
		m_shards[shard]->set(data.private_key(), data.to_str());
	}

	template <typename store_data>
	store_data url_store<store_data>::get(const string &public_key) {
		const size_t shard = algorithm::hash(store_data::public_key_to_private_key(public_key)) % config::url_store_shards;
		string value = m_shards[shard]->get(store_data::public_key_to_private_key(public_key));
		if (value.size()) return store_data(value);
		return store_data();
	}

	template <typename store_data>
	void url_store<store_data>::write_batch(url_store_batch<store_data> &batch) {
		for (size_t shard = 0; shard < config::url_store_shards; shard++) {
			m_shards[shard]->db()->Write(leveldb::WriteOptions(), &batch.m_batches[shard]);
		}
	}

	template <typename store_data>
	bool url_store<store_data>::has_pending_insert() {
		return m_pending_inserts.size() > 0;
	}

	template <typename store_data>
	string url_store<store_data>::next_pending_insert() {
		string file = m_pending_inserts.front();
		m_pending_inserts.pop_front();
		return file;
	}

	template <typename store_data>
	void url_store<store_data>::add_pending_insert(const string &file) {
		m_pending_inserts.push_back(file);
	}

	template <typename store_data>
	void print_binary_url_data(const store_data &data, std::ostream &stream) {
		stream << data.to_str();
	}

	template <typename store_data>
	void print_url_data(const store_data &data, std::ostream &stream) {
		stream << data.to_json().dump(4);
	}

	template <typename store_data>
	void print_url_data(const store_data &data) {
		return print_url_data(data, std::cout);
	}

	template <typename store_data>
	void handle_put_request(url_store<store_data> &store, const std::string &write_data, std::stringstream &response_stream) {

		(void)response_stream;

		const char *cstr = write_data.c_str();
		const size_t len = write_data.size();
		if (len < 2*sizeof(size_t)) return;
		const size_t deferr_bitmask = *((size_t *)&cstr[0]);

		if (deferr_bitmask) {
			using namespace std::chrono;

			// Only save put data to disk. Then a background job will consume the files.
			auto now = std::chrono::system_clock::now();
			auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
			auto epoch = now_ms.time_since_epoch();
			auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
			long micros = value.count();

			const string filename = config::url_store_cache_path + "/" + std::to_string(micros) + "-" + std::to_string(rand()) + ".cache";
			std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
			outfile << write_data;

			outfile.close();

			store.add_pending_insert(filename);
		} else {
			url_store_batch batch = get_write_data<store_data>(store, write_data);
			write_insert_data<store_data>(store, batch);
		}
	}

	template <typename store_data>
	void handle_binary_get_request(url_store<store_data> &store, const string &public_key, std::stringstream &response_stream) {
		store_data data = store.get(public_key);
		print_binary_url_data(data, response_stream);
	}

	template <typename store_data>
	void handle_get_request(url_store<store_data> &store, const string &public_key, std::stringstream &response_stream) {
		store_data data = store.get(public_key);
		print_url_data(data, response_stream);
	}

	template <typename store_data>
	vector<string> post_data_to_keys(const string &post_data) {
		vector<string> keys;
		boost::algorithm::split(keys, post_data, boost::is_any_of("\n"));
		return keys;
	}

	template <typename store_data>
	void handle_binary_post_request(url_store<store_data> &store, const std::string &post_data, std::stringstream &response_stream) {
		vector<string> public_keys = post_data_to_keys<store_data>(post_data);
		for (const string &public_key : public_keys) {
			store_data data = store.get(public_key);
			const string bin_data = data.to_str();
			const size_t len = bin_data.size();
			response_stream.write((char *)&len, sizeof(size_t));
			response_stream.write(bin_data.c_str(), len);
		}
	}

	template <typename store_data>
	void handle_post_request(url_store<store_data> &store, const std::string &post_data, std::stringstream &response_stream) {
		vector<string> public_keys = post_data_to_keys<store_data>(post_data);
		nlohmann::ordered_json arr;
		for (const string &public_key : public_keys) {
			store_data data = store.get(public_key);
			nlohmann::ordered_json message = data.to_json();
			arr.push_back(message);
		}
		response_stream << arr.dump(4);
	}

	template <typename store_data>
	void append_data_str(const store_data &data, string &append_to) {
		string data_str = data.to_str();
		size_t data_len = data_str.size();
		append_to.append((char *)&data_len, sizeof(size_t));
		append_to.append(data_str);
	}

	template <typename store_data>
	void append_bitmask(size_t bitmask, string &append_to) {
		append_to.append((char *)&bitmask, sizeof(size_t));
	}

	template <typename store_data>
	void set(const store_data &data) {
		update(data, 0x0);
	}

	template <typename store_data>
	void set_many(const std::vector<store_data> &datas) {
		update_many(datas, 0x0);
	}

	template <typename store_data>
	void update(const store_data &data, size_t update_bitmask) {
		string put_data;
		append_bitmask<store_data>(0x0, put_data);
		append_bitmask<store_data>(update_bitmask, put_data);
		append_data_str(data, put_data);
		transfer::put(config::url_store_host + "/store/" + store_data::uri, put_data);
	}

	template <typename store_data>
	void update_many(const std::vector<store_data> &datas, size_t update_bitmask) {
		string put_data;
		append_bitmask<store_data>(0x1, put_data);
		append_bitmask<store_data>(update_bitmask, put_data);
		for (const store_data &data : datas) {
			append_data_str(data, put_data);
		}
		transfer::put(config::url_store_host + "/store/" + store_data::uri, put_data);
	}

	template <typename store_data>
	int get(const string &url, store_data &data) {
		transfer::Response res = transfer::get(config::url_store_host + "/store/" + store_data::uri + "/" + url, {"Accept: application/octet-stream"});
		if (res.code == 200) {
			data = store_data(res.body);
			return OK;
		}
		return ERROR;
	}

	template <typename store_data>
	int get_many(const std::vector<std::string> &public_keys, std::vector<store_data> &datas) {
		const string post_data = boost::algorithm::join(public_keys, "\n");
		transfer::Response res = transfer::post(config::url_store_host + "/store/" + store_data::uri, post_data, {"Accept: application/octet-stream"});
		if (res.code == 200) {

			const size_t len = res.body.size();
			const char *cstr = res.body.c_str();
			size_t iter = 0;
			while (iter < len) {
				size_t data_len = *((size_t *)&cstr[iter]);
				iter += sizeof(size_t);

				if (data_len + iter > len) break;

				datas.emplace_back(store_data(&cstr[iter], data_len));

				iter += data_len;
			}
			return OK;
		}
		return ERROR;
	}

	template <typename store_data>
	url_store_batch<store_data> get_write_data(url_store<store_data> &store, const string &write_data) {
		profiler::instance prof1("parse and write to batches");

		const char *cstr = write_data.c_str();
		const size_t len = write_data.size();
		//const size_t deferr_bitmask = *((size_t *)&cstr[0]);
		const size_t update_bitmask = *((size_t *)&cstr[sizeof(size_t)]);

		size_t iter = 2*sizeof(size_t);
		url_store_batch<store_data> batch;
		while (iter < len) {
			size_t data_len = *((size_t *)&cstr[iter]);
			iter += sizeof(size_t);

			if (data_len + iter > len) break;

			store_data data(&cstr[iter], data_len);
			if (update_bitmask) {
				store_data to_update = store.get(data.public_key());
				to_update.apply_update(data, update_bitmask);
				batch.set(to_update);
			} else {
				batch.set(data);
			}

			iter += data_len;
		}

		return batch;
	}

	template <typename store_data>
	url_store_batch<store_data> read_insert_data(url_store<store_data> &store, const string &filename) {

		std::ifstream infile(filename, std::ios::binary);
		std::stringstream buffer;
		buffer << infile.rdbuf();
		const string write_data = buffer.str();

		url_store_batch batch = get_write_data<store_data>(store, write_data);

		file::delete_file(filename);

		return batch;
	}

	template <typename store_data>
	void write_insert_data(url_store<store_data> &store, url_store_batch<store_data> &batch) {
		profiler::instance prof2("leveldb Write");
		store.write_batch(batch);
	}

	template <typename store_data>
	void run_inserter(url_store<store_data> &store) {

		const size_t num_files_between_compactions = 20;

		vector<string> filenames;

		while (store.has_pending_insert() && filenames.size() < num_files_between_compactions) {
			const string filename = store.next_pending_insert();
			filenames.push_back(filename);
		}

		vector<std::future<url_store_batch<store_data>>> futures;
		for (const string &filename : filenames) {
			std::future<url_store_batch<store_data>> fut = async(read_insert_data<store_data>, std::ref(store), filename);
			futures.emplace_back(move(fut));
		}

		for (auto &fut : futures) {
			url_store_batch<store_data> batch = fut.get();
			write_insert_data<store_data>(store, batch);
		}
	}

	template <typename store_data>
	void urlstore_inserter(url_store<store_data> &store) {
		using namespace std::literals::chrono_literals;
		while (true) {
			run_inserter<store_data>(store);
			std::this_thread::sleep_for(100ms);
		}
	}

}
