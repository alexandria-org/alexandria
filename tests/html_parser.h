/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "parser/HtmlParser.h"
#include "file/File.h"

BOOST_AUTO_TEST_SUITE(html_parser)

BOOST_AUTO_TEST_CASE(html_parse1) {
	HtmlParser parser;

	parser.parse("<title>test1</title>");
	BOOST_CHECK_EQUAL(parser.title(), "test1");

	parser.parse("<title>test1</title><h1>test2</h1>");
	BOOST_CHECK_EQUAL(parser.h1(), "test2");

	parser.parse("he oisjdf osdjfo idjsofi djsof<h1></h1>");
	BOOST_CHECK_EQUAL(parser.title(), "");
	BOOST_CHECK_EQUAL(parser.h1(), "");

	parser.parse("<html><title>test1</title><meta name=\"description\" content=\"Recensioner av Vår vid sommen och andra böcker.\"></html>");
	BOOST_CHECK_EQUAL(parser.meta(), "Recensioner av Vår vid sommen och andra böcker");

	parser.parse(File::read_test_file("test1.html"));
	BOOST_CHECK_EQUAL(parser.meta(), "Pris: 199 kr. Inbunden, 2021. Finns i lager. Köp Sammetsdiktaturen : motstånd och medlöpare i dagens Ryssland av Anna-Lena Laurén på Bokus.com. Boken har 3 st läsarrecensioner");

	parser.parse("<title>test1</title><h1><span>Hej Hopp</span></h1>");
	BOOST_CHECK_EQUAL(parser.h1(), "Hej Hopp");

	parser.parse("<html><title>test1</title><h1>test2</h1> lite text efter</html>");
	BOOST_CHECK_EQUAL(parser.text(), "lite text efter");
}

