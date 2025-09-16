# Copyright (c) 2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This script will replace the old paths with the new ones in all files within the testcases directory.

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Check if the 'ets' directory exists in ScriptDir
if (-not (Test-Path -Path "$ScriptDir\ets" -PathType Container)) {
    Write-Error "Error: 'ets' directory not found in $ScriptDir."
    Write-Error "Please make sure the 'ets' directory exists before running bindings test."
    exit 1
}

$RestoreMode = 0
if ($args.Count -gt 0 -and $args[0] -eq "--restore") {
    $RestoreMode = 1
}

if ($RestoreMode -eq 1) {
    if (Test-Path -Path "$ScriptDir\..\ets2panda") {
        Remove-Item -Path "$ScriptDir\..\ets2panda" -Recurse -Force
        Write-Host "Removed '$ScriptDir\..\ets2panda' directory."
    }
}
else {
    $sourceDir = "$ScriptDir\ets\ets1.2\build-tools\ets2panda"
    $destinationDir = "$ScriptDir\..\ets2panda"
    
    # Check if source directory exists
    if (-not (Test-Path -Path $sourceDir -PathType Container)) {
        Write-Error "Source directory '$sourceDir' does not exist."
        exit 1
    }
    
    # Remove destination directory if it exists
    if (Test-Path -Path $destinationDir) {
        Remove-Item -Path $destinationDir -Recurse -Force
    }
    
    # Copy directory
    try {
        Copy-Item -Path $sourceDir -Destination $destinationDir -Recurse -Force
        Write-Host "Directory copied successfully from '$sourceDir' to '$destinationDir'."
    }
    catch {
        Write-Error "Failed to copy directory."
        Write-Error $_.Exception.Message
        exit 1
    }
}

exit 0