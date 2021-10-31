
#include <iostream>
#include <vector>
#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;

void splitter(const string &warc_path) {
	ifstream infile(warc_path);
	boost::iostreams::filtering_istream decompress_stream;
	decompress_stream.push(boost::iostreams::gzip_decompressor());
	decompress_stream.push(infile);

	string line;
	while (getline(decompress_stream, line)) {
		const string url = line.substr(0, line.find("\t"));
	}
}

int main() {

	vector<string> batches = {
		"CC-MAIN-2021-31",
		"CC-MAIN-2021-25",
	};

	ThreadPool pool(12);

	for (const string &batch : batches) {

		const string file_name = string("/mnt/crawl-data/") + batch + "/warc.paths.gz";

		ifstream infile(file_name);

		boost::iostreams::filtering_istream decompress_stream;
		decompress_stream.push(boost::iostreams::gzip_decompressor());
		decompress_stream.push(infile);

		string line;
		while (getline(decompress_stream, line)) {
			string warc_path = string("/mnt/") + line;
			const size_t pos = warc_path.find(".warc.gz");
			if (pos != string::npos) {
				warc_path.replace(pos, 8, ".gz");
			}

			pool.enqueue([warc_path]() {
				splitter(warc_path);
			});
		}
	}

	return 0;
}