BOOST_AUTO_TEST_CASE(html_parse_links) {

	string html;
	vector<HtmlLink> links;

	string test2_html = File::read_test_file("test2.html");

	HtmlParser parser;
	parser.parse(test2_html);
	BOOST_CHECK_EQUAL(parser.title(), "Resebyrån Främmande Världar - L. D. Lapinski - inbunden (9789178937943) | Adlibris Bokhandel");
	BOOST_CHECK_EQUAL(parser.meta(), "inbunden, 2021. Köp boken Resebyrån Främmande Världar av L. D. Lapinski (ISBN 9789178937943) hos Adlibris. Fraktfritt över 229 kr Alltid bra priser och snabb leverans. | Adlibris");
	BOOST_CHECK_EQUAL(parser.h1(), "Resebyrån Främmande Världar - inbunden, Svenska, 2021");

	BOOST_CHECK_EQUAL(parser.text(), "");
	BOOST_CHECK(parser.should_insert());

	string test4_html = File::read_test_file("test4.html");
	parser.parse(test4_html);
	BOOST_CHECK_EQUAL(parser.title(), "Corona – samlad information för privatpersoner | Skatteverket");
	BOOST_CHECK_EQUAL(parser.h1(), "Corona – information för privatpersoner");
	BOOST_CHECK_EQUAL(parser.meta(), "Här har vi samlat information för privatpersoner som påverkas av corona på olika sätt");
	BOOST_CHECK(parser.should_insert());

	string stackoverflow_html = File::read_test_file("stackoverflow.html");
	parser.parse(stackoverflow_html);
	BOOST_CHECK_EQUAL(parser.title(), "node.js - How to use Async and Await with AWS SDK Javascript - Stack Overflow");
	BOOST_CHECK_EQUAL(parser.h1(), "How to use Async and Await with AWS SDK Javascript");
	BOOST_CHECK_EQUAL(parser.meta(), "I am working with the AWS SDK using the KMS libary. I would like to use async and await instead of callbacks. import AWS, { KMS } from \"aws-sdk\"; this.kms = new AWS.KMS(); const key = await this");
	BOOST_CHECK(parser.should_insert());

	html = File::read_test_file("hallakonsument.html");
	parser.parse(html, "https://www.hallakonsument.se/konsumentratt-kopsatt/innan-du-tar-ett-lan/");
	BOOST_CHECK_EQUAL(parser.title(), "Innan du tar ett lån | Hallå konsument – Konsumentverket");
	BOOST_CHECK_EQUAL(parser.h1(), "Innan du tar ett lån");
	BOOST_CHECK_EQUAL(parser.meta(), "Om du har ett behov av att låna pengar är det viktigt att läsa på om vilken typ av lån som passar dig. Prata med flera banker, jämför villkoren och kostnaderna för olika lån");
	BOOST_CHECK(parser.should_insert());

	links = parser.links();
	bool found_link = false;
	for (const auto &link : links) {
		if (link.target_host() == "konsumenternas.se" &&
			link.target_path() == "/lan--betalningar/lan/sa-fungerar-ett-lan/forhandsinformation/" &&
			link.text() == "Läs mer om förhandsinformation på webbplatsen konsumenternas.se") {
			found_link = true;
		}
	}

	BOOST_CHECK(found_link);

	html = File::read_test_file("konsumenternas.html");
	parser.parse(html, "https://www.konsumenternas.se/lan--betalningar/lan/");
	BOOST_CHECK_EQUAL(parser.title(), "Lån");
	BOOST_CHECK_EQUAL(parser.h1(), "Lån");
	BOOST_CHECK_EQUAL(parser.meta(), "Att låna pengar kan vara ett sätt att finansiera något som du behöver eller gärna vill köpa, men inte har råd att betala direkt. Men ett lån kostar pengar i form av avgifter och räntor");
	BOOST_CHECK(parser.should_insert());

	links = parser.links();
	found_link = false;
	for (const auto &link : links) {
		if (link.target_host() == "konsumenternas.us17.list-manage.com" &&
			link.target_path() == "/subscribe?u=a63ab96c95e9b06c9a857d5f9&id=132436ec8d" &&
			link.text() == "Nyhetsbrev") {
			found_link = true;
		}
	}
	BOOST_CHECK(found_link);

	html = File::read_test_file("sbab.html");
	parser.parse(html, "https://www.sbab.se/1/privat/lana/privatlan/privatlan_-_sa_funkar_det.html#/berakna_manadskostnad");
	BOOST_CHECK_EQUAL(parser.title(), "Privatlån - låna pengar till bra ränta - SBAB");
	BOOST_CHECK_EQUAL(parser.h1(), "Privatlån – låna pengar till bra ränta");
	BOOST_CHECK_EQUAL(parser.meta(), "Ansök om ett privatlån mellan 30 000 och 500 000 kronor. Låna pengar utan säkerhet. Ansök och få besked direkt");
	BOOST_CHECK(parser.should_insert());

	links = parser.links();
	found_link = false;
	for (const auto &link : links) {
		if (link.target_host() == "sbab.kundo.se" &&
			link.target_path() == "/org/sbab/" &&
			link.text() == "Kundforum") {
			found_link = true;
		}
	}
	BOOST_CHECK(found_link);

	html = File::read_test_file("kronofogden.html");
	parser.parse(html, "https://www.kronofogden.se/82374.html");
	BOOST_CHECK_EQUAL(parser.title(), "Fem tips om ekonomin förändras | Kronofogden");
	BOOST_CHECK_EQUAL(parser.h1(), "Fem tips om ekonomin förändras");
	BOOST_CHECK_EQUAL(parser.meta(), "");
	BOOST_CHECK(parser.should_insert());

	links = parser.links();
	found_link = false;
	for (const auto &link : links) {
		if (link.target_host() == "hallakonsument.se" &&
			link.target_path() == "/" &&
			link.text() == "Välkommen till Hallå konsument") {
			found_link = true;
		}
	}
	BOOST_CHECK(found_link);

	html = File::read_test_file("uppsala.html");
	parser.parse(html, "https://www.uppsala.se/stod-och-omsorg/privatekonomi-och-ekonomiskt-stod/boka-tid-for-budget--och-skuldradgivning/");
	BOOST_CHECK_EQUAL(parser.title(), "Budget- och skuldrådgivning hos Konsument Uppsala - Uppsala kommun");
	BOOST_CHECK_EQUAL(parser.h1(), "Budget- och skuldrådgivning hos Konsument Uppsala");
	BOOST_CHECK_EQUAL(parser.meta(), "Om du vill göra din egen hushållsbudget, vill ha ekonomisk rådgivning eller har skulder och inte får pengarna att räcka till kan du vända dig till Konsument Uppsala. ");
	BOOST_CHECK(parser.should_insert());

	links = parser.links();
	found_link = false;
	for (const auto &link : links) {
		if (link.target_host() == "outlook.office365.com" &&
			link.target_path() == "/owa/calendar/Budgetochskuldrdgivning@uppsalakommun1.onmicrosoft.com/bookings/" &&
			link.text() == "Boka tid online") {
			found_link = true;
		}
	}
	BOOST_CHECK(found_link);

	html = File::read_test_file("chessgames.com");
	parser.parse(html, "http://store.chessgames.com/chess-books/chess-notation-type/an---algebraic/author/s/alexander-cherniaev-anatoly-karpov-joe-gallagher-joel-r.-steed-miguel-a.-sanchez-richard-obrien/hardware-requirements/windows.html");
	BOOST_CHECK_EQUAL(parser.title(), "Chess Books : Windows, AN - Algebraic, Alexander Cherniaev, Anatoly Karpov, Joe Gallagher, Joel R. Steed, Miguel A. Sanchez and Richard O'Brien");
	BOOST_CHECK_EQUAL(parser.h1(), "Chess Books");
	BOOST_CHECK_EQUAL(parser.meta(), "Shop for Chess Books at US Chess Federation Sales. We offer the widest selection of Chess Books at the lowest prices with same-day shipping.Windows, AN - Algebraic, Alexander Cherniaev, Anatoly Karpov, Joe Gallagher, Joel R. Steed, Miguel A. Sanchez and Richard O'Brien");

	BOOST_CHECK_EQUAL(parser.links().size(), 0);
	BOOST_CHECK(parser.should_insert());

	html = File::read_test_file("acomesf.org");
	parser.parse(html, "http://acomesf.org/download/42104960-3er-congreso-acomesf/");
	BOOST_CHECK_EQUAL(parser.title(), "42104960 3er Congreso ACOMESF | Asociación Colombiana de Médicos Especialistas en Salud Familiar (ACOMESF");
	BOOST_CHECK_EQUAL(parser.h1(), "42104960 3er Congreso ACOMESF");
	BOOST_CHECK_EQUAL(parser.meta(), "");
	BOOST_CHECK(parser.should_insert());

	html = File::read_test_file("automobileszone.com");
	parser.parse(html, "http://automobileszone.com/wp-login.php?redirect_to=http%3A%2F%2Fautomobileszone.com%2Fbest-bronco-build-off-our-editors-weigh-in-on-their-ideal-suvs%2F");
	BOOST_CHECK_EQUAL(parser.text(), "Username or Email Address Password Remember Me Lost your password? ← Back to Automobiles Zone Log in with WordPress.com");
	BOOST_CHECK(parser.should_insert());

	html = File::read_test_file("vcareprojectmanagement.com");
	parser.parse(html, "https://vcareprojectmanagement.com/products/project-manager-project-management-certification-pmi-atp-authorised-training-provider-pmp-capm-2021-online-training-course-class");
	BOOST_CHECK_EQUAL(parser.h1(), "");
	BOOST_CHECK_EQUAL(parser.text(), "");
}

