
#pragma once

class Index {

public:

	Index(const string &path);
	Index(const string &location, const string &file);
	Index(const string &location, const string &word, const string &type);
	~Index();

	void download();
	void upload() const;

	vector<string> next_row();


private:

};


