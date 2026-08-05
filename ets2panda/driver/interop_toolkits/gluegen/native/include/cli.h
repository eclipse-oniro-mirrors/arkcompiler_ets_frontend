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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_CLI_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_CLI_H

#include <string>

#include "libarkbase/utils/pandargs.h"

namespace ark::es2panda::gluegen {

// Wraps `ark::PandArgParser` (libarkbase/pandarg) to parse gluegen's command line:
//   --input-file-list  <path>  Path to a file listing input source files, one per line. Required.
//   --single-file-emit          When set, emit one output file per input source file.
//   --arktsconfig      <path>  Path to arktsconfig.json. Optional.
//   --output           <path>  Path where the final generated artifact is written. Optional.
//   --cache-path       <path>  Path to the intermediate cache/artifacts directory. Optional.
//   --report-path      <path>  Directory where the diagnostics report is written. Optional.
class CliOptions {
public:
    CliOptions();

    CliOptions(const CliOptions &) = delete;
    CliOptions &operator=(const CliOptions &) = delete;
    CliOptions(CliOptions &&) = delete;
    CliOptions &operator=(CliOptions &&) = delete;
    ~CliOptions() = default;

    // Parses `argv` (argv[0] is expected to be the program name, matching
    // `ark::PandArgParser::Parse(int, const char *const *)`'s convention).
    // Returns false when parsing fails or a required option is missing; in
    // that case, callers should report `GetErrorString()`/`GetHelpString()`.
    bool Parse(int argc, const char *const argv[]);

    const std::string &InputFileList() const
    {
        return inputFileList_.GetValue();
    }

    bool SingleFileEmit() const
    {
        return singleFileEmit_.GetValue();
    }

    const std::string &ArktsConfig() const
    {
        return arktsconfig_.GetValue();
    }

    const std::string &Output() const
    {
        return output_.GetValue();
    }

    const std::string &CachePath() const
    {
        return cachePath_.GetValue();
    }

    const std::string &ReportPath() const
    {
        return reportPath_.GetValue();
    }

    std::string GetErrorString() const;
    std::string GetHelpString() const;

private:
    ark::PandArgParser parser_;
    // Initialized in cli.cpp's constructor (PandArg has no default constructor, and the
    // name/default-value/description arguments are long enough to unwrap poorly here).
    ark::PandArg<std::string> inputFileList_;
    ark::PandArg<bool> singleFileEmit_;
    ark::PandArg<std::string> arktsconfig_;
    ark::PandArg<std::string> output_;
    ark::PandArg<std::string> cachePath_;
    ark::PandArg<std::string> reportPath_;
    ark::PandArg<int> targetApiVersion_;
};

}  // namespace ark::es2panda::gluegen

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_CLI_H