BOOST_AUTO_TEST_CASE(html_parser_encodings) {

	HtmlParser parser;
	BOOST_CHECK(!parser.is_exotic_language("hej jag heter josef cullhed"));
	BOOST_CHECK(!parser.is_exotic_language("åäö"));
	BOOST_CHECK(!parser.is_exotic_language("Đảng,Đoàn thể - tnxp.hochiminhcity.gov.vn"));
	BOOST_CHECK(!parser.is_exotic_language("Maktspelet i Volvo : en skildring inifr&aring;n - Hans Nyman - Kartonnage (9789189323056) | Bokus"));

	BOOST_CHECK(parser.is_exotic_language("В КФУ проходят съемки короткометражного фильма в рамках проекта «Кино за 7 дней» | ВидеоПрокат+"));
	BOOST_CHECK(parser.is_exotic_language("2015-09-09から1日間の記事一覧 - Nani-Sore　何それ？"));
	BOOST_CHECK(parser.is_exotic_language("Ремонт Принтеров Hp в Спб Адреса | Ремонт принтеров"));
}

/*
	test these links: <a href="http://skatteverket.se/">Skatteverket</A>
	here: http://nomell.se/2009/03/24/prisa-gud-har-kommer-skatteaterbaringen/
*/

BOOST_AUTO_TEST_SUITE_END();
