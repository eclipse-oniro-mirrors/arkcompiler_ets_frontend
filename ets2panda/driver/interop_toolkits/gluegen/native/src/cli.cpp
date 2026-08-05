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

#include <sstream>

namespace ark::es2panda::gluegen {

namespace {
constexpr const char *PROGRAM_NAME = "gluegen";
}  // namespace

CliOptions::CliOptions()
    : inputFileList_("input-file-list", "", "Path to a file listing input source files, one per line (required)"),
      singleFileEmit_("single-file-emit", false, "Emit one output file per input source file"),
      arktsconfig_("arktsconfig", "", "Path to arktsconfig.json"),
      output_("output", "", "Path where the final generated artifact is written"),
      cachePath_("cache-path", "", "Path to the intermediate cache/artifacts directory"),
      reportPath_("report-path", "",
                  "Path where the diagnostics report is written; if a directory is given, the "
                  "report is written to <directory>/report.json"),
      targetApiVersion_("target-api-version", 0, "Target API version")
{
    parser_.Add(&inputFileList_);
    parser_.Add(&singleFileEmit_);
    parser_.Add(&arktsconfig_);
    parser_.Add(&output_);
    parser_.Add(&cachePath_);
    parser_.Add(&reportPath_);
    parser_.Add(&targetApiVersion_);
}

bool CliOptions::Parse(int argc, const char *const argv[])
{
    if (!parser_.Parse(argc, argv)) {
        return false;
    }
    if (!inputFileList_.WasSet()) {
        return false;
    }
    return true;
}

std::string CliOptions::GetErrorString() const
{
    std::string err = parser_.GetErrorString();
    if (!inputFileList_.WasSet()) {
        err += std::string(PROGRAM_NAME) + ": '--input-file-list' is required\n";
    }
    return err;
}

std::string CliOptions::GetHelpString() const
{
    std::ostringstream ss;
    ss << "Usage: " << PROGRAM_NAME << " --input-file-list <files> [OPTIONS]\n\n";
    ss << "optional arguments:\n";
    ss << parser_.GetHelpString();
    return ss.str();
}

}  // namespace ark::es2panda::gluegen
