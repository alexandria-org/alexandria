

#include "test2.h"
#include "CCIndex.h"

/*
 * Test CCIndex
 */
int test2_1(void) {
	int ok = 1;

	CCIndex index;

	vector<string> words = index.get_words("Hej asd!asd jag, heter! !josef. cullhed 	\
		jfoidjfoai823hr9hfhwe9f8hshgohewogiqhoih");

	ok = ok && words.size() == 7;
	ok = ok && words[0] == "hej";
	ok = ok && words[1] == "asd";
	ok = ok && words[2] == "asd";
	ok = ok && words[3] == "jag";
	ok = ok && words[4] == "heter";
	ok = ok && words[5] == "josef";
	ok = ok && words[6] == "cullhed";

	return ok;
}

int test2_2(void) {
	int ok = 1;

	CCIndex index;

	ifstream infile("tests/data/cc_index1.gz", ios_base::in | ios_base::binary);

	ok = ok && infile.is_open();

	index.read_stream(infile);
	index.build_index();


	return ok;
}
