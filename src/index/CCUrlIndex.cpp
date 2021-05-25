
#include "CCUrlIndex.h"

CCUrlIndex::CCUrlIndex(const SubSystem *sub_system) :
BasicIndexer(sub_system), m_url_data(sub_system)
{
}

void CCUrlIndex::download(const string &bucket, const string &file, int shard, int id) {

	string key = file;
	key.replace(key.find(".warc.gz"), 8, ".gz");

	BasicUrlData url_data(m_sub_system);
	url_data.download(bucket, key);
	url_data.build_index(CCUrlIndex::get_downloaded_file_name(shard, id));
}

void CCUrlIndex::sorter(const vector<string> &words) {

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
				stringstream ss(line);

				size_t pos = line.find("\t"); // pos = url
				pos = line.find("\t", pos + 1); // pos = score
				size_t pos_stop = line.find("\t", pos + 1); // pos = next

				size_t len = pos_stop - pos;
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

		ofstream output_file("/mnt/"+to_string(target_shard)+"/upload/index_" + word + ".tsv", ios::trunc);
		const size_t max_num_lines = 50000;
		size_t line_num = 0;
		for (size_t i : indices) {
			output_file << lines[i] << endl;
			line_num++;
			if (line_num >= max_num_lines) break;
		}
	}
}

void CCUrlIndex::upload(const string &cc_batch, const string &word, size_t retries) {
	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket("alexandria-index");
	string key = cc_batch + "/index_" + word + ".tsv.gz";
	request.SetKey(key);

	ifstream infile;
	for (int shard = 0; shard < 8; shard++) {
		infile.open("/mnt/"+to_string(shard)+"/upload/index_" + word + ".tsv");
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
			upload(cc_batch, word, retries - 1);
		}
	}
}
