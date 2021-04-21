
#include <iostream>
#include <stdlib.h>

#include "test1.h"

using namespace std;

int main(void) {

	cout << "Running static tests" << endl;

	int numSuites = 1;
	int numTestsInSuite [] = {
		1
	};

	int (* testSuite1 [])() = {
		test1_1,
	};

	/*int (* testSuite2 [])() = {
		test2_1,
	};

	int (* testSuite3 [])() = {
		test3_1,
	};*/

	int (**testSuites[])() = {
		testSuite1,
		//testSuite2,
		//testSuite3,
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
