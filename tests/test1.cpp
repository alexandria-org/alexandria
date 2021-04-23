

#include "test1.h"
#include "HtmlParser.h"
#include <chrono>

/*
 * Test HtmlParser
 */
int test1_1(void) {
	int ok = 1;

	HtmlParser parser;

	parser.parse("<title>test1</title>");
	ok = ok && parser.title() == "test1";

	parser.parse("<title>test1</title><h1>test2</h1>");
	ok = ok && parser.h1() == "test2";

	parser.parse("he oisjdf osdjfo idjsofi djsof<h1></h1>");
	ok = ok && parser.title() == "";
	ok = ok && parser.h1() == "";

	parser.parse("<html><title>test1</title><meta name=\"description\" content=\"Recensioner av Vår vid sommen och andra böcker.\"></html>");
	ok = ok && parser.meta() == "Recensioner av Vår vid sommen och andra böcker";

	parser.parse(read_test_file("test1.html"));
	ok = ok && parser.meta() == "Pris: 199 kr. Inbunden, 2021. Finns i lager. Köp Sammetsdiktaturen : motstånd och medlöpare i dagens Ryssland av Anna-Lena Laurén på Bokus.com. Boken har 3 st läsarrecensioner";

	parser.parse("<title>test1</title><h1><span>Hej Hopp</span></h1>");
	ok = ok && parser.h1() == "Hej Hopp";

	parser.parse("<html><title>test1</title><h1>test2</h1> lite text efter</html>");
	ok = ok && parser.text() == "lite text efter";

	return ok;
}

