/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "cli.h"
#include "diagnostic.h"
#include "gluegen.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "libarkbase/utils/expected.h"
#include "utils.h"

namespace {

// Reads the file at `path` (the file pointed to by --input-file-list), one
// source file path per line; blank lines are skipped. Returns the list of
// source files, or an error message if the file cannot be opened.
ark::Expected<std::vector<std::string>, std::string> ReadInputFileList(const std::string &path)
{
    std::ifstream file(ark::es2panda::gluegen::ToLongPathIfNeeded(path));
    if (!file.is_open()) {
        return ark::Unexpected<std::string>("Gluegen: failed to open --input-file-list file: " + path);
    }
    std::vector<std::string> files;
    std::string line;
    while (std::getline(file, line)) {
        auto first = line.find_first_not_of(" \t\n\r");
        auto last = line.find_last_not_of(" \t\n\r");
        if (first == std::string::npos || last == std::string::npos) {
            continue;
        }
        files.push_back(line.substr(first, last - first + 1));
    }
    return files;
}

}  // namespace

int main(int argc, char **argv)
{
    ark::es2panda::gluegen::CliOptions cliOptions;
    if (!cliOptions.Parse(argc, argv)) {
        std::cerr << cliOptions.GetErrorString();
        std::cerr << cliOptions.GetHelpString();
        return 1;
    }

    auto sourceFiles = ReadInputFileList(cliOptions.InputFileList());
    if (!sourceFiles) {
        std::cerr << sourceFiles.Error() << std::endl;
        return 1;
    }

    ark::es2panda::gluegen::GluegenOptions options;
    options.sourceFiles = std::move(sourceFiles.Value());
    options.singleFileEmit = cliOptions.SingleFileEmit();
    options.arktsconfigPath = cliOptions.ArktsConfig();
    options.outputPath = cliOptions.Output();
    options.cacheDir = cliOptions.CachePath();
    options.reportPath = cliOptions.ReportPath();

    try {
        ark::es2panda::gluegen::StreamDiagnosticConsumer diagnosticConsumer(std::cerr);
        auto gluegen = ark::es2panda::gluegen::Gluegen(options, &diagnosticConsumer);
        if (auto result = gluegen.Run(); !result) {
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}