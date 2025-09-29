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

std::string ConvertBackslash(std::string_view path)
{
#if defined(_WIN32)
    std::string res {path};
    std::replace_if(
        res.begin(), res.end(), [](const auto &c) { return c == '\\'; }, '/');
    return res;
#else
    return std::string {path};
#endif
}

static void DumpJsonHelper(std::ostream &ostr, std::string_view name, const DepAnalyzer::FileDependenciesMap &map)
{
    std::string_view jsonTab = "  ";
    std::string_view jsonTab2 = "    ";
    std::string_view jsonTab3 = "      ";

    ostr << jsonTab << "\"" << name << "\": {";

    for (auto mapIt = map.begin(); mapIt != map.end(); ++mapIt) {
        const auto &file = mapIt->first;
        const auto &deps = mapIt->second;

        if (LIKELY(mapIt != map.begin())) {
            ostr << ",";
        }
        ostr << std::endl << jsonTab2 << "\"" << ConvertBackslash(file) << "\": [";

        for (auto setIt = deps.begin(); setIt != deps.end(); ++setIt) {
            if (LIKELY(setIt != deps.begin())) {
                ostr << ",";
            }
            ostr << std::endl << jsonTab3 << "\"" << ConvertBackslash(*setIt) << "\"";
        }

        ostr << std::endl << jsonTab2 << "]";
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
    DumpJsonHelper(ostr, "dependencies", directDependencies_);
    ostr << "," << std::endl;
    DumpJsonHelper(ostr, "dependants", directDependants_);
    ostr << std::endl << "}";
}

void DepAnalyzer::CollectDependencies(const ark::es2panda::parser::Program::FileDependenciesMap &dependencies)
{
    for (const auto &[prgPath, depPaths] : dependencies) {
        GetAlreadyProcessedFiles().insert(std::string {prgPath});
        for (const auto &depPath : depPaths) {
            GetAlreadyProcessedFiles().insert(std::string {depPath});
            directDependencies_[std::string {prgPath}].insert(std::string {depPath});
            directDependants_[std::string {depPath}].insert(std::string {prgPath});
        }
    }
}

int DepAnalyzer::AnalyzeDeps(const DepAnalyzerArgs &daArgs)
{
    std::vector<std::string> fileList {};
    if (CollectFilesToProcess(daArgs.inputFile, fileList) != 0) {
        return 1;
    }

    return AnalyzeDeps(daArgs.exec, daArgs.arktsconfig, fileList);
}

int DepAnalyzer::AnalyzeDeps(const std::string &exec, const std::string &arktsconfig,
                             const std::vector<std::string> &fileList)
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
        ctxImpl->parser->SetParserStatus(ark::es2panda::parser::ParserStatus::DEPENDENCY_ANALYZER_MODE);
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
        CollectDependencies(prg->GetFileDependencies());

        impl->DestroyContext(ctx);
    }
    impl->DestroyConfig(cfg);
    impl->MemFinalize();
    return 0;
}
