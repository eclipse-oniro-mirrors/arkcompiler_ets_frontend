/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include <chrono>

void DepAnalyzer::Dump(std::string &outFilePath)
{
    std::ofstream outFile(outFilePath);
    if (!outFile) {
        std::cerr << "Error when opening a file " << outFilePath << std::endl;
        return;
    }

    Dump(outFile);

    outFile.close();
}

std::string DepAnalyzer::ConvertPath(const std::string &path) const
{
    std::string converted = path;

#if defined(_WIN32)
    constexpr char BACKSLASH = '\\';
    constexpr size_t ESCAPED_BACKSLASH_LENGTH = 2;

    size_t foundPos = 0;
    while ((foundPos = converted.find(BACKSLASH, foundPos)) != std::string::npos) {
        converted.replace(foundPos, 1, "\\\\");
        foundPos += ESCAPED_BACKSLASH_LENGTH;
    }
#endif

    return converted;
}

void DepAnalyzer::Dump(std::ostream &ostr)
{
    std::string jsonTab = "  ";
    ostr << "{\n";

    ostr << jsonTab << "\"dependencies\": {\n";
    bool isFirst = true;
    for (auto [file, deps] : fileDirectDependencies_) {
        if (!isFirst) {
            ostr << ",\n";
        } else {
            isFirst = false;
        }
        ostr << jsonTab << jsonTab << "\"" << ConvertPath(file) << "\": [";
        bool isFirst2 = true;
        for (const auto &dep : deps) {
            if (!isFirst2) {
                ostr << ", ";
            } else {
                isFirst2 = false;
            }
            ostr << "\"" << ConvertPath(dep) << "\"";
        }
        ostr << "]";
    }
    ostr << "\n" << jsonTab << "},\n";

    ostr << jsonTab << "\"dependants\": {\n";
    isFirst = true;
    for (auto [file, deps] : fileDirectDependants_) {
        if (!isFirst) {
            ostr << ",\n";
        } else {
            isFirst = false;
        }
        ostr << jsonTab << jsonTab << "\"" << ConvertPath(file) << "\": [";
        bool isFirst2 = true;
        for (const auto &dep : deps) {
            if (!isFirst2) {
                ostr << ", ";
            } else {
                isFirst2 = false;
            }
            ostr << "\"" << ConvertPath(dep) << "\"";
        }
        ostr << "]";
    }
    ostr << "\n" << jsonTab << "}\n";
    ostr << "}";
}

void DepAnalyzer::AddImports(ark::es2panda::parser::ETSParser *parser)
{
    ark::es2panda::util::StringView firstSourceFilePath = parser->GetGlobalProgramAbsName();
    sourcePaths_.emplace_back(std::string(firstSourceFilePath));

    ark::es2panda::util::ImportPathManager *manager = parser->GetImportPathManager();
    auto &parseList = manager->ParseList();

    for (auto &pl : parseList) {
        sourcePaths_.emplace_back(std::string(pl.importData.resolvedSource));
    }
}

void DepAnalyzer::MergeFileDeps(ark::es2panda::parser::Program *mainProgram)
{
    std::string progAbsPath = std::string {mainProgram->AbsoluteName()};

    if (mainProgram->GetFileDependencies().empty()) {
        fileDirectDependencies_.emplace(progAbsPath, std::unordered_set<std::string>());
    }
    if (fileDirectDependants_.count(progAbsPath) == 0U) {
        fileDirectDependants_.emplace(progAbsPath, std::unordered_set<std::string>());
    }
    for (auto &[_, progs] : mainProgram->DirectExternalSources()) {
        for (auto prog : progs) {
            auto extprogAbsPath = std::string {prog->AbsoluteName()};
            if (extprogAbsPath == progAbsPath) {
                continue;
            }

            fileDirectDependencies_[progAbsPath].insert(extprogAbsPath);
            fileDirectDependants_[extprogAbsPath].insert(progAbsPath);
        }
    }
    for (const auto &[key, valueSet] : mainProgram->GetFileDependencies()) {
        fileDirectDependencies_[key].insert(valueSet.begin(), valueSet.end());
        for (const auto &v : valueSet) {
            fileDirectDependants_[v].insert(key);
        }
    }
}

