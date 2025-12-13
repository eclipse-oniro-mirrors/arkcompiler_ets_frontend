/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "public/es2panda_lib.h"
#include "public/public.h"
#include "declgenEts2Ts.h"
#include "ir/astNodeHistory.h"
#include "isolatedDeclgenChecker.h"

namespace ark::es2panda::declgen_ets2ts {

static void PrintUsage()
{
    std::cerr << "Usage: declgen_ets2ts [OPTIONS] [input]\n";
    std::cerr << "    --export-all         Treat all top level statements as exported\n";
    std::cerr << "    --output-dets=[FILE] Path to output .d.ets file\n";
    std::cerr << "    --output-ets=[FILE]  Path to output .ets file\n";
    std::cerr << "    --help               Print this message and exit. Default: false\n";
    std::cerr << "Tail arguments:\n";
    std::cerr << "input: input file\n";
}

void FilterArgs(Span<const char *const> args, int &newArgc, const char **&newArgv)
{
    ES2PANDA_ASSERT(args.size() > 1);
    std::vector<const char *> filteredArgs;
    filteredArgs.push_back(args[0]);
    for (size_t i = 1; i < args.size(); ++i) {
        if (std::strcmp(args[i], "--export-all") == 0 ||
            std::strncmp(args[i], "--output-dets", std::strlen("--output-dets")) == 0 ||
            std::strncmp(args[i], "--output-ets", std::strlen("--output-ets")) == 0) {
            continue;
        }
        filteredArgs.push_back(args[i]);
    }

    newArgc = static_cast<int>(filteredArgs.size());
    if (newArgc <= 0 || static_cast<size_t>(newArgc) > args.size()) {
        return;
    }
    newArgv = new const char *[newArgc];
    std::copy(filteredArgs.begin(), filteredArgs.end(), newArgv);
}

static DeclgenOptions ParseOptions(Span<const char *const> args)
{
    DeclgenOptions options;
    for (size_t i = 1; i < args.size(); ++i) {
        if (std::strcmp(args[i], "--export-all") == 0) {
            options.exportAll = true;
        } else if (std::strncmp(args[i], "--output-dets", std::strlen("--output-dets")) == 0 &&
                   std::strchr(args[i], '=') != nullptr) {
            std::string arg = args[i];
            options.outputDeclEts = arg.substr(std::strlen("--output-dets="));
        } else if (std::strncmp(args[i], "--output-ets", std::strlen("--output-ets")) == 0 &&
                   std::strchr(args[i], '=') != nullptr) {
            std::string arg = args[i];
            options.outputEts = arg.substr(std::strlen("--output-ets="));
        }
    }
    return options;
}

static int Run(int argc, const char **argv)
{
    Span<const char *const> args(argv, static_cast<size_t>(argc));
    if (args.size() == 1 || std::strcmp(args[1], "--help") == 0) {
        PrintUsage();
        return 1;
    }
    auto declgenOptions = ParseOptions(args);
    int newArgc = 0;
    const char **newArgv = nullptr;
    FilterArgs(args, newArgc, newArgv);
    const auto *impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *cfg = impl->CreateConfig(newArgc, newArgv);
    if (cfg == nullptr) {
        return 1;
    }
    auto *cfgImpl = reinterpret_cast<ark::es2panda::public_lib::ConfigImpl *>(cfg);
    auto parserInputCStr = cfgImpl->options->CStrParserInputContents().first;
    es2panda_Context *ctx =
        impl->CreateContextFromStringWithHistory(cfg, parserInputCStr, cfgImpl->options->SourceFileName().c_str());
    auto *ctxImpl = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *checker = reinterpret_cast<checker::ETSChecker *>(ctxImpl->GetChecker());
    ctxImpl->lazyCheck = false;

    impl->ProceedToState(ctx, ES2PANDA_STATE_CHECKED);

    int res = 0;
    if (!GenerateTsDeclarations(checker, ctxImpl->parserProgram, declgenOptions)) {
        res = 1;
    }

    impl->DestroyContext(ctx);
    impl->DestroyConfig(cfg);
    delete[] newArgv;

    return res;
}
}  // namespace ark::es2panda::declgen_ets2ts

int main(int argc, const char **argv)
{
    return ark::es2panda::declgen_ets2ts::Run(argc, argv);
}
