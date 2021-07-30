

#include "test2.h"
#include "text/Text.h"
#include "common/common.h"
#include "parser/Unicode.h"
#include "system/Logger.h"

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

	ok = ok && Unicode::encode("hej jag heter josef") == "hej jag heter josef";
	ok = ok && Unicode::encode("hej jag heter josef och jag tillåter utf8 åäö chars$€") ==
		"hej jag heter josef och jag tillåter utf8 åäö chars$€";
	ok = ok && Unicode::encode("是美国民主党政治家，于19世纪下半叶担") == "是美国民主党政治家，于19世纪下半叶担";

	ok = ok && Unicode::is_valid(Unicode::encode("L�gg i varukorg Om produkten Specifikation Anv�ndning Våra bönor är \
		rika på protein, mineraler och fibrer. Smaken är söt och konsistensen le"));

	vector<string> words = Text::get_words_without_stopwords("Hej asd!asd jag, heter! !josef. cullhed 	\
		jfoidjfoai823hr9hfhwe9f8hshgohewogiqhoih");

	ok = ok && words.size() == 8;
	ok = ok && words[0] == "hej";
	ok = ok && words[1] == "asd";
	ok = ok && words[2] == "asd";
	ok = ok && words[3] == "jag";
	ok = ok && words[4] == "heter";
	ok = ok && words[5] == "josef";
	ok = ok && words[6] == "cullhed";
	ok = ok && words[7] == "jfoidjfoai823hr9hfhwe9f8hshgohewogiqhoih";

	return ok;
}

int test2_2(void) {
	int ok = 1;

	ok = ok && Text::clean_word("hej") == "hej";
	ok = ok && Text::clean_word("åäö") == "åäö";
	ok = ok && Text::clean_word("123") == "123";
	ok = ok && Text::clean_word("$Üç") == "";
	ok = ok && Text::clean_word("hejç") == "hej";
	ok = ok && Text::clean_word("açd") == "ad";

	ok = ok && Text::is_clean_word("hej");
	ok = ok && Text::is_clean_word("åäö");
	ok = ok && Text::is_clean_word("123");
	ok = ok && !Text::is_clean_word("$Üç");
	ok = ok && !Text::is_clean_word("hejç");
	ok = ok && !Text::is_clean_word("açd");

	ok = ok && Text::get_words_without_stopwords("hej")[0] == "hej";
	ok = ok && Text::get_words_without_stopwords("åäö")[0] == "åäö";
	ok = ok && Text::get_words_without_stopwords("123")[0] == "123";
	ok = ok && Text::get_words_without_stopwords("$Üç").size() == 0;
	ok = ok && Text::get_words_without_stopwords("hejç").size() == 0;
	ok = ok && Text::get_words_without_stopwords("açd").size() == 0;

	ok = ok && Text::get_words_without_stopwords("hej josef") == vector<string>({"hej", "josef"});
	ok = ok && Text::get_words_without_stopwords("hej, josef!") == vector<string>({"hej", "josef"});
	ok = ok && Text::get_words_without_stopwords("hej jÜsef cullhed du är bäst") ==
		vector<string>({"hej", "cullhed", "du", "bäst"});

	ok = ok && Text::get_words_without_stopwords("Låna! (Pengar till bilar)") ==
		vector<string>({"låna", "pengar", "bilar"});
	ok = ok && Text::get_words_without_stopwords("Dallas Swarner | Character | zKillboard", 3) ==
		vector<string>({"dallas", "swarner", "character"});
	ok = ok && Text::get_words_without_stopwords("Tapis Fleur des Champs Moutarde | Zen Dos", 3) ==
		vector<string>({"tapis", "fleur", "des"});
	ok = ok && Text::get_words_without_stopwords("Gina Osorno & The Dreamers", 3) ==
		vector<string>({"gina", "osorno", "dreamers"});

	ok = ok && Text::get_words_without_stopwords("IMG_2190 | Zhenyu (Tony) Tian") ==
		vector<string>({"zhenyu", "tony", "tian"});
	ok = ok && Text::get_words_without_stopwords("Tills alla dör - Diamant Salihu - Bok (9789189061842) | Bokus", 3) ==
		vector<string>({"tills", "dör", "diamant"});

	ok = ok && Text::get_words_without_stopwords("Messages postés par Prechan • Forum • Zeste de Savoir", 3) ==
		vector<string>({"messages", "par", "prechan"});
	ok = ok && Text::get_words_without_stopwords("Science SARU – 紙本分格") == vector<string>({"science", "saru"});
	ok = ok && Text::get_words_without_stopwords("Realiteti i trishtë shqiptar përmes fotove të gazetarit gjerman që komunizmi nuk i lejoi \
		të bëheshin publike | Gazeta Malesia", 3) ==
		vector<string>({"realiteti", "shqiptar", "fotove"});
	ok = ok && Text::get_words_without_stopwords("York County, VA") == vector<string>({"york", "county", "va"});
	ok = ok && Text::get_words_without_stopwords("HTML Sitemap 14 - zfreeti.com", 3) ==
		vector<string>({"html", "sitemap", "14"});
	ok = ok && Text::get_words_without_stopwords("HTML Sitemap 14 - zfreeti.com") ==
		vector<string>({"html", "sitemap", "14"});
	ok = ok && Text::get_words_without_stopwords("Archives.com zfreeti.com best. stream. in .the world") ==
		vector<string>({"best", "stream", "world"});

	return ok;
}

int test2_3(void) {
	int ok = 1;

	ok = ok && Text::get_expanded_full_text_words("hej jag heter josef") == vector<string>({"hej", "jag", "heter", "josef"});
	ok = ok && Text::get_expanded_full_text_words("hej jag heter.com josef") == vector<string>({"hej", "jag", "heter.com", "heter", "com", "josef"});
	ok = ok && Text::get_expanded_full_text_words("hej jag het-er.com josef") == vector<string>({"hej", "jag", "het-er.com", "het", "er", "com", "josef"});
	ok = ok && Text::get_expanded_full_text_words("test:test2-test3.test4") == vector<string>({"test:test2-test3.test4", "test", "test2", "test3", "test4"});

	return ok;
}
