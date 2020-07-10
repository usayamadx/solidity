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
// SPDX-License-Identifier: GPL-3.0
/**
 * UnusedFunctionParameterPruner: Optimiser step that removes unused parameters from function
 * definition.
 */

#include <libyul/optimiser/UnusedFunctionParameterPruner.h>
#include <libyul/optimiser/UnusedFunctionsCommon.h>
#include <libyul/optimiser/OptimiserStep.h>
#include <libyul/optimiser/NameCollector.h>
#include <libyul/optimiser/NameDisplacer.h>
#include <libyul/optimiser/NameDispenser.h>
#include <libyul/YulString.h>
#include <libyul/AsmData.h>

#include <libsolutil/CommonData.h>

#include <algorithm>
#include <optional>
#include <variant>

using namespace std;
using namespace solidity::util;
using namespace solidity::yul;
using namespace solidity::yul::unusedFunctionsCommon;

void UnusedFunctionParameterPruner::run(OptimiserStepContext& _context, Block& _ast)
{
	map<YulString, size_t> references = ReferencesCounter::countReferences(_ast);
	auto used = [&](auto v) -> bool { return references.count(v.name); };

	// Function name and a pair of boolean masks, the first corresponds to parameters and the second
	// corresponds to returnVariables.
	//
	// For the first vector in the pair, a value `false` at index `i` indicates that the function
	// argument at index `i` in `FunctionDefinition::parameters` is unused inside the function body.
	//
	// Similarly for the second vector in the pair, a value `false` at index `i` indicates that the
	// return parameter at index `i` in `FunctionDefinition::returnVariables` is unused inside
	// function body.
	map<YulString, pair<vector<bool>, vector<bool>>> usedParametersAndReturnVariables;

	// Step 1 of UnusedFunctionParameterPruner: Find functions whose parameters (both arguments and
	// return-parameters) are not used in its body.
	for (auto const& statement: _ast.statements)
		if (holds_alternative<FunctionDefinition>(statement))
		{
			FunctionDefinition const& f = std::get<FunctionDefinition>(statement);

			if (tooSimpleToBePruned(f))
				continue;

			vector<bool> parameters = applyMap(f.parameters, used);
			vector<bool> returnVariables = applyMap(f.returnVariables, used);

			pair<vector<bool>, vector<bool>> parametersAndReturnVariables{
				vector<bool>(f.parameters.size(), true),
				vector<bool>(f.returnVariables.size(), true)
			};

			bool hasUnused = false;
			if (anyFalse(parameters))
			{
				parametersAndReturnVariables.first = move(parameters);
				hasUnused = true;
			}
			if (anyFalse(returnVariables))
			{
				parametersAndReturnVariables.second = move(returnVariables);
				hasUnused = true;
			}

			if (hasUnused)
				usedParametersAndReturnVariables[f.name] = move(parametersAndReturnVariables);
		}

	set<YulString> functionNamesToFree = util::keys(usedParametersAndReturnVariables);

	// Step 2 of UnusedFunctionParameterPruner: Renames the function and replaces all references to
	// the function, say `f`, by its new name, say `f_1`.
	NameDisplacer replace{_context.dispenser, functionNamesToFree};
	replace(_ast);

	// Inverse-Map of the above translations. In the above example, this will store an element with
	// key `f_1` and value `f`.
	std::map<YulString, YulString> newToOriginalNames = invertMap(replace.translations());

	// Step 3 of UnusedFunctionParameterPruner: introduce a new function in the block with body of
	// the old one. Replace the body of the old one with a function call to the new one with reduced
	// parameters.
	//
	// For example: introduce a new 'linking' function `f` with the same the body as `f_1`, but with
	// reduced parameters, i.e., `function f() -> y { y := 1 }`. Now replace the body of `f_1` with
	// a call to `f`, i.e., `f_1(x) -> y { y := f() }`.
	iterateReplacing(_ast.statements, [&](Statement& _s) -> optional<vector<Statement>> {
		if (holds_alternative<FunctionDefinition>(_s))
		{
			FunctionDefinition& originalFunctionWithNewName = get<FunctionDefinition>(_s);
			if (newToOriginalNames.count(originalFunctionWithNewName.name))
			{
				YulString originalName = newToOriginalNames.at(originalFunctionWithNewName.name);

				// After this, the name of `originalFunctionWithNewName` will be changed back to
				// original, i.e., `f_1 -> f`. Its parameters (both arguments and return-variables)
				// will be pruned. The function returned will have the new name, i.e., `f_1`, and
				// becomes the 'linking' function.
				FunctionDefinition linkingFunction = createLinkingFunction(
					originalFunctionWithNewName,
					usedParametersAndReturnVariables.at(originalName),
					_context.dispenser,
					originalName
				);

				return make_vector<Statement>(move(originalFunctionWithNewName), move(linkingFunction));
			}
		}

		return nullopt;
	});
}
