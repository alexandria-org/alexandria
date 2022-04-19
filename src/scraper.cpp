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

#include <iostream>
#include <signal.h>
#include <set>
#include "fcgio.h"
#include "config.h"
#include "logger/logger.h"
#include "worker/worker.h"
#include "scraper/scraper.h"

using namespace std;

void custom_scraper() {

	set<string> files = {
		"1081037252118226853.gz",
		"10929784512354426297.gz",
		"11734959054377540990.gz",
		"1231587059077024966.gz",
		"12502184239462757041.gz",
		"12938836205580400636.gz",
		"13296278169331508461.gz",
		"14413462586171452382.gz",
		"15525439295995440529.gz",
		"16672519014390713150.gz",
		"18394430357962364895.gz",
		"10327881400750748691.gz",
		"10670281930934377105.gz",
		"10803309592637608156.gz",
		"1081037252118226853.gz", 
		"10834835858785818363.gz",
		"10929784512354426297.gz",
		"11126428663436160103.gz",
		"11147566439172409894.gz",
		"11190665490273023949.gz",
		"11494937404220367031.gz",
		"11734959054377540990.gz",
		"11828921816388240862.gz",
		"12060772154545358825.gz",
		"12162727308599252185.gz",
		"1231587059077024966.gz", 
		"12422730800151531594.gz",
		"12502184239462757041.gz",
		"12607232937660003080.gz",
		"12718743898666138934.gz",
		"12938836205580400636.gz",
		"13296278169331508461.gz",
		"13298202493829067141.gz",
		"13361744378846796689.gz",
		"13490885160851937523.gz",
		"13574739826384812082.gz",
		"13587802784601809709.gz",
		"13631835647153009173.gz",
		"1367770908792956967.gz", 
		"14046839555269968094.gz",
		"14413462586171452382.gz",
		"14541904792326560616.gz",
		"1482373106349460952.gz", 
		"14837337010216722341.gz",
		"15086873759162732674.gz",
		"15141235398943116798.gz",
		"15184607826907101421.gz",
		"15202491165257081552.gz",
		"15282359210281111669.gz",
		"15389582257311135463.gz",
		"15391345478373482283.gz",
		"15525439295995440529.gz",
		"15534406110118601925.gz",
		"15538335442391548855.gz",
		"15612477389751002303.gz",
		"15624474507591924007.gz",
		"15676254393982196237.gz",
		"15984927866124019398.gz",
		"16082148041043793761.gz",
		"16126091541072713257.gz",
		"16255682052513253306.gz",
		"16337701239641827376.gz",
		"16383716280375787103.gz",
		"16529912269361020733.gz",
		"16534544105461457700.gz",
		"16639969140692056885.gz",
		"16672519014390713150.gz",
		"16744732358440828846.gz",
		"16836166158893839160.gz",
		"17068835535637839797.gz",
		"1729061688188470388.gz", 
		"17360561405055540730.gz",
		"1746843565446970019.gz", 
		"17640709097762418065.gz",
		"18131842535353305093.gz",
		"18187211227753083566.gz",
		"18394430357962364895.gz",
		"1934117982241616211.gz", 
		"2211216046817783595.gz", 
		"2239809113491403275.gz", 
		"2327635888646701575.gz", 
		"2478041411438244752.gz", 
		"2551177065288807556.gz", 
		"2601237824066336189.gz", 
		"2646934360799240353.gz", 
		"2868212837076456812.gz", 
		"2926810779085983621.gz", 
		"3091319073926623211.gz", 
		"338937183383628192.gz",  
		"3604690558929123764.gz", 
		"3606044194188728481.gz", 
		"3852426225324652244.gz", 
		"3972328001646307399.gz", 
		"4007769859008228127.gz", 
		"4072548759689568430.gz", 
		"4193623627004305293.gz", 
		"4226856446620685890.gz", 
		"4312881270332666532.gz", 
		"4473520710685818343.gz", 
		"4720198542499220909.gz", 
		"4734886902380514989.gz", 
		"4800764859071121577.gz", 
		"4837392932044495189.gz", 
		"493001789945179170.gz",  
		"5263808122620003539.gz", 
		"5284265763220135234.gz", 
		"5322267948444699594.gz", 
		"5339170779334172446.gz", 
		"5496827761574196815.gz", 
		"5683557192991319856.gz", 
		"5772366474889297285.gz", 
		"5790856524309526271.gz", 
		"5853082621493931535.gz", 
		"5936310530969939988.gz", 
		"5958586233415593683.gz", 
		"5969382542874041237.gz", 
		"5969882935831645732.gz", 
		"6133590028181400561.gz", 
		"6168304203247739410.gz", 
		"619121932569169133.gz",  
		"6233832895907042056.gz", 
		"6371233587304885182.gz", 
		"6665598992901336677.gz", 
		"6747719063536596803.gz", 
		"6783121411632321193.gz", 
		"6878954272251422334.gz", 
		"6944679014837000907.gz", 
		"7204366432079867323.gz", 
		"7261759399318904627.gz", 
		"7279922463899918193.gz", 
		"7372161099870305017.gz", 
		"7483704574748382827.gz", 
		"7500975006697782336.gz", 
		"7577940383110528297.gz", 
		"7660839115654270407.gz", 
		"7690859939878490358.gz", 
		"7794216653216203685.gz", 
		"7969521158007747392.gz", 
		"7972503305086309118.gz", 
		"7977087069524267698.gz", 
		"801925665986995127.gz",  
		"8357461134896215565.gz", 
		"8473327975000475483.gz", 
		"8558287370764624669.gz", 
		"88637784417391575.gz",   
		"9219910288440466216.gz", 
		"9257832192261807811.gz", 
		"9300442310473380111.gz", 
		"9529889625719263624.gz", 
		"9668036200275969373.gz", 
		"990293958999783642.gz"
	};

	boost::filesystem::create_directories("output");

	for (string file : files) {

		ifstream infile("output/" + file);
		if (infile.is_open()) continue;

		stringstream ss;
		int error;
		transfer::gz_file_to_stream("crawl-data/ALEXANDRIA-TEST-SIZES/files/" + file, ss, error);

		if (error == transfer::OK) {
			string line;

			scraper::scraper_store store(false);
			map<string, unique_ptr<scraper::scraper>> scrapers;
			while (getline(ss, line)) {
				vector<string> cols;
				boost::algorithm::split(cols, line, boost::is_any_of("\t"));

				URL url(cols[0]);

				if (scrapers.count(url.host()) == 0) {
					scrapers[url.host()] = make_unique<scraper::scraper>(url.host(), &store);
					scrapers[url.host()]->set_timeout(0);
				}

				scrapers[url.host()]->push_url(url);
			}

			for (auto &_scraper : scrapers) {
				_scraper.second->run();
			}

			const string filename = "output/" + file;
			ofstream outfile(filename, ios::trunc | ios::binary);

			boost::iostreams::filtering_ostream compress_stream;
			compress_stream.push(boost::iostreams::gzip_compressor());
			compress_stream.push(outfile);

			for (const string row : store.get_results()) {
				compress_stream << row;
			}
		}
		return;
	}
/*
	scraper::scraper_store store(false);
	scraper::scraper _scraper("heroes.thelazy.net", &store);
	_scraper.set_timeout(0);
	_scraper.push_url(URL("https://heroes.thelazy.net//index.php/Main_Page"));
	_scraper.push_url(URL("https://heroes.thelazy.net//index.php/Dungeon"));
	_scraper.run();

	for (const string row : store.get_results()) {
		cout << row << endl;
	}*/
}

int main(int argc, const char **argv) {

	struct sigaction act{SIG_IGN};
	sigaction(SIGPIPE, &act, NULL);

	logger::start_logger_thread();

	if (getenv("ALEXANDRIA_CONFIG") != NULL) {
		config::read_config(getenv("ALEXANDRIA_CONFIG"));
	} else {
		config::read_config("/etc/alexandria.conf");
	}

	custom_scraper();

	/*worker::start_scraper_server();
	worker::wait_for_scraper_server();*/

	logger::join_logger_thread();

	return 0;
}

