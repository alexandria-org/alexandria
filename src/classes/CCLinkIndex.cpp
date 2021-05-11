
#include "CCLinkIndex.h"

CCLinkIndex::CCLinkIndex(const SubSystem *sub_system) :
m_sub_system(sub_system), m_link_data(sub_system)
{
}

CCLinkIndex::~CCLinkIndex() {
}

string CCLinkIndex::download(const string &bucket, const string &file, int shard, int id) {

	string link_key = file;
	link_key.replace(link_key.find(".warc.gz"), 8, ".links.gz");

	download_file(bucket, link_key, m_link_data);
	//download_file(m_key, m_link_data);

	//m_link_data.build_index();

	return m_link_data.build_index(shard, id);
}

void CCLinkIndex::sorter(const vector<string> &words) {

	// Open all my output files.
	map<string, ofstream> out_files;
	int target_shard = rand() % 8;
	for (const string &word : words) {

		vector<string> lines;
		vector<size_t> indices;
		vector<double> scores;
		size_t index = 0;
		for (int shard = 0; shard < 8; shard++) {
			string file_name = "/mnt/"+to_string(shard)+"/output/index_" + word + ".tsv";
			ifstream input_file(file_name);

			if (!input_file.is_open()) {
				continue;
			}

			string line;
			while (getline(input_file, line)) {
				size_t pos = line.find("\t"); // pos = target domain
				pos = line.find("\t", pos + 1); // pos = target path
				pos = line.find("\t", pos + 1); // pos = from domain
				pos = line.find("\t", pos + 1); // pos = from path
				pos = line.find("\t", pos + 1); // pos = score
				const string col = line.substr(pos + 1, line.find("\t", pos + 1));
				double score = stod(col);
				scores.push_back(score);
				indices.push_back(index);
				lines.push_back(line);
				index++;
			}
			input_file.close();
		}

		sort(indices.begin(), indices.end(), [&](const size_t& a, const size_t& b) {
			return (scores[a] > scores[b]);
		});

		ofstream output_file("/mnt/"+to_string(target_shard)+"/upload/links_" + word + ".tsv", ios::trunc);
		const size_t max_num_lines = 500000;
		size_t line_num = 0;
		for (size_t i : indices) {
			output_file << lines[i] << endl;
			line_num++;
			if (line_num >= max_num_lines) break;
		}
	}

}

void CCLinkIndex::download_file(const string &bucket, const string &key, BasicData &data) {

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
