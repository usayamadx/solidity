/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <test/tools/ossfuzz/SolidityMutator.h>

#include <libsolutil/Whiskers.h>

#include <boost/algorithm/string/join.hpp>

#include <algorithm>

using namespace std;
using namespace solidity::util;
using namespace solidity::test::fuzzer;

string SolidityMutator::genRandString(unsigned int _length, char _start, char _end)
{
	uniform_int_distribution<char> dist(_start, _end);
	string result{};
	generate_n(back_inserter(result), _length, [&]{ return dist(Rand); });
	return result;
}

antlrcpp::Any SolidityMutator::visitPragmaDirective(SolidityParser::PragmaDirectiveContext*)
{
	auto PickLiteral = [](unsigned const _idx) -> string
	{
		static const vector<string> alphanum = {
			"0",
			"experimental",
			"solidity < 142857",
			"solidity >=0.0.0",
			"solidity >=0.0.0 <0.8.0",
			"experimental __test",
			"experimental SMTChecker",
			"experimental ABIEncoderV2",
			"experimental \"xyz\""
		};

		return alphanum[_idx % alphanum.size()];
	};

	string pragma{};
	if (coinFlip())
		pragma = PickLiteral(Rand());
	else
	{
		// Generate a random pragma that does not contain a semicolon
		pragma = genRandString(randOneToN(30), '<');
		pragma.erase(remove(pragma.begin(), pragma.end(), ';'), pragma.end());
	}

	Out << Whiskers(R"(pragma <string>;<nl>)")
		("string", pragma)
		("nl", "\n")
		.render();
	return antlrcpp::Any();
}

antlrcpp::Any SolidityMutator::visitImportDirective(SolidityParser::ImportDirectiveContext*)
{
	string path = genRandomPath();
	string id = genRandomId();
	switch (randOneToN(4))
	{
	case 1:
		Out << Whiskers(R"(import <path> as <id>;<nl>)")
			("path", path)
			("id", id)
			("nl", "\n")
			.render();
		break;
	case 2:
		Out << Whiskers(R"(import * as <id> from <path>;<nl>)")
			("id", id)
			("path", path)
			("nl", "\n")
			.render();
		break;
	case 3:
	{
		vector<string> symbols{};
		unsigned numElements = randOneToN(10);
		generate_n(
			back_inserter(symbols),
			numElements,
			[&](){
			  return Whiskers((R"(<?as><path> as <id><!as><symbol></as>)"))
				  ("as", coinFlip())
				  ("path", genRandomPath())
				  ("id", genRandomId())
				  ("symbol", genRandString(5, 'A', 'z'))
				  .render();
			}
		);
		Out << Whiskers(R"(import {<syms>} from <path>;<nl>)")
			("syms", boost::algorithm::join(symbols, ", "))
			("path", path)
			("nl", "\n")
			.render();
		break;
	}
	case 4:
		Out << Whiskers(R"(import <path>;<nl>)")
				("path", path)
				("nl", "\n")
				.render();
		break;
	}
	return antlrcpp::Any();
}