# ---------------------------------------------------------------------------
# Batch file for implementing package flow for solidity for Windows.
#
# The documentation for solidity is hosted at:
#
#     https://solidity.readthedocs.org
#
# ---------------------------------------------------------------------------
# This file is part of solidity.
#
# solidity is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# solidity is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with solidity.  If not, see <http://www.gnu.org/licenses/>
#
# Copyright (c) 2016 solidity contributors.
# ---------------------------------------------------------------------------

$ZipFile = "solidity-windows.zip"

Write-Output "Collecting files ..."
$ExeFiles = Get-ChildItem -Path . -Recurse -File `
    | Where-Object {$_.Name -eq "soltest.exe" `
                -or $_.Name -eq "solc.exe" `
                } `
    | Select-Object -ExpandProperty FullName

Write-Output "Collected EXE files: $ExeFiles"

# Sadly file globbing inside the PowerShell script didn't work (only on command line), so I had
# to do some Where-Object- & findstr-fu to achieve the same.
# Normally the prefix should be something like: "C:\Program Files (x86)\Microsoft Visual Studio\2019\*\VC\Redist\MSVC\*\x86\"
# that should only expand to a single path (unless you have multiple versions installed).
# $DLLS = Get-ChildItem -Path "C:\Program Files (x86)\Microsoft Visual Studio\2019" `
#                       -Exclude onecore `
#                       -Recurse -File -Include "msvc*.dll" `
#                       | Where-Object {$_.Directory -match "Microsoft.VC142.CRT"} `
#                       | Select-Object -ExpandProperty FullName `
#                       | findstr "\x86\" `
#                       | findstr /V "\onecore\"

$DLLS = Get-ChildItem -Path "C:\Program Files (x86)\Microsoft Visual Studio\2019" -Recurse -Filter "VCRUNTIME140_1.dll" `
	  | Where-Object {$_.DirectoryName -Match "Hostx64\\x86"} `
	  | Select-Object -ExpandProperty FullName

Write-Output "Collected DLL files: $DLLS"

7z a $ZipFile $ExeFiles $DLLS

Write-Output "Resulting ZIP file:"

7z l $ZipFile
