/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dep_analyzer.h"

DepAnalyzerArgs ParseArguments(const ark::Span<const char *const> &args)
{
    DepAnalyzerArgs parsedArgs {};
    std::vector<std::string> daArgs {};

    for (const auto &arg : args) {
        daArgs.emplace_back(arg);
    }

    parsedArgs.exec = daArgs.front();
    for (const auto &arg : daArgs) {
        if (arg.find("--arktsconfig=") == 0U) {
            parsedArgs.arktsconfig = arg.substr(std::strlen("--arktsconfig="));
        } else if (arg.find("--output=") == 0U) {
            parsedArgs.outputFile = arg.substr(std::strlen("--output="));
        } else if (arg.find("@") == 0U) {
            parsedArgs.inputFile = arg.substr(std::strlen("@"));
        }
    }

    return parsedArgs;
}

int main(int argc, const char **argv)
{
    ark::Span<const char *const> args(argv, argc);

    DepAnalyzer da;
    DepAnalyzerArgs parsedArgs = ParseArguments(args);
    if (da.AnalyzeDeps(parsedArgs) != 0) {
        return 1;
    }

    if (parsedArgs.outputFile.empty()) {
        da.DumpJson();
    } else {
        da.DumpJson(parsedArgs.outputFile);
    }

    return 0;
}