int test1_2(void) {
	int ok = 1;

	string html;
	vector<Link> links;

	string test2_html = read_test_file("test2.html");

	HtmlParser parser;
	parser.parse(test2_html);
	ok = ok && parser.title() == "Resebyrån Främmande Världar - L. D. Lapinski - inbunden (9789178937943) | Adlibris Bokhandel";
	ok = ok && parser.meta() == "inbunden, 2021. Köp boken Resebyrån Främmande Världar av L. D. Lapinski (ISBN 9789178937943) hos Adlibris. Fraktfritt över 229 kr Alltid bra priser och snabb leverans. | Adlibris";
	ok = ok && parser.h1() == "Resebyrån Främmande Världar - inbunden, Svenska, 2021";

	ok = ok && parser.text() == "";

	/*auto time_start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 40000; i++) {
		string copy = test2_html;
		parser.parse(copy);
	}
	auto time_elapsed = std::chrono::high_resolution_clock::now() - time_start;
	auto time_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_elapsed).count();

	ok = ok && time_milliseconds < 3000;
	cout << "Took " << time_milliseconds << "ms" << endl;

	string test3_html = read_test_file("test3.html");

	//parser.parse(test3_html, "https://www.bokus.com/bok/9789189061842/tills-alla-dor/");

	time_start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 40000; i++) {
		string copy = test3_html;
		parser.parse(copy, "https://www.bokus.com/bok/9789189061842/tills-alla-dor/");
	}
	time_elapsed = std::chrono::high_resolution_clock::now() - time_start;
	time_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_elapsed).count();
	cout << "Took " << time_milliseconds << "ms" << endl;
	*/

	string test4_html = read_test_file("test4.html");
	parser.parse(test4_html);
	ok = ok && parser.title() == "Corona – samlad information för privatpersoner | Skatteverket";
	ok = ok && parser.h1() == "Corona – information för privatpersoner";
	ok = ok && parser.meta() == "Här har vi samlat information för privatpersoner som påverkas av corona på olika sätt";

	string stackoverflow_html = read_test_file("stackoverflow.html");
	parser.parse(stackoverflow_html);
	ok = ok && parser.title() == "node.js - How to use Async and Await with AWS SDK Javascript - Stack Overflow";
	ok = ok && parser.h1() == "How to use Async and Await with AWS SDK Javascript";
	ok = ok && parser.meta() == "I am working with the AWS SDK using the KMS libary. I would like to use async and await instead of callbacks. import AWS, { KMS } from \"aws-sdk\"; this.kms = new AWS.KMS(); const key = await this";

	html = read_test_file("hallakonsument.html");
	parser.parse(html, "https://www.hallakonsument.se/konsumentratt-kopsatt/innan-du-tar-ett-lan/");
	ok = ok && parser.title() == "Innan du tar ett lån | Hallå konsument – Konsumentverket";
	ok = ok && parser.h1() == "Innan du tar ett lån";
	ok = ok && parser.meta() == "Om du har ett behov av att låna pengar är det viktigt att läsa på om vilken typ av lån som passar dig. Prata med flera banker, jämför villkoren och kostnaderna för olika lån";

	links = parser.links();
	bool found_link = false;
	for (const auto &link : links) {
		if (link.target_host() == "www.konsumenternas.se" &&
			link.target_path() == "/lan--betalningar/lan/sa-fungerar-ett-lan/forhandsinformation/" &&
			link.text() == "Läs mer om förhandsinformation på webbplatsen konsumenternas.se") {
			found_link = true;
		}
	}

	ok = ok && found_link;

	html = read_test_file("konsumenternas.html");
	parser.parse(html, "https://www.konsumenternas.se/lan--betalningar/lan/");
	ok = ok && parser.title() == "Lån";
	ok = ok && parser.h1() == "Lån";
	ok = ok && parser.meta() == "Att låna pengar kan vara ett sätt att finansiera något som du behöver eller gärna vill köpa, men inte har råd att betala direkt. Men ett lån kostar pengar i form av avgifter och räntor";

	links = parser.links();
	found_link = false;
	for (const auto &link : links) {
		if (link.target_host() == "konsumenternas.us17.list-manage.com" &&
			link.target_path() == "/subscribe?u=a63ab96c95e9b06c9a857d5f9&id=132436ec8d" &&
			link.text() == "Nyhetsbrev") {
			found_link = true;
		}
	}
	ok = ok && found_link;

	html = read_test_file("sbab.html");
	parser.parse(html, "https://www.sbab.se/1/privat/lana/privatlan/privatlan_-_sa_funkar_det.html#/berakna_manadskostnad");
	ok = ok && parser.title() == "Privatlån - låna pengar till bra ränta - SBAB";
	ok = ok && parser.h1() == "Privatlån – låna pengar till bra ränta";
	ok = ok && parser.meta() == "Ansök om ett privatlån mellan 30 000 och 500 000 kronor. Låna pengar utan säkerhet. Ansök och få besked direkt";

	links = parser.links();
	found_link = false;
	for (const auto &link : links) {
		if (link.target_host() == "sbab.kundo.se" &&
			link.target_path() == "/org/sbab/" &&
			link.text() == "Kundforum") {
			found_link = true;
		}
	}
	ok = ok && found_link;

	html = read_test_file("kronofogden.html");
	parser.parse(html, "https://www.kronofogden.se/82374.html");
	ok = ok && parser.title() == "Fem tips om ekonomin förändras | Kronofogden";
	ok = ok && parser.h1() == "Fem tips om ekonomin förändras";
	ok = ok && parser.meta() == "";

	links = parser.links();
	found_link = false;
	for (const auto &link : links) {
		if (link.target_host() == "www.hallakonsument.se" &&
			link.target_path() == "/" &&
			link.text() == "Välkommen till Hallå konsument") {
			found_link = true;
		}
	}
	ok = ok && found_link;

	html = read_test_file("uppsala.html");
	parser.parse(html, "https://www.uppsala.se/stod-och-omsorg/privatekonomi-och-ekonomiskt-stod/boka-tid-for-budget--och-skuldradgivning/");
	ok = ok && parser.title() == "Budget- och skuldrådgivning hos Konsument Uppsala - Uppsala kommun";
	ok = ok && parser.h1() == "Budget- och skuldrådgivning hos Konsument Uppsala";
	ok = ok && parser.meta() == "Om du vill göra din egen hushållsbudget, vill ha ekonomisk rådgivning eller har skulder och inte får pengarna att räcka till kan du vända dig till Konsument Uppsala. ";

	links = parser.links();
	found_link = false;
	for (const auto &link : links) {
		if (link.target_host() == "outlook.office365.com" &&
			link.target_path() == "/owa/calendar/Budgetochskuldrdgivning@uppsalakommun1.onmicrosoft.com/bookings/" &&
			link.text() == "Boka tid online") {
			found_link = true;
		}
	}
	ok = ok && found_link;

	html = read_test_file("chessgames.com");
	parser.parse(html, "http://store.chessgames.com/chess-books/chess-notation-type/an---algebraic/author/s/alexander-cherniaev-anatoly-karpov-joe-gallagher-joel-r.-steed-miguel-a.-sanchez-richard-obrien/hardware-requirements/windows.html");
	ok = ok && parser.title() == "Chess Books : Windows, AN - Algebraic, Alexander Cherniaev, Anatoly Karpov, Joe Gallagher, Joel R. Steed, Miguel A. Sanchez and Richard O'Brien";
	ok = ok && parser.h1() == "Chess Books";
	ok = ok && parser.meta() == "Shop for Chess Books at US Chess Federation Sales. We offer the widest selection of Chess Books at the lowest prices with same-day shipping.Windows, AN - Algebraic, Alexander Cherniaev, Anatoly Karpov, Joe Gallagher, Joel R. Steed, Miguel A. Sanchez and Richard O'Brien";

	ok = ok && parser.links().size() == 0;

	// Test this one: https://www.printingbusinessdirectory.com/company/807554/print-science

	return ok;
}

int test1_3(void) {
	int ok = 1;

	HtmlParser parser;
	ok = ok && !parser.is_exotic_language("hej jag heter josef cullhed");
	ok = ok && !parser.is_exotic_language("åäö");
	ok = ok && !parser.is_exotic_language("Đảng,Đoàn thể - tnxp.hochiminhcity.gov.vn");
	ok = ok && !parser.is_exotic_language("Maktspelet i Volvo : en skildring inifr&aring;n - Hans Nyman - Kartonnage (9789189323056) | Bokus");

	ok = ok && parser.is_exotic_language("В КФУ проходят съемки короткометражного фильма в рамках проекта «Кино за 7 дней» | ВидеоПрокат+");
	ok = ok && parser.is_exotic_language("2015-09-09から1日間の記事一覧 - Nani-Sore　何それ？");
	ok = ok && parser.is_exotic_language("Ремонт Принтеров Hp в Спб Адреса | Ремонт принтеров");

	return ok;
}

