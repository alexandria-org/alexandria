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

#include "robots.h"

BOOST_AUTO_TEST_SUITE(robot_parser)

BOOST_AUTO_TEST_CASE(parse) {
	std::string robots_content = "Sitemap: https://www.omnible.se/sitemap.xml\n"
		"User-agent: AlexandriaBot\n"
		"Disallow: *\n"
		"User-agent: *   # all agents\n"
		"Disallow: /*crawl=no*\n"
		"Disallow: /basket/add*\n"
	;
	std::string user_agent = "AlexandriaBot";
	googlebot::RobotsMatcher matcher;
	std::string url = "/visit";
	bool allowed = matcher.OneAgentAllowedByRobots(robots_content, user_agent, url);
	BOOST_CHECK(!allowed);
}

BOOST_AUTO_TEST_SUITE_END()
