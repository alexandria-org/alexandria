
#pragma once

#include <map>
#include <unordered_map>
#include "file/TsvFile.h"

#include "DictionaryRow.h"

using namespace std;

class Dictionary {

public:

	Dictionary();
	Dictionary(TsvFile &tsv_file);
	~Dictionary();

	unordered_map<size_t, DictionaryRow>::const_iterator find(const string &key) const;

	unordered_map<size_t, DictionaryRow>::const_iterator begin() const;
	unordered_map<size_t, DictionaryRow>::const_iterator end() const;

	bool has_key(const string &key) const;

private:

	unordered_map<size_t, DictionaryRow> m_rows;

	void handle_collision(size_t key, const string &col);

};
