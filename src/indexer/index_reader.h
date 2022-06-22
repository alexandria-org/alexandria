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
#include <fstream>
#include <memory>

namespace indexer {

	/*
		This class provides an abstraction of data reading used by the index class.
		We provide an interface and two classes:
		index_reader_file
		and
		index_reader_ram
		to provide data directly from the file or from a preloaded sequence of bytes.
	*/

	class index_reader {

		public:

			virtual bool seek(size_t position) = 0;
			virtual void read(char *buffer, size_t length) = 0;
			virtual size_t size() = 0;
		
	};

	class index_reader_file : public index_reader {

		private:
			index_reader_file(const index_reader_file &);
			index_reader_file &operator=(const index_reader_file &);

		public:

			index_reader_file(const std::string &filename);
			index_reader_file(index_reader_file &&other);

			bool seek(size_t position);
			void read(char *buffer, size_t length);
			size_t size();
		
		private:
		
			std::unique_ptr<std::ifstream> m_reader;

	};

	class index_reader_ram : public index_reader {

		private:
			index_reader_ram(const index_reader_file &);
			index_reader_ram &operator=(const index_reader_file &);

		public:

			explicit index_reader_ram(const std::string &str);
			index_reader_ram(const char *buffer, size_t length);
			index_reader_ram(index_reader_ram &&other);

			bool seek(size_t position);
			void read(char *buffer, size_t length);
			size_t size() {return m_len; };
		
		private:
		
			const char *m_buffer;
			size_t m_len;
			size_t m_pos = 0;

	};


}
