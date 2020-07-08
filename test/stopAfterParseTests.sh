#! /usr/bin/env bash

REPO_ROOT=$(readlink -f "$(dirname "$0")"/..)
SOLIDITY_BUILD_DIR=${SOLIDITY_BUILD_DIR:-${REPO_ROOT}/build}
SOLC=${SOLIDITY_BUILD_DIR}/solc/solc
SPLITSOURCES=${REPO_ROOT}/scripts/splitSources.py

FILETMP=$(mktemp -d)
cd "$FILETMP" || exit 1


function testFile()
{
	ALLOUTPUT=$($SOLC --combined-json ast,compact-format --pretty-json "$@" --stop-after parsing 2>&1)
	if test $? -ne 0; then
		# solc returned failure. Compilation errors and unimplemented features
		# are okay, everything else is a failed test (segfault)
		if ! echo "$ALLOUTPUT" | grep -e "Unimplemented feature:" -e "Error:" -q; then
			echo -n "Test failed on ";
			echo "$@"
			echo "$ALLOUTPUT"
			exit 1;
		fi
	else
		echo -n .
	fi
}

while read -r file; do
	OUTPUT=$($SPLITSOURCES "$file")
	RETURN_CODE=$?
	if [ $RETURN_CODE -eq 0 ]
	then
		# shellcheck disable=SC2086
		testFile $OUTPUT
		rm "${FILETMP:?}/"* -r
	elif [ $RETURN_CODE -eq 1 ]
	then
		# Exclude file as it generates a false error
		if [ "$file" != "${REPO_ROOT}/test/libsolidity/ASTJSON/documentation.sol" ]
		then
			testFile "$file"
		fi
	elif [ $RETURN_CODE -eq 2 ]
	then
		echo -n "<skipping utf8 error>"
	else
		exit 3
	fi
done < <(find "${REPO_ROOT}/test" -iname "*.sol")
echo
