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

#include <map>
#include <unordered_map>
#include "file/TsvFile.h"

#include "DictionaryRow.h"

class Dictionary {

public:

	Dictionary();
	explicit Dictionary(TsvFile &tsv_file);
	~Dictionary();

	std::unordered_map<size_t, DictionaryRow>::const_iterator find(const std::string &key) const;

	std::unordered_map<size_t, DictionaryRow>::const_iterator begin() const;
	std::unordered_map<size_t, DictionaryRow>::const_iterator end() const;

	bool has_key(const std::string &key) const;
	size_t size() const { return m_rows.size(); }

private:

	std::unordered_map<size_t, DictionaryRow> m_rows;

	void handle_collision(size_t key, const std::string &col);

};
