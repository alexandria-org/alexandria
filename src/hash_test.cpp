
#include <iostream>
#include "system/SubSystem.h"
#include "parser/URL.h"
#include "full_text/FullTextRecord.h"
#include "full_text/FullTextShardBuilder.h"
#include "full_text/FullText.h"

int main() {

	hash<string> hasher;
	URL url("http://heroes.thelazy.net/index.php/List_of_heroes");

	FullTextIndex<FullTextRecord> fti("main_index_0");

	//fti.read_num_results("the", 3000000);

	return 0;
}
