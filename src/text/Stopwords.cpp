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

#include "stopwords.h"

using namespace std;

bool stopwords::is_stop_word(const string &word) {
	return (s_english.find(word) != s_english.end()) || (s_swedish.find(word) != s_swedish.end());
}

set<string> stopwords::s_english{
	"the",
	"of",
	"and",
	"in",
	"to",
	"a",
	"is",
	"as",
	"for",
	"was",
	"by",
	"that",
	"with",
	"on",
	"from",
	"are",
	"an",
	"or",
	"it",
	"at",
	"his",
	"be",
	"which",
	"this",
	"he",
	"were",
	"not",
	"also",
	"has",
	"have",
	"its",
	"their",
	"but",
	"first",
	"had",
	"one",
	"other",
	"new",
	"they",
	"such",
	"been",
	"can",
	"after",
	"more",
	"who",
	"two",
	"all",
	"some",
	"most",
	"may",
	"into",
	"when",
	"between",
	"than",
	"there",
	"these",
	"during",
	"only",
	"many",
	"time",
	"would",
	"states",
	"no",
	"over",
	"about",
	"while",
	"use",
	"both",
	"if",
	"where",
	"then",
	"i",
	"through",
	"since",
	"being",
	"made",
	"became",
	"part",
	"her",
	"de",
	"three",
	"any",
	"up",
	"each",
	"them",
	"often",
	"will",
	"him",
	"so",
	"out",
	"same",
	"because",
	"well",
	"several",
	"form",
	"name",
	"could",
	"although",
	"set",
	"different",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"0"
};

set<string> stopwords::s_swedish{
	"och",
	"i",
	"av",
	"som",
	"en",
	"att",
	"till",
	"den",
	"med",
	"på",
	"är",
	"för",
	"det",
	"de",
	"ett",
	"var",
	"från",
	"har",
	"om",
	"vid",
	"inte",
	"även",
	"eller",
	"sig",
	"men",
	"efter",
	"man",
	"kan",
	"sin",
	"där",
	"andra",
	"hade",
	"blev",
	"då",
	"första",
	"finns",
	"mot",
	"sedan",
	"så",
	"genom",
	"över",
	"detta",
	"också",
	"bland",
	"mellan",
	"två",
	"när",
	"fick",
	"samt",
	"skulle",
	"annat",
	"dock",
	"denna",
	"inom",
	"olika",
	"vilket",
	"ut",
	"flera",
	"se",
	"vara",
	"upp",
	"ha",
	"senare",
	"många",
	"kom",
	"än",
	"dessa",
	"alla",
	"samma",
	"del",
	"stora",
	"sitt",
	"sina",
	"mycket",
	"tre",
	"mer",
	"utan",
	"nya",
	"ofta",
	"enligt",
	"blir",
	"några",
	"kunde",
	"hela",
	"gjorde",
	"varit",
	"här",
	"ska",
	"eftersom",
	"få",
	"fanns",
	"bara",
	"något",
	"kommer",
	"både",
	"kallas",
	"vissa",
	"får",
	"cirka",
	"ur",
	"endast",
	"tog",
	"dem",
	"medan",
	"redan",
	"fyra",
	"någon",
	"nu",
	"går",
	"innan",
	"bli",
	"allt",
	"därefter",
	"därför",
	"hur",
	"varje",
	"per",
	"åt",
	"antal",
	"delen",
	"vilken",
	"vad",
	"helt",
	"sätt",
	"vill",
	"åren",
	"gör",
	"kallade",
	"främst",
	"båda",
	"själv",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"0"
};
