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
#include "hash/Hash.h"
#include "transfer/Transfer.h"
#include "system/Profiler.h"
#include "system/Logger.h"
#include "file/File.h"
#include "json.hpp"
#include "KeyValueStore.h"
#include "parser/URL.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

#include "UrlData.h"

namespace UrlStore {

	using std::string;
	using std::vector;

	const int OK = 0;
	const int ERROR = 1;

	/*class DomainData {
		public:
			UrlData();
			UrlData(const UrlData &other);
			explicit UrlData(const std::string &str);
			UrlData(const char *cstr, size_t len);
			~UrlData();

			string domain;
			bool use_https;
			bool use_www;
			float harmonic_centrality;

			std::string to_str();
	};*/

	template <typename StoreData>
	class UrlStoreBatch {
		public:
			UrlStoreBatch();
			~UrlStoreBatch();

			void set(const StoreData &data);

			std::vector<leveldb::WriteBatch> m_batches;
	};

	template <typename StoreData>
	class UrlStore {
		public:
			UrlStore();
			explicit UrlStore(const std::string &path_prefix);
			~UrlStore();

			// Key value interface
			void set(const StoreData &data);
			StoreData get(const string &public_key);

			// Bulk inserts.
			void write_batch(UrlStoreBatch<StoreData> &batch);

			// Pending inserts.
			bool has_pending_insert();
			std::string next_pending_insert();
			void add_pending_insert(const std::string &file);

		private:
			std::vector<KeyValueStore *> m_shards;
			std::deque<std::string> m_pending_inserts;

	};

	template <typename StoreData>
	UrlStoreBatch<StoreData>::UrlStoreBatch()
	: m_batches(Config::url_store_shards)
	{
		
	}

	template <typename StoreData>
	UrlStoreBatch<StoreData>::~UrlStoreBatch() {

	}

	template <typename StoreData>
	void UrlStoreBatch<StoreData>::set(const StoreData &data) {
		const size_t shard = Hash::str(data.private_key()) % Config::url_store_shards;
		m_batches[shard].Put(data.private_key(), data.to_str());
	}

	template <typename StoreData>
	UrlStore<StoreData>::UrlStore() : UrlStore<StoreData>("/mnt") {
	}

	template <typename StoreData>
	UrlStore<StoreData>::UrlStore(const string &path_prefix)  {
		for (size_t i = 0; i < Config::url_store_shards; i++) {
			boost::filesystem::create_directories(path_prefix + "/" + std::to_string(i % 8) + "/url_store_" + std::to_string(i));
			m_shards.push_back(new KeyValueStore(path_prefix + "/" + std::to_string(i % 8) + "/url_store_" + std::to_string(i)));
		}
	}

	template <typename StoreData>
	UrlStore<StoreData>::~UrlStore() {
		for (KeyValueStore *shard : m_shards) {
			delete shard;
		}
	}

	template <typename StoreData>
	void UrlStore<StoreData>::set(const StoreData &data) {
		const size_t shard = Hash::str(data.private_key()) % Config::url_store_shards;
		m_shards[shard]->set(data.private_key(), data.to_str());
	}

	template <typename StoreData>
	StoreData UrlStore<StoreData>::get(const string &public_key) {
		const size_t shard = Hash::str(StoreData::public_key_to_private_key(public_key)) % Config::url_store_shards;
		string value = m_shards[shard]->get(StoreData::public_key_to_private_key(public_key));
		if (value.size()) return StoreData(value);
		return StoreData();
	}

	template <typename StoreData>
	void UrlStore<StoreData>::write_batch(UrlStoreBatch<StoreData> &batch) {
		for (size_t shard = 0; shard < Config::url_store_shards; shard++) {
			m_shards[shard]->db()->Write(leveldb::WriteOptions(), &batch.m_batches[shard]);
		}
	}

	template <typename StoreData>
	bool UrlStore<StoreData>::has_pending_insert() {
		return m_pending_inserts.size() > 0;
	}