int DepAnalyzer::AnalyzeDepsForMultiFiles(const char *exec, std::vector<std::string> &fileList,
                                          std::string &arktsconfig)
{
    std::unordered_set<std::string> parsedFileList;
    const auto *impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);

    impl->MemInitialize();

    for (auto &file : fileList) {
        if (parsedFileList.count(file) != 0U || fileDirectDependencies_.count(file) != 0U) {
            continue;
        }

        es2panda_Config *cfg = nullptr;
        if (!arktsconfig.empty()) {
            std::array<const char *, 3> args = {exec, file.c_str(), arktsconfig.c_str()};
            cfg = impl->CreateConfig(args.size(), args.data());
        } else {
            std::array<const char *, 2> args = {exec, file.c_str()};
            cfg = impl->CreateConfig(args.size(), args.data());
        }

        if (cfg == nullptr) {
            std::cerr << "Failed to create config" << std::endl;
            return 1;
        }
        auto *cfgImpl = reinterpret_cast<ark::es2panda::public_lib::ConfigImpl *>(cfg);
        auto parserInputCStr = cfgImpl->options->CStrParserInputContents().first;

        es2panda_Context *ctx =
            impl->CreateContextFromString(cfg, parserInputCStr, cfgImpl->options->SourceFileName().c_str());
        auto *ctxImpl = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
        ctxImpl->parser->SetParserStatus(ark::es2panda::parser::ParserStatus::DEPENDENCY_ANALYZER_MODE);
        ctxImpl->depAnalyzer = this;
        impl->ProceedToState(ctx, ES2PANDA_STATE_PARSED);

        if (ctxImpl->state == ES2PANDA_STATE_ERROR) {
            ctxImpl->GetChecker()->LogTypeError(std::string("Parse Failed: ").append(ctxImpl->errorMessage),
                                                ctxImpl->errorPos);
            impl->DestroyContext(ctx);
            impl->DestroyConfig(cfg);
            return 1;
        }

        ark::es2panda::parser::Program *mainProgram = ctxImpl->parserProgram;
        std::string mainProgramAbsPath = std::string {mainProgram->AbsoluteName()};
        parsedFileList.insert(mainProgramAbsPath);
        MergeFileDeps(mainProgram);

        impl->DestroyContext(ctx);
        impl->DestroyConfig(cfg);
    }

    impl->MemFinalize();
    return 0;
}

static void AddFileList(std::string &fileListPath, std::vector<std::string> &fileList)
{
    std::ifstream inFile(fileListPath);
    if (!inFile.is_open()) {
        std::cerr << "Error when opening a file " << fileListPath << std::endl;
        return;
    }
    std::string line;
    while (getline(inFile, line)) {
        if (!line.empty()) {
            fileList.emplace_back(line);
        }
    }
}

std::optional<DepAnalyzerArgs> ParseArguments(ark::Span<const char *const> args)
{
    DepAnalyzerArgs parsedArgs;
    parsedArgs.programName = args[0];

    for (size_t i = 1; i < args.size(); i++) {
        if (std::strncmp(args[i], "--arktsconfig=", std::strlen("--arktsconfig=")) == 0) {
            parsedArgs.arktsconfig = args[i];
            continue;
        }
        if (std::strncmp(args[i], "@", std::strlen("@")) == 0) {
            std::string_view arg(args[i]);
            std::string fileListPath(arg.substr(1));
            AddFileList(fileListPath, parsedArgs.fileList);
            continue;
        }
        parsedArgs.fileList.emplace_back(args[i]);
    }

    return parsedArgs;
}

int DepAnalyzer::AnalyzeDeps(int argc, const char **argv)
{
    // NOLINTBEGIN
    int minArgCount = 2;
    if (argc < minArgCount) {
        std::cerr << "No file has been entered for analysis" << std::endl;
        return 1;
    }
    ark::Span<const char *const> args(argv, static_cast<size_t>(argc));
    auto parsedArgs = ParseArguments(args);
    if (AnalyzeDepsForMultiFiles(parsedArgs->programName.c_str(), parsedArgs->fileList, parsedArgs->arktsconfig) != 0) {
        return 1;
    }
    return 0;
    // NOLINTEND
}
