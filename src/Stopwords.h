
#pragma once

#include <iostream>
#include <set>

using namespace std;

class Stopwords {

public:

	static bool is_stop_word(const string &word);

private:

	static set<string> s_english;
	static set<string> s_swedish;

};