	template <typename StoreData>
	string UrlStore<StoreData>::next_pending_insert() {
		string file = m_pending_inserts.front();
		m_pending_inserts.pop_front();
		return file;
	}

	template <typename StoreData>
	void UrlStore<StoreData>::add_pending_insert(const string &file) {
		m_pending_inserts.push_back(file);
	}

	template <typename StoreData>
	void print_binary_url_data(const StoreData &data, std::ostream &stream) {
		stream << data.to_str();
	}

	template <typename StoreData>
	void print_url_data(const StoreData &data, std::ostream &stream) {
		stream << data.to_json().dump(4);
	}

	template <typename StoreData>
	void print_url_data(const StoreData &data) {
		return print_url_data(data, std::cout);
	}

	template <typename StoreData>
	void handle_put_request(UrlStore<StoreData> &store, const std::string &write_data, std::stringstream &response_stream) {

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

			const string filename = Config::url_store_cache_path + "/" + std::to_string(micros) + "-" + std::to_string(rand()) + ".cache";
			std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
			outfile << write_data;

			outfile.close();

			store.add_pending_insert(filename);
		} else {
			UrlStoreBatch batch = get_write_data<StoreData>(store, write_data);
			write_insert_data<StoreData>(store, batch);
		}
	}

	template <typename StoreData>
	void handle_binary_get_request(UrlStore<StoreData> &store, const string &public_key, std::stringstream &response_stream) {
		StoreData data = store.get(public_key);
		print_binary_url_data(data, response_stream);
	}

	template <typename StoreData>
	void handle_get_request(UrlStore<StoreData> &store, const string &public_key, std::stringstream &response_stream) {
		StoreData data = store.get(public_key);
		print_url_data(data, response_stream);
	}

	template <typename StoreData>
	vector<string> post_data_to_keys(const string &post_data) {
		vector<string> keys;
		boost::algorithm::split(keys, post_data, boost::is_any_of("\n"));
		return keys;
	}

	template <typename StoreData>
	void handle_binary_post_request(UrlStore<StoreData> &store, const std::string &post_data, std::stringstream &response_stream) {
		vector<string> public_keys = post_data_to_keys<StoreData>(post_data);
		for (const string &public_key : public_keys) {
			StoreData data = store.get(public_key);
			const string bin_data = data.to_str();
			const size_t len = bin_data.size();
			response_stream.write((char *)&len, sizeof(size_t));
			response_stream.write(bin_data.c_str(), len);
		}
	}

	template <typename StoreData>
	void handle_post_request(UrlStore<StoreData> &store, const std::string &post_data, std::stringstream &response_stream) {
		vector<string> public_keys = post_data_to_keys<StoreData>(post_data);
		nlohmann::ordered_json arr;
		for (const string &public_key : public_keys) {
			StoreData data = store.get(public_key);
			nlohmann::ordered_json message = data.to_json();
			arr.push_back(message);
		}
		response_stream << arr.dump(4);
	}

	template <typename StoreData>
	void append_data_str(const StoreData &data, string &append_to) {
		string data_str = data.to_str();
		size_t data_len = data_str.size();
		append_to.append((char *)&data_len, sizeof(size_t));
		append_to.append(data_str);
	}

	template <typename StoreData>
	void append_bitmask(size_t bitmask, string &append_to) {
		append_to.append((char *)&bitmask, sizeof(size_t));
	}

	template <typename StoreData>
	void set(const StoreData &data) {
		update(data, 0x0);
	}

	template <typename StoreData>
	void set_many(const std::vector<StoreData> &datas) {
		update_many(datas, 0x0);
	}

	template <typename StoreData>
	void update(const StoreData &data, size_t update_bitmask) {
		string put_data;
		append_bitmask<StoreData>(0x0, put_data);
		append_bitmask<StoreData>(update_bitmask, put_data);
		append_data_str(data, put_data);
		Transfer::put(Config::url_store_host + "/store/url", put_data);
	}

	template <typename StoreData>
	void update_many(const std::vector<StoreData> &datas, size_t update_bitmask) {
		string put_data;
		append_bitmask<StoreData>(0x1, put_data);
		append_bitmask<StoreData>(update_bitmask, put_data);
		for (const StoreData &data : datas) {
			append_data_str(data, put_data);
		}
		Transfer::put(Config::url_store_host + "/store/url", put_data);
	}

	template <typename StoreData>
	int get(const string &url, StoreData &data) {
		Transfer::Response res = Transfer::get(Config::url_store_host + "/store/url/" + url, {"Accept: application/octet-stream"});
		if (res.code == 200) {
			data = StoreData(res.body);
			return OK;
		}
		return ERROR;
	}

	template <typename StoreData>
	int get_many(const std::vector<std::string> &public_keys, std::vector<StoreData> &datas) {
		const string post_data = boost::algorithm::join(public_keys, "\n");
		Transfer::Response res = Transfer::post(Config::url_store_host + "/store/url", post_data, {"Accept: application/octet-stream"});
		if (res.code == 200) {

			const size_t len = res.body.size();
			const char *cstr = res.body.c_str();
			size_t iter = 0;
			while (iter < len) {
				size_t data_len = *((size_t *)&cstr[iter]);
				iter += sizeof(size_t);

				if (data_len + iter > len) break;

				datas.emplace_back(StoreData(&cstr[iter], data_len));

				iter += data_len;
			}
			return OK;
		}
		return ERROR;
	}

	template <typename StoreData>
	UrlStoreBatch<StoreData> get_write_data(UrlStore<StoreData> &store, const string &write_data) {
		Profiler::instance prof1("parse and write to batches");

		const char *cstr = write_data.c_str();
		const size_t len = write_data.size();
		//const size_t deferr_bitmask = *((size_t *)&cstr[0]);
		const size_t update_bitmask = *((size_t *)&cstr[sizeof(size_t)]);

		size_t iter = 2*sizeof(size_t);
		UrlStoreBatch<StoreData> batch;
		while (iter < len) {
			size_t data_len = *((size_t *)&cstr[iter]);
			iter += sizeof(size_t);

			if (data_len + iter > len) break;

			StoreData data(&cstr[iter], data_len);
			if (update_bitmask) {
				StoreData to_update = store.get(data.public_key());
				to_update.apply_update(data, update_bitmask);
				batch.set(to_update);
			} else {
				batch.set(data);
			}

			iter += data_len;
		}

		return batch;
	}

	template <typename StoreData>
	UrlStoreBatch<StoreData> read_insert_data(UrlStore<StoreData> &store, const string &filename) {

		std::ifstream infile(filename, std::ios::binary);
		std::stringstream buffer;
		buffer << infile.rdbuf();
		const string write_data = buffer.str();

		UrlStoreBatch batch = get_write_data<StoreData>(store, write_data);

		File::delete_file(filename);

		return batch;
	}

	template <typename StoreData>
	void write_insert_data(UrlStore<StoreData> &store, UrlStoreBatch<StoreData> &batch) {
		Profiler::instance prof2("leveldb Write");
		store.write_batch(batch);
	}

	template <typename StoreData>
	void run_inserter(UrlStore<StoreData> &store) {

		const size_t num_files_between_compactions = 20;

		vector<string> filenames;

		while (store.has_pending_insert() && filenames.size() < num_files_between_compactions) {
			const string filename = store.next_pending_insert();
			filenames.push_back(filename);
		}

		vector<std::future<UrlStoreBatch<StoreData>>> futures;
		for (const string &filename : filenames) {
			std::future<UrlStoreBatch<StoreData>> fut = async(read_insert_data<StoreData>, std::ref(store), filename);
			futures.emplace_back(move(fut));
		}

		for (auto &fut : futures) {
			UrlStoreBatch<StoreData> batch = fut.get();
			write_insert_data<StoreData>(store, batch);
		}
	}

	template <typename StoreData>
	void urlstore_inserter(UrlStore<StoreData> &store) {
		using namespace std::literals::chrono_literals;
		while (true) {
			run_inserter<StoreData>(store);
			std::this_thread::sleep_for(100ms);
		}
	}

}
