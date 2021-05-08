
#include "test1.h"
#include "test2.h"
#include "test3.h"

string read_test_file(const string &file_name) {
	ifstream file("tests/data/" + file_name);
	if (file.is_open()) {
		string ret;
		file.seekg(0, ios::end);
		ret.resize(file.tellg());
		file.seekg(0, ios::beg);
		file.read(&ret[0], ret.size());
		file.close();
		return ret;
	}
	return "";
}

int main(void) {

	cout << "Running static tests" << endl;

	int numSuites = 3;
	int numTestsInSuite [] = {
		3,
		2,
		3
	};

	int (* testSuite1 [])() = {
		test1_1,
		test1_2,
		test1_3,
	};

	int (* testSuite2 [])() = {
		test2_1,
		test2_2
	};

	int (* testSuite3 [])() = {
		test3_1,
		test3_2,
		test3_3
	};

	int (**testSuites[])() = {
		testSuite1,
		testSuite2,
		testSuite3,
	};

	int allOk = 1;
	for (int i = 0; i < numSuites; i++) {

		int numTests = numTestsInSuite[i];
		cout << "Running suite test" << (i + 1) << ".h" <<  endl;
		int suiteOk = 1;
		for (int j = 0; j < numTests; j++) {
			int ok = testSuites[i][j]();
			if (ok) {
				cout << "\ttest" << (i + 1) << "_" << (j + 1) << " passed" << endl;
			} else {
				cout << "\ttest" << (i + 1) << "_" << (j + 1) << " failed" << endl;
			}
			suiteOk = suiteOk && ok;
		}
		cout << endl;
		allOk = allOk && suiteOk;
	}

	if (allOk) {
		cout << "All tests passed" << endl;
	} else {
		cout << "ERROR Tests failed" << endl;
	}

	return 0;
}
