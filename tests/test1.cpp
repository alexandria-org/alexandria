

#include "test1.h"
#include "HtmlParser.h"

/*
 * Test HtmlParser
 */
int test1_1(void) {
	int ok = 1;

	HtmlParser parser;
	parser.parse("<title>test1</title>");
	ok = ok && parser.title() == "test1";

	parser.parse("<h1>test2</h1>");
	ok = ok && parser.h1() == "test2";

	parser.parse("he oisjdf osdjfo idjsofi djsof<h1></h1>");
	cout << "title: " << parser.title() << endl;
	cout << "h1: " << parser.h1() << endl;
	ok = ok && parser.title() == "";
	ok = ok && parser.h1() == "";

	parser.parse("<html><meta name=\"description\" content=\"Recensioner av Vår vid sommen och andra böcker.\"></html>");
	ok = ok && parser.meta() == "Recensioner av Vår vid sommen och andra böcker";

	parser.parse("<!DOCTYPE html>\
		<html lang=\"sv\" class=\"no-js\"  xmlns:book=\"http://ogp.me/ns/book#\" xmlns:og=\"http://opengraphprotocol.org/schema/\" xmlns:fb=\"http://www.facebook.com/2008/fbml\">\
		<head >\
		<meta http-equiv=\"X-UA-Compatible\" content=\"IE=Edge,chrome=1\">\
			<meta http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\">\
			<meta http-equiv=\"Content-Language\" content=\"sv\">\
			<meta name=\"viewport\" content=\"width=990\">		<title>Sammetsdiktaturen : motst&aring;nd och medl&ouml;pare i dagens Ryssland - Anna-Lena Laur&eacute;n - Bok (9789113108636) | Bokus</title>\
			<meta name=\"title\" content=\"Sammetsdiktaturen : motst&aring;nd och medl&ouml;pare i dagens Ryssland - Anna-Lena Laur&eacute;n - Bok (9789113108636) | Bokus\">\
			<meta name=\"description\" content=\"Pris: 199 kr. Inbunden, 2021. Finns i lager. K&ouml;p Sammetsdiktaturen : motst&aring;nd och medl&ouml;pare i dagens Ryssland av Anna-Lena Laur&eacute;n p&aring; Bokus.com. Boken har 3 st l&auml;sarrecensioner.\">\
			<meta property=\"fb:app_id\" content=\"126656811299\">\
			<meta property=\"og:site_name\" content=\"Bokus.com\">");
	ok = ok && parser.meta() == "Pris: 199 kr. Inbunden, 2021. Finns i lager. Köp Sammetsdiktaturen : motstånd och medlöpare i dagens Ryssland av Anna-Lena Laurén på Bokus.com. Boken har 3 st läsarrecensioner";

	parser.parse("<h1><span>Hej Hopp</span></h1>");
	cout << parser.h1() << endl;
	ok = ok && parser.h1() == "Hej Hopp";

	parser.parse("<html><h1>test2</h1> lite text efter</html>");
	cout << parser.text() << endl;
	ok = ok && parser.text() == "lite text efter";

	return ok;
}

