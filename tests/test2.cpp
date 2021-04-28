

#include "test2.h"
#include "BasicUrlData.h"
#include "common.h"

#include <vector>
#include <iostream>

using namespace std;

void var_dump(const vector<string> &vec) {
	int i = 0;
	cout << "vector[" << vec.size() << "]:" << endl;
	for (const string &str : vec) {
		cout << "vector[" << i << "] = \"" << str << '"' << endl;
		i++;
	}
}

/*
 * Test BasicUrlData
 */
int test2_1(void) {
	int ok = 1;

	BasicUrlData url_data;

	vector<string> words = url_data.get_words("Hej asd!asd jag, heter! !josef. cullhed 	\
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

	URL url;
	ok = ok && url.clean_word("hej") == "hej";
	ok = ok && url.clean_word("åäö") == "åäö";
	ok = ok && url.clean_word("123") == "123";
	ok = ok && url.clean_word("$Üç") == "";
	ok = ok && url.clean_word("hejç") == "hej";
	ok = ok && url.clean_word("açd") == "ad";

	ok = ok && url.is_clean_word("hej");
	ok = ok && url.is_clean_word("åäö");
	ok = ok && url.is_clean_word("123");
	ok = ok && !url.is_clean_word("$Üç");
	ok = ok && !url.is_clean_word("hejç");
	ok = ok && !url.is_clean_word("açd");

	ok = ok && url.get_words("hej")[0] == "hej";
	ok = ok && url.get_words("åäö")[0] == "åäö";
	ok = ok && url.get_words("123")[0] == "123";
	ok = ok && url.get_words("$Üç").size() == 0;
	ok = ok && url.get_words("hejç").size() == 0;
	ok = ok && url.get_words("açd").size() == 0;

	ok = ok && url.get_words("hej josef") == vector<string>({"hej", "josef"});
	ok = ok && url.get_words("hej, josef!") == vector<string>({"hej", "josef"});
	ok = ok && url.get_words("hej jÜsef cullhed du är bäst") == vector<string>({"hej", "cullhed", "du", "bäst"});

	ok = ok && url.get_words("Låna! (Pengar till bilar)") == vector<string>({"låna", "pengar", "bilar"});
	ok = ok && url.get_words("Dallas Swarner | Character | zKillboard", 3) == vector<string>({"dallas", "swarner", "character"});
	ok = ok && url.get_words("Tapis Fleur des Champs Moutarde | Zen Dos", 3) == vector<string>({"tapis", "fleur", "des"});
	ok = ok && url.get_words("Gina Osorno & The Dreamers", 3) == vector<string>({"gina", "osorno", "dreamers"});

	ok = ok && url.get_words("IMG_2190 | Zhenyu (Tony) Tian") == vector<string>({"zhenyu", "tony", "tian"});
	ok = ok && url.get_words("Tills alla dör - Diamant Salihu - Bok (9789189061842) | Bokus", 3) == vector<string>({"tills", "dör", "diamant"});

	ok = ok && url.get_words("Messages postés par Prechan • Forum • Zeste de Savoir", 3) ==
		vector<string>({"messages", "par", "prechan"});
	ok = ok && url.get_words("Science SARU – 紙本分格") == vector<string>({"science", "saru"});
	ok = ok && url.get_words("Realiteti i trishtë shqiptar përmes fotove të gazetarit gjerman që komunizmi nuk i lejoi \
		të bëheshin publike | Gazeta Malesia", 3) == vector<string>({"realiteti", "shqiptar", "fotove"});
	ok = ok && url.get_words("York County, VA") == vector<string>({"york", "county", "va"});
	ok = ok && url.get_words("HTML Sitemap 14 - zfreeti.com", 3) == vector<string>({"html", "sitemap", "14"});

	BasicUrlData url_data;

	ifstream infile("tests/data/CC-MAIN-20210225124124-20210225154124-00003.gz", ios_base::in | ios_base::binary);

	ok = ok && infile.is_open();

	url_data.read_stream(infile);
	url_data.build_index();

	return ok;
}
