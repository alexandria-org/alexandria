
#include "CCIndexMerger.h"

CCIndexMerger::CCIndexMerger(const string &cc_batch_source, const string &cc_batch_destination)
: m_cc_batch_source(cc_batch_source), m_cc_batch_destination(cc_batch_destination)
{
}

CCIndexMerger::~CCIndexMerger() {
}

void CCIndexMerger::run_all() {
	run_all(0);
}

void CCIndexMerger::run_all(size_t limit) {

	m_sub_system = new SubSystem();

	//run_merge_thread("låna");

	ThreadPool pool(CC_NUM_THREADS_MERGING);
	std::vector<std::future<void>> results;

	vector<string> words = m_sub_system->words();

	for (const string &word : words) {

		results.emplace_back(
			pool.enqueue([this, word] {
				run_merge_thread(word);
			})
		);

	}

	for(auto && result: results) {
		result.get();
	}

	delete m_sub_system;
}

void CCIndexMerger::run_merge_thread(const string &word) {
	
	string file1 = download_file(m_cc_batch_source + "/index_" + word + ".link.tsv.gz");
	string file2 = "";//download_file(m_cc_batch_destination + "/index_" + word + ".link.tsv.gz");
	file2 = merge_file(file1, file2, {1, 2}, 5, 200000);
	upload_file(m_cc_batch_destination + "/index_" + word + ".link.tsv.gz", file2, 3);

	string file3 = download_file(m_cc_batch_source + "/index_" + word + ".tsv.gz");
	string file4 = "";//download_file(m_cc_batch_destination + "/index_" + word + ".tsv.gz");
	file4 = merge_file(file3, file4, {1}, 2, 50000);
	upload_file(m_cc_batch_destination + "/index_" + word + ".tsv.gz", file4, 3);
}

string CCIndexMerger::download_file(const string &file_name) {
	Aws::S3::Model::GetObjectRequest request;
	const string bucket = "alexandria-index";
	cout << "Downloading " << bucket << " key: " << file_name << endl;
	request.SetBucket(bucket);
	request.SetKey(file_name);

	auto outcome = m_sub_system->s3_client().GetObject(request);

	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();
		filtering_istream decompress_stream;
		decompress_stream.push(gzip_decompressor());
		decompress_stream.push(stream);

		return string(istreambuf_iterator<char>(decompress_stream), {});
	}

	return "";
}

string CCIndexMerger::merge_file(const string &file1, const string &file2, vector<size_t> merge_by, size_t sort_by,
	size_t limit) {
	
	stringstream ss1(file1);
	stringstream ss2(file2);

	vector<string> lines1;
	vector<string> lines2;

	vector<size_t> merge1;
	vector<size_t> merge2;

	vector<size_t> sort1;
	vector<size_t> sort2;

	vector<size_t> permutation1;
	vector<size_t> permutation2;

	string line;
	size_t idx = 0;
	while (getline(ss1, line)) {
		permutation1.push_back(idx);
		lines1.push_back(line);
		vector<string> cols;
		boost::split(cols, line, boost::is_any_of("\t"));
		string merge = "";
		for (const size_t i : merge_by) {
			merge += cols[i];
		}
		merge1.push_back(m_hasher(merge));
		sort1.push_back(stoi(cols[sort_by]));
		idx++;
	}
	idx = 0;
	while (getline(ss2, line)) {
		permutation2.push_back(idx);
		lines2.push_back(line);
		vector<string> cols;
		boost::split(cols, line, boost::is_any_of("\t"));
		string merge = "";
		for (const size_t i : merge_by) {
			merge += cols[i];
		}
		merge2.push_back(m_hasher(merge));
		sort2.push_back(stoi(cols[sort_by]));
		idx++;
	}

	sort(permutation1.begin(), permutation1.end(), [&](const size_t &id_a, const size_t &id_b) {
		return merge1[id_a] < merge1[id_b];
	});

	sort(permutation2.begin(), permutation2.end(), [&](const size_t &id_a, const size_t &id_b) {
		return merge2[id_a] < merge2[id_b];
	});

	vector<string> merged_lines;
	vector<size_t> merged_sorts;
	vector<size_t> merged_indices;
	size_t i = 0, j = 0, k = 0;
	size_t min_len = min(merge1.size(), merge2.size());
	idx = 0;
	for (i = 0, j = 0; i < min_len && j < min_len;) {
		if (merge1[permutation1[i]] <= merge2[permutation2[j]]) {
			merged_lines.push_back(lines1[permutation1[i]]);
			merged_sorts.push_back(sort1[permutation1[i]]);
			merged_indices.push_back(idx);
			if (merge1[permutation1[i]] == merge2[permutation2[j]]) j++;
			i++;
		} else {
			merged_lines.push_back(lines2[permutation2[j]]);
			merged_sorts.push_back(sort2[permutation2[j]]);
			merged_indices.push_back(idx);
			j++;
		}
		idx++;
	}
	// Exhaust
	for (; i < merge1.size(); i++) {
		merged_lines.push_back(lines1[permutation1[i]]);
		merged_sorts.push_back(sort1[permutation1[i]]);
		merged_indices.push_back(idx);
		idx++;
	}
	for (; j < merge2.size(); j++) {
		merged_lines.push_back(lines2[permutation2[j]]);
		merged_sorts.push_back(sort2[permutation2[j]]);
		merged_indices.push_back(idx);
		idx++;
	}

	// Sort lines by sorts.
	sort(merged_indices.begin(), merged_indices.end(), [&](const size_t id_a, const size_t id_b) {
		return merged_sorts[id_a] > merged_sorts[id_b];
	});

	vector<string> sorted_lines;
	idx = 0;
	for (size_t id : merged_indices) {
		sorted_lines.push_back(merged_lines[id]);
		if (idx >= limit) break;
		idx++;
	}

	return boost::algorithm::join(sorted_lines, "\n");
}

void CCIndexMerger::upload_file(const string &file_name, const string &data, int retries) {

	stringstream infile(data);

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket("alexandria-index");
	request.SetKey(file_name);

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
			cout << "Upload failed, retrying for word: " << file_name << endl;
			upload_file(file_name, data, retries - 1);
		}
	}

}
