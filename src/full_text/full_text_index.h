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

namespace full_text {
	template<typename data_record> class full_text_index;
}

#include <iostream>
#include <vector>

#include "config.h"
#include "URL.h"
#include "common/sub_system.h"
#include "common/ThreadPool.h"
#include "full_text_record.h"
#include "full_text_shard.h"
#include "search_metric.h"
#include "text/text.h"

#include "logger/logger.h"

namespace full_text {

	template<typename data_record>
	class full_text_index {

	public:
		full_text_index(const std::string &name);
		~full_text_index();

		size_t disk_size() const;

		const std::vector<full_text_shard<data_record> *> &shards() const { return m_shards; };
		const std::vector<full_text_shard<data_record> *> *shard_ptr() const { return &m_shards; };

	private:

		std::string m_db_name;
		std::vector<full_text_shard<data_record> *> m_shards;

	};

	template<typename data_record>
	full_text_index<data_record>::full_text_index(const std::string &db_name)
	: m_db_name(db_name)
	{
		for (size_t shard_id = 0; shard_id < config::ft_num_shards; shard_id++) {
			m_shards.push_back(new full_text_shard<data_record>(m_db_name, shard_id));
		}
	}

	template<typename data_record>
	full_text_index<data_record>::~full_text_index() {
		for (full_text_shard<data_record> *shard : m_shards) {
			delete shard;
		}
	}

	template<typename data_record>
	size_t full_text_index<data_record>::disk_size() const {
		size_t size = 0;
		for (auto shard : m_shards) {
			size += shard->disk_size();
		}
		return size;
	}

}
