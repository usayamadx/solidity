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

#pragma once

#include <test/tools/ossfuzz/SolidityBaseVisitor.h>

#include <random>

namespace solidity::test::fuzzer {
	using RandomEngine = std::mt19937_64;
	class SolidityMutator: public SolidityBaseVisitor
	{
	public:
		SolidityMutator(unsigned int _seed): Rand(_seed) {}
		std::string toString()
		{
			return Out.str();
		}

		antlrcpp::Any visitPragmaDirective(SolidityParser::PragmaDirectiveContext* _ctx) override;
		antlrcpp::Any visitImportDirective(SolidityParser::ImportDirectiveContext* _ctx) override;
	private:
		/// @returns either true or false with roughly the same probability
		bool coinFlip()
		{
			return Rand() % 2 == 0;
		}

		/// @returns a pseudo randomly chosen unsigned number between one
		/// and @param _n
		unsigned randOneToN(unsigned _n = 20)
		{
			return Rand() % _n + 1;
		}

		/// @returns a pseudo randomly generated path string of
		/// the form `<name>.sol` where `<name>` is a single
		/// character in the ascii range [A-Z].
		std::string genRandomPath()
		{
			return "\"" + genRandString(1, 'A', 'Z') + ".sol" + "\"";
		}

		/// @returns a pseudo randomly generated identifier that
		/// comprises a single character in the ascii range [a-z].
		std::string genRandomId()
		{
			return genRandString(1, 'a', 'z');
		}

		/// @returns a pseudo randomly generated string of length @param _length
		/// characters where each character in the string is in the ASCII character
		/// range between @param _start and @param _end characters.
		std::string genRandString(unsigned _length, char _start = '!', char _end = '~');

		/// Mutant stream
		std::ostringstream Out;
		/// Random number generator
		RandomEngine Rand;
	};
}