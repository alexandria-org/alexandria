
#include "CCLinkIndex.h"

CCLinkIndex::CCLinkIndex(const SubSystem *sub_system) :
BasicIndexer(sub_system), m_link_data(sub_system)
{
}

void CCLinkIndex::download(const string &bucket, const string &file, int shard, int id) {

	string link_key = file;
	link_key.replace(link_key.find(".warc.gz"), 8, ".links.gz");

	download_file(bucket, link_key, m_link_data);
	//download_file(m_key, m_link_data);

	//m_link_data.build_index();

	const string file_name = CCLinkIndex::get_downloaded_file_name(shard, id);
	m_link_data.build_index(file_name);
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
				size_t pos_stop = line.find("\t", pos + 1); // pos = next
				size_t len = pos - pos_stop;
				const string col = line.substr(pos, len);
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
		const size_t max_num_lines = 200000;
		size_t line_num = 0;
		for (size_t i : indices) {
			output_file << lines[i] << endl;
			line_num++;
			if (line_num >= max_num_lines) break;
		}
	}

}

void CCLinkIndex::upload(const string &word, size_t retries) {
	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket("alexandria-index");
	string key = "CC-MAIN-2021-10/index_" + word + ".link.tsv.gz";
	request.SetKey(key);

	ifstream infile;
	for (int shard = 0; shard < 8; shard++) {
		infile.open("/mnt/"+to_string(shard)+"/upload/links_" + word + ".tsv");
		if (infile.is_open()) {
			break;
		}
	}

	if (!infile.is_open()) {
		cout << "ERROR could not find output file for word: " << word << endl;
		return;
	}

	filtering_istream in;
	in.push(gzip_compressor());
	in.push(infile);

	std::shared_ptr<Aws::StringStream> request_body = Aws::MakeShared<Aws::StringStream>("");
	*request_body << in.rdbuf();
	request.SetBody(request_body);

	Aws::S3::Model::PutObjectOutcome outcome = m_sub_system->s3_client().PutObject(request);
	if (!outcome.IsSuccess()) {
		// Retry.
		if (retries > 0) {
			infile.close();
			cout << "Upload failed, retrying for word: " << word << endl;
			upload(word, retries - 1);
		}
	}
}