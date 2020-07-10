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
#pragma once

#include <libyul/optimiser/Metrics.h>
#include <libyul/optimiser/NameDispenser.h>
#include <libyul/AsmData.h>
#include <libyul/Dialect.h>
#include <libyul/Exceptions.h>

#include <liblangutil/SourceLocation.h>

#include <libsolutil/CommonData.h>

#include <variant>

namespace solidity::yul::unusedFunctionsCommon
{

template<typename T>
std::vector<T> applyBooleanMask(std::vector<T> const& _vec, std::vector<bool> const& _mask)
{
	yulAssert(_vec.size() == _mask.size(), "");

	std::vector<T> ret;

	for (size_t i = 0; i < _mask.size(); ++i)
		if (_mask[i])
			ret.push_back(_vec[i]);

	return ret;
}

bool anyFalse(std::vector<bool> const& _mask)
{
	return any_of(_mask.begin(), _mask.end(), [](bool b){ return !b; });
}

/// Find functions whose body satisfies a heuristic about pruning. Returns true if applying
/// UnusedFunctionParameterPruner is not helpful or redundant because the inliner will be able to
/// handle it anyway.
bool tooSimpleToBePruned(FunctionDefinition const& _f)
{
	return _f.body.statements.size() <= 1 && CodeSize::codeSize(_f.body) <= 1;
}

FunctionDefinition createLinkingFunction(
	FunctionDefinition& _original,
	std::pair<std::vector<bool>, std::vector<bool>> const& _usedParametersAndReturns,
	NameDispenser& _nameDispenser,
	YulString const& _newFunctionName
)
{
	auto generateName = [&](TypedName t)
	{
		return TypedName{
			t.location,
			_nameDispenser.newName(t.name),
			t.type
		};
	};

	langutil::SourceLocation loc = _original.location;

	TypedNameList renamedParameters = util::applyMap(_original.parameters, generateName);
	TypedNameList reducedRenamedParameters = applyBooleanMask(renamedParameters, _usedParametersAndReturns.first);

	TypedNameList renamedReturnVariables = util::applyMap(_original.returnVariables, generateName);
	TypedNameList reducedRenamedReturnVariables = applyBooleanMask(renamedReturnVariables, _usedParametersAndReturns.second);

	FunctionDefinition newFunction{
		loc,
		_original.name,
		renamedParameters,
		renamedReturnVariables,
		{loc, {}} // body
	};

	_original.name = _newFunctionName;
	_original.parameters = applyBooleanMask(_original.parameters, _usedParametersAndReturns.first);
	_original.returnVariables = applyBooleanMask(_original.returnVariables, _usedParametersAndReturns.second);

	FunctionCall call{loc, Identifier{loc, _original.name}, {}};
	for (auto const& p: reducedRenamedParameters)
		call.arguments.emplace_back(Identifier{loc, p.name});

	// Replace the body of `f_1` by an assignment which calls `f`, i.e.,
	// `return_parameters = f(reduced_parameters)`
	if (!_original.returnVariables.empty())
	{
		Assignment assignment;
		assignment.location = loc;

		// The LHS of the assignment.
		for (auto const& r: reducedRenamedReturnVariables)
			assignment.variableNames.emplace_back(Identifier{loc, r.name});

		assignment.value = std::make_unique<Expression>(std::move(call));

		newFunction.body.statements.emplace_back(std::move(assignment));
	}
	else
		newFunction.body.statements.emplace_back(ExpressionStatement{loc, std::move(call)});

	return newFunction;
}

}
