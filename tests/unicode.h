
#include "parser/Unicode.h"

BOOST_AUTO_TEST_SUITE(unicode)

BOOST_AUTO_TEST_CASE(unicode) {
	BOOST_CHECK_EQUAL(Unicode::encode("hej jag heter josef"), "hej jag heter josef");
	BOOST_CHECK_EQUAL(Unicode::encode("hej jag heter josef och jag tillåter utf8 åäö chars$€"),
		"hej jag heter josef och jag tillåter utf8 åäö chars$€");
	BOOST_CHECK_EQUAL(Unicode::encode("是美国民主党政治家，于19世纪下半叶担"), "是美国民主党政治家，于19世纪下半叶担");

	BOOST_CHECK(Unicode::is_valid(Unicode::encode("L�gg i varukorg Om produkten Specifikation Anv�ndning Våra bönor är \
		rika på protein, mineraler och fibrer. Smaken är söt och konsistensen le")));
}

BOOST_AUTO_TEST_SUITE_END();
