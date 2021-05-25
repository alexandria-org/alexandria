
#pragma once

#include "common/common.h"
#include <string>

using namespace std;

class Link {

public:
	Link(const string &host, const string &path, const string &target_host, const string &target_path,
		const string &text);
	~Link();

	string host() const;
	string path() const;
	string target_host() const;
	string target_path() const;
	string text() const;

private:
	string m_host;
	string m_path;
	string m_target_host;
	string m_target_path;
	string m_text;
	string m_rel;

};
