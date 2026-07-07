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

void DepAnalyzer::DumpJson(std::string &outFilePath)
{
    std::ofstream outFile(outFilePath);
    if (outFile.fail()) {
        std::cerr << "Error when opening a file " << outFilePath << std::endl;
        return;
    }

    std::stringstream ss;
    DumpJson(ss);
    outFile << ss.rdbuf();
}

std::string EscapeBackslash(std::string_view path)
{
#if defined(_WIN32)
    std::ostringstream ss {};
    for (const auto &c : path) {
        if (c == '\\') {
            // Print backslash twice
            ss << c;
        }
        ss << c;
    }
    return ss.str();
#else
    return std::string {path};
#endif
}

// DepAnalyzer::FileDependenciesMap
template <typename ValueT>
static void DumpJsonHelper(std::ostream &ostr, std::string_view name, const ValueT &map)
{
    std::string_view jsonTab = "  ";
    std::string_view jsonTab2 = "    ";
    std::string_view jsonTab3 = "      ";

    ostr << jsonTab << "\"" << name << "\": {";

    auto dumpArray = [&ostr, &jsonTab3](const auto &valueArray) {
        for (auto setIt = valueArray.begin(); setIt != valueArray.end(); ++setIt) {
            if (LIKELY(setIt != valueArray.begin())) {
                ostr << ",";
            }
            ostr << std::endl << jsonTab3 << "\"" << EscapeBackslash(*setIt) << "\"";
        }
    };

    for (auto mapIt = map.begin(); mapIt != map.end(); ++mapIt) {
        const auto &file = mapIt->first;
        const auto &value = mapIt->second;

        if (LIKELY(mapIt != map.begin())) {
            ostr << ",";
        }

        if constexpr (std::is_same_v<ValueT, DepAnalyzer::FileOutputMatching>) {
            ostr << std::endl
                 << jsonTab2 << "\"" << EscapeBackslash(file) << "\": \"" << EscapeBackslash(value) << "\"";
        } else {
            ostr << std::endl << jsonTab2 << "\"" << EscapeBackslash(file) << "\": [";
            dumpArray(value);
            ostr << std::endl << jsonTab2 << "]";
        }
    }
    ostr << std::endl << jsonTab << "}";
}

static int CollectFilesToProcess(const std::string &fileListPath, std::vector<std::string> &fileList)
{
    std::ifstream inFile(fileListPath);
    if (inFile.fail()) {
        std::cerr << "Error when opening a file " << fileListPath << std::endl;
        return 1;
    }

    std::stringstream ss;
    ss << inFile.rdbuf();

    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty()) {
            fileList.emplace_back(line);
        }
    }

    return 0;
}

void DepAnalyzer::DumpJson(std::ostream &ostr)
{
    ostr << "{" << std::endl;
    DumpJsonHelper<DepAnalyzer::FileDependenciesMap>(ostr, "dependencies", directDependencies_);
    ostr << "," << std::endl;
    DumpJsonHelper<DepAnalyzer::FileDependenciesMap>(ostr, "dependants", directDependants_);
    ostr << "," << std::endl;
    DumpJsonHelper<DepAnalyzer::FileOutputMatching>(ostr, "outputMatching", outputMatching_);
    ostr << std::endl << "}";
}

void DepAnalyzer::CollectData(const ark::es2panda::util::ImportPathManager *ipm)
{
    const auto &dependencies = ipm->GetFileDependencies();
    for (const auto &[prgPath, depPaths] : dependencies) {
        GetAlreadyProcessedFiles().insert(std::string {prgPath});
        for (const auto &depPath : depPaths) {
            GetAlreadyProcessedFiles().insert(std::string {depPath});
            directDependencies_[std::string {prgPath}].insert(std::string {depPath});
            directDependants_[std::string {depPath}].insert(std::string {prgPath});
        }
    }

    const auto &outputMatching = ipm->GetOutputMatching();
    for (const auto &[prgPath, abcPath] : outputMatching) {
        outputMatching_.try_emplace(std::string {prgPath}, std::string {abcPath});
    }
}

int DepAnalyzer::AnalyzeDeps(const DepAnalyzerArgs &daArgs)
{
    std::vector<std::string> fileList {};
    if (CollectFilesToProcess(daArgs.inputFile, fileList) != 0) {
        return 1;
    }

    return AnalyzeDeps(daArgs.exec, daArgs.arktsconfig, fileList, daArgs.incremental);
}

int DepAnalyzer::AnalyzeDeps(const std::string &exec, const std::string &arktsconfig,
                             const std::vector<std::string> &fileList, bool incremental)
{
    const auto *impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    impl->MemInitialize();

    es2panda_Config *cfg = nullptr;

    std::vector es2pandaArgs = {exec.c_str()};

    std::string arktsconfigArg {};
    if (!arktsconfig.empty()) {
        arktsconfigArg = "--arktsconfig=" + arktsconfig;
        es2pandaArgs.push_back(arktsconfigArg.c_str());
    }

    cfg = impl->CreateConfig(es2pandaArgs.size(), es2pandaArgs.data());
    if (cfg == nullptr) {
        std::cerr << "Failed to create config" << std::endl;
        return 1;
    }

    for (const auto &fileToAnalyze : fileList) {
        if (GetAlreadyProcessedFiles().find(fileToAnalyze) != GetAlreadyProcessedFiles().end()) {
            continue;
        }

        es2panda_Context *ctx = impl->CreateContextFromFile(cfg, fileToAnalyze.c_str());
        auto *ctxImpl = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
        ctxImpl->parser->AddParserStatus(ark::es2panda::parser::ParserStatus::DEPENDENCY_ANALYZER_MODE);
        if (incremental) {
            ctxImpl->parser->AddParserStatus(ark::es2panda::parser::ParserStatus::INCREMENTAL_DEPENDENCY_ANALYZER_MODE);
        }
        ctxImpl->depAnalyzer = this;

        impl->ProceedToState(ctx, ES2PANDA_STATE_PARSED);
        if (ctxImpl->state == ES2PANDA_STATE_ERROR) {
            ctxImpl->GetChecker()->LogTypeError(std::string("Parse Failed: ").append(ctxImpl->errorMessage),
                                                ctxImpl->errorPos);
            impl->DestroyContext(ctx);
            impl->DestroyConfig(cfg);
            impl->MemFinalize();
            return 1;
        }

        ark::es2panda::parser::Program *prg = ctxImpl->parserProgram;
        GetAlreadyProcessedFiles().insert(prg->AbsoluteName().Mutf8());
        CollectData(ctxImpl->parser->GetImportPathManager());

        impl->DestroyContext(ctx);
    }
    impl->DestroyConfig(cfg);
    impl->MemFinalize();
    return 0;
}
