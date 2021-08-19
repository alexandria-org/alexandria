
#include <iostream>
#include "system/SubSystem.h"
#include "parser/URL.h"
#include "full_text/FullTextRecord.h"
#include "full_text/FullTextShardBuilder.h"
#include "full_text/FullText.h"

int main() {

	hash<string> hasher;
	URL url("http://url6.com/test");

	cout << url.hash() << endl;
	cout << url.host_hash() << endl;

	return 0;
}
