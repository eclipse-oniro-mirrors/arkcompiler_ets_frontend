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

#ifndef ES2PANDA_DEPENDENCY_ANALYZER_H
#define ES2PANDA_DEPENDENCY_ANALYZER_H

#include "public/public.h"

struct DepAnalyzerArgs {
    std::string exec;
    std::string arktsconfig;
    std::string inputFile;
    std::string outputFile;
};

class DepAnalyzer {
public:
    using ProcessedFilesMap = std::unordered_set<std::string>;
    using FileDependenciesMap = std::unordered_map<std::string, std::unordered_set<std::string>>;

    int AnalyzeDeps(const DepAnalyzerArgs &args);
    int AnalyzeDeps(const std::string &exec, const std::string &arktsconfig, const std::vector<std::string> &fileList);
    void DumpJson(std::string &outFilePath);
    void DumpJson(std::ostream &ostr = std::cout);

    const ProcessedFilesMap &GetAlreadyProcessedFiles() const
    {
        return alreadyProcessedFiles_;
    }

    const FileDependenciesMap &GetDirectDependencies() const
    {
        return directDependencies_;
    }

    const FileDependenciesMap &GetDirectDependants() const
    {
        return directDependants_;
    }

private:
    ProcessedFilesMap &GetAlreadyProcessedFiles()
    {
        return alreadyProcessedFiles_;
    }

    void CollectDependencies(const ark::es2panda::parser::Program::FileDependenciesMap &dependencies);

    ProcessedFilesMap alreadyProcessedFiles_ {};
    FileDependenciesMap directDependencies_ {};
    FileDependenciesMap directDependants_ {};
};

#endif  // ES2PANDA_DEPENDENCY_ANALYZER_H
