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

#include "document.h"
#include "algorithm/hash.h"
#include "text/text.h"
#include "URL.h"

using namespace std;

namespace cluster {

	document::document() 
	: m_name("unnamed document"){
	}

	document::document(const string &name)
	: m_name(name) {

	}

	document::~document() {

	}

	void document::read_text(const string &text) {
		const vector<string> words = text::get_words(text, 0);

		for (const string &word : words) {
			m_counts[algorithm::hash(word)]++;
		}
	}

	void read_text_to_corpus(corpus &corp, const string &text) {
		const vector<string> words = text::get_words(text, 0);

		for (const string &word : words) {
			size_t key = algorithm::hash(word);
			corp.counts[key]++;
			if (corp.words.count(key) == 0) {
				corp.words[key] = word;
			}
		}
	}

	void read_corpus(corpus &corp, documents &documents, stringstream &tsv) {
		string line;
		while (getline(tsv, line)) {
			const size_t pos = line.find('\t');
			if (pos == string::npos) continue;

			URL url(line.substr(0, pos));
			const string doc_text = line.substr(pos);

			const size_t key = url.host_hash();

			if (!documents.count(key)) {
				documents.emplace(key, url.host());
			}
			documents[key].read_text(doc_text);
			if (key == algorithm::hash("annicaviklund.se")) {
				cout << doc_text << endl;
			}
			read_text_to_corpus(corp, doc_text);
		}
	}

	void print_document(corpus &corp, const document &document) {
		vector<pair<size_t, size_t>> keys;
		for (const auto &iter : document.m_counts) {
			keys.emplace_back(iter.first, iter.second);
		}

		sort(keys.begin(), keys.end(), [](const auto &a, const auto &b) {
			return a.second > b.second;
		});

		size_t len = keys.size();
		for (size_t i = 0; i < min(100ul, len); i++) {
			cout << corp.words[keys[i].first] << " = " << keys[i].second << endl;
		}
	}
}

