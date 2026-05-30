#!/bin/bash
# Copyright (c) 2026 Huawei Device Co., Ltd.
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

set -e

find test/e2e/obfuscation_config_demo -name 'build_config.json' -exec sed -i 's|${absolute_path_to_build_system}|'"$(pwd)"'|g' {} +

scripts=(
"obfuscation_config_demo_hap:gen_abc"
"obfuscation_config_demo_har:gen_abc"
"obfuscation_config_demo_har2:gen_abc"
"obfuscation_config_demo_hsp:gen_abc"
"obfuscation_config_demo_hsp_har:gen_abc"
)

passed=()
failed=()

for script in "${scripts[@]}"; do
  echo "Running E2E test: $script"
  TEST=$script npx jest --testMatch='**/test/e2e/*.test.ts' --testPathIgnorePatterns='test/ut/'
  #npm run "$script"
  if [ $? -eq 0 ]; then
    passed+=("$script")
  else
    failed+=("$script")
  fi
done

echo
echo "================== E2E Test Summary =================="
total=$(( ${#scripts[@]} ))
echo "Total: $total"
echo "Passed: ${#passed[@]}"
echo "Failed: ${#failed[@]}"
if [ ${#passed[@]} -gt 0 ]; then
  echo "Passed tests:"
  for s in "${passed[@]}"; do
    echo "  $s"
  done
fi
if [ ${#failed[@]} -gt 0 ]; then
  echo "Failed tests:"
  for s in "${failed[@]}"; do
    echo "  $s"
  done
fi
echo "======================================================"

exit 0