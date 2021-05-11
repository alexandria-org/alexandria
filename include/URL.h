
#pragma once

#include <iostream>
#include <boost/algorithm/string/join.hpp>
#include "common.h"

#include "TextBase.h"

using namespace std;

class URL : public TextBase {

public:
	URL();
	URL(const string &url);
	~URL();

	static string host_reverse(const string &host);

	void set_url_string(const string &url);
	string host() const;
	string path() const;
	string host_reverse() const;

	friend istream &operator >>(istream &ss, URL &url);
	friend ostream &operator <<(ostream& os, const URL& url);

private:

	string m_url_string;
	string m_host;
	string m_host_reverse;
	string m_path;
	int m_status;

	int parse();
	inline void remove_www(string &path);


};
