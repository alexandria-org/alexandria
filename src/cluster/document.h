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
#include <unordered_map>
#include <cstdint>

namespace cluster {

	typedef struct corpus_s {
		std::unordered_map<size_t, std::string> words;
		std::unordered_map<size_t, size_t> counts;
	} corpus;

	class document {
		public:
			document();
			document(const std::string &name);
			~document();
			std::string name() const { return m_name; };
			size_t size() const { return m_counts.size(); };

			void read_text(const std::string &text);
			friend void print_document(corpus &corp, const document &document);

		private:

			std::string m_name;
			std::unordered_map<size_t, size_t> m_counts;

	};

	typedef document topic;
	typedef std::unordered_map<size_t, document> documents;

	void read_corpus(corpus &corp, documents &documents, std::stringstream &tsv);
	void print_document(corpus &corp, const document &document);
}
