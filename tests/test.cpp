
#include "test1.h"
#include "test2.h"
#include "test3.h"
#include "test4.h"
#include "test5.h"
#include "test6.h"

#include "File.h"

string read_test_file(const string &file_name) {
	ifstream file("../tests/data/" + file_name);
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

/*
int main(int argc, const char **argv) {

	string run_tests = "all";
	size_t run_one = 0;
	if (argc > 1) {
		run_tests = "one";
		run_one = atoi(argv[1]);
	}

	int numSuites = 6;
	int numTestsInSuite [] = {
		3,
		3,
		4,
		4,
		3,
		5
	};

	int (* testSuite1 [])() = {
		test1_1,
		test1_2,
		test1_3,
	};

	int (* testSuite2 [])() = {
		test2_1,
		test2_2,
		test2_3
	};

	int (* testSuite3 [])() = {
		test3_1,
		test3_2,
		test3_3,
		test3_4
	};

	int (* testSuite4 [])() = {
		test4_1,
		test4_2,
		test4_3,
		test4_4
	};

	int (* testSuite5 [])() = {
		test5_1,
		test5_2,
		test5_3
	};

	int (* testSuite6 [])() = {
		test6_1,
		test6_2,
		test6_3,
		test6_4,
		test6_5
	};

	int (**testSuites[])() = {
		testSuite1,
		testSuite2,
		testSuite3,
		testSuite4,
		testSuite5,
		testSuite6
	};

	int allOk = 1;
	string test_report = "Alexandria.org test report\n";
	for (int i = 0; i < numSuites; i++) {

		if (run_tests == "one") {
			if (i != run_one - 1) continue;
		}

		int numTests = numTestsInSuite[i];
		test_report += "Running suite test" + to_string(i + 1) + ".h\n";
		int suiteOk = 1;
		for (int j = 0; j < numTests; j++) {
			int ok = testSuites[i][j]();
			if (ok) {
				test_report += "\ttest" + to_string(i + 1) + "_" + to_string(j + 1) + " passed\n";
			} else {
				test_report += "\ttest" + to_string(i + 1) + "_" + to_string(j + 1) + " failed\n";
			}
			suiteOk = suiteOk && ok;
		}
		test_report += "\n";
		allOk = allOk && suiteOk;
	}

	if (allOk) {
		test_report += "All tests passed\n";
	} else {
		test_report += "ERROR Tests failed\n";
	}

	cout << test_report << endl;

	return 0;
}
*/
