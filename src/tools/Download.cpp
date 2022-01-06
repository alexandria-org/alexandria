/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Download.h"
#include "transfer/Transfer.h"
#include "file/TsvFileRemote.h"

using namespace std;

namespace Tools {

	vector<string> download_batch(const string &batch) {

		File::TsvFileRemote warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");
		vector<string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		vector<string> files_to_download;
		for (const string &str : warc_paths) {
			string warc_path = str;
			const size_t pos = warc_path.find(".warc.gz");
			if (pos != string::npos) {
				warc_path.replace(pos, 8, ".gz");
			}
			files_to_download.push_back(warc_path);
		}

		return Transfer::download_gz_files_to_disk(files_to_download);
	}

}

