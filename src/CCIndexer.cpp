
#include "CCIndexer.h"

CCIndexer::CCIndexer(const SubSystem *sub_system) :
m_sub_system(sub_system), m_url_data(sub_system)
{
}

CCIndexer::~CCIndexer() {
}

void CCIndexer::run(const string &bucket, const string &file, int shard, int id) {

	string key = file;
	key.replace(key.find(".warc.gz"), 8, ".gz");

	string link_key = file;
	link_key.replace(link_key.find(".warc.gz"), 8, ".links.gz");

	download_file(bucket, key, m_url_data);
	//download_file(m_key, m_link_data);

	//m_link_data.build_index();

	m_url_data.build_index(shard, id);
}

void CCIndexer::index(const vector<string> &words, const vector<string> &input_files, int shard) {

	// Open all my output files.
	map<string, ofstream> out_files;
	for (const string &word : words) {
		string file_name = "/mnt/"+to_string(shard)+"/output/index_" + word + ".tsv";

		out_files[word].open(file_name, ios::trunc);

		if (!out_files[word].is_open()) {
			throw runtime_error("Could not open file: " + file_name + " error: " + strerror(errno));
		}
	}

	map<string, string> cache;

	const size_t max_cache_size = 10000000;
	size_t cache_size = 0;

	for (const string &input_file_name : input_files) {
		ifstream input_file(input_file_name);

		if (!input_file.is_open()) {
			throw runtime_error("Could not open file: " + input_file_name + " error: " + strerror(errno));
		}

		string line;
		while (getline(input_file, line)) {
			stringstream ss(line);
			string word;
			getline(ss, word, '\t');
			if (out_files.find(word) != out_files.end()) {
				cache[word] += line + '\n';
				cache_size++;
				//out_files[word] << line << endl;
			}
		}

		if (cache_size > max_cache_size) {
			// Flush cache.
			for (auto &iter : out_files) {
				iter.second << cache[iter.first];
			}
			cache.clear();
			cache_size = 0;
		}

		input_file.close();
	}

	// Flush cache.
	for (auto &iter : out_files) {
		iter.second << cache[iter.first];
	}
	cache.clear();
	cache_size = 0;

	for (auto &iter : out_files) {
		iter.second.close();
	}

}

void CCIndexer::sorter(const vector<string> &words, int shard) {

	// Open all my output files.
	map<string, ofstream> out_files;
	for (const string &word : words) {
		string file_name = "/mnt/"+to_string(shard)+"/output/index_" + word + ".tsv";
		ifstream input_file(file_name);

		if (!input_file.is_open()) {
			throw runtime_error("Could not open file: " + file_name + " error: " + strerror(errno));
		}

		string line;
		vector<string> lines;
		vector<size_t> indices;
		vector<double> scores;
		size_t index = 0;
		while (getline(input_file, line)) {
			stringstream ss(line);
			string col;
			getline(ss, col, '\t'); // word
			getline(ss, col, '\t'); // url
			getline(ss, col, '\t'); // score
			double score = stod(col);
			scores.push_back(score);
			indices.push_back(index);
			lines.push_back(line);
			index++;
		}
		input_file.close();

		sort(indices.begin(), indices.end(), [&](const size_t& a, const size_t& b) {
			return (scores[a] > scores[b]);
		});

		ofstream output_file(file_name, ios::trunc);
		const size_t max_num_lines = 50000;
		size_t line_num = 0;
		for (size_t i : indices) {
			output_file << lines[i] << endl;
			line_num++;
			if (line_num >= max_num_lines) break;
		}
	}

}

void CCIndexer::download_file(const string &bucket, const string &key, BasicData &data) {

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
