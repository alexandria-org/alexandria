
#include "BasicIndexer.h"

BasicIndexer::BasicIndexer(const SubSystem *sub_system)
: m_sub_system(sub_system)
{
}

void BasicIndexer::index(const vector<string> &file_names, int shard) {

	map<string, string> index;
	size_t bytes_read = 0;
	const size_t max_bytes_read = (1024ul*1024ul*1024ul*10ul); // 10 gb
	for (const string &file_name : file_names) {
		ifstream infile(file_name);
		string line;
		while (getline(infile, line)) {
			const string word = line.substr(0, line.find("\t"));
			index[word] += line + "\n";
			bytes_read += line.size() + 1;
		}
		infile.close();

		if (bytes_read > max_bytes_read) {
			// Flush.
			for (const auto &iter : index) {
				if (iter.second.size()) {
					ofstream outfile("/mnt/" + to_string(shard) + "/output/index_" + iter.first + ".tsv", ios::app);
					if (outfile.is_open()) {
						outfile << iter.second;
						outfile.close();
					}
				}
			}
			index.clear();
			bytes_read = 0;
		}
	}
	// Flush.
	for (const auto &iter : index) {
		if (iter.second.size()) {
			ofstream outfile("/mnt/" + to_string(shard) + "/output/index_" + iter.first + ".tsv", ios::app);
			if (outfile.is_open()) {
				outfile << iter.second;
				outfile.close();
			}
		}
	}

}

string BasicIndexer::get_downloaded_file_name(int shard, int id) {
	return "/mnt/"+to_string(shard)+"/input/downloaded_"+to_string(id)+".tsv";
}

void BasicIndexer::download_file(const string &bucket, const string &key, BasicData &data) {

	Aws::S3::Model::GetObjectRequest request;
	cout << "Downloading " << bucket << " key: " << key << endl;
	request.SetBucket(bucket);
	request.SetKey(key);

	auto outcome = m_sub_system->s3_client().GetObject(request);

	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();
		data.read_stream(stream);

	}

}
