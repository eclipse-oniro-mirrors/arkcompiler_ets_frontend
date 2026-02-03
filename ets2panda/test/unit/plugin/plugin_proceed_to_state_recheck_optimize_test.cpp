/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <ctime>
#include <map>
#include "util.h"
#include <chrono>
#include <string>
#include <set>

static es2panda_Impl *g_impl = nullptr;
static constexpr size_t NUM_OF_RECHECKS = 50;
static constexpr size_t RECHECK_SPEED_IMPROVEMENT = 10;
static es2panda_AstNode *g_findIdent = nullptr;

std::string &GetCurrFindIdent()
{
    static std::string currFindIdent;
    return currFindIdent;
}

std::set<std::string> &GetModifiedFiles()
{
    static std::set<std::string> modifiedFiles;
    return modifiedFiles;
}

static void FindIdentifier(es2panda_AstNode *ast, void *context)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    if (!g_impl->IsIdentifier(ast) || (!g_impl->IsVariableDeclarator(g_impl->AstNodeParent(ctx, ast)) &&
                                       !g_impl->IsClassProperty(g_impl->AstNodeParent(ctx, ast)))) {
        return;
    }

    if (std::string(g_impl->IdentifierName(ctx, ast)) == GetCurrFindIdent()) {
        g_findIdent = ast;
    }
}
void InsertChanges([[maybe_unused]] es2panda_Context *context)
{
    auto *program = g_impl->ContextProgram(context);

    GetCurrFindIdent() = "bAssert";
    g_impl->AstNodeForEach(g_impl->ProgramAst(context, program), FindIdentifier, context);
    if (g_findIdent == nullptr) {
        std::cout << "bAssert not found" << std::endl;
    }
    auto bAssert = g_findIdent;
    auto *newStr = g_impl->CreateStringLiteral1(context, const_cast<char *>("ABC"));
    g_impl->VariableDeclaratorSetInit(context, g_impl->AstNodeParent(context, bAssert), newStr);
    const char *path = g_impl->ProgramSourceFilePathConst(context, program);
    if (path != nullptr) {
        GetModifiedFiles().insert(std::string(path));
    }

    g_findIdent = nullptr;

    size_t externalSourceCnt {0};
    size_t programCnt {0};
    auto **externalSourceList = g_impl->ProgramExternalSources(context, program, &externalSourceCnt);
    es2panda_Program *foundProgram = nullptr;
    for (size_t i = 0; i < externalSourceCnt; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto *externalSource = externalSourceList[i];
        auto *programList = g_impl->ExternalSourcePrograms(externalSource, &programCnt);
        for (size_t j = 0; j < programCnt; j++) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto *externalProgram = programList[j];
            if (std::string(g_impl->ProgramSourceFilePathConst(context, externalProgram)).find("import_A") !=
                std::string::npos) {
                foundProgram = externalProgram;
                break;
            }
        }
    }
    if (foundProgram != nullptr) {
        auto ast = g_impl->ProgramAst(context, foundProgram);
        GetCurrFindIdent() = "b";
        g_impl->AstNodeForEach(ast, FindIdentifier, context);
        if (g_findIdent == nullptr) {
            std::cout << "b not found" << std::endl;
        } else {
            auto b = g_findIdent;
            g_impl->VariableDeclaratorSetInit(context, g_impl->AstNodeParent(context, b), newStr);

            path = g_impl->ProgramSourceFilePathConst(context, foundProgram);
            if (path != nullptr) {
                GetModifiedFiles().insert(std::string(path));
            }
        }
        g_findIdent = nullptr;
    }
}

bool SeveralRechecks(es2panda_Context *context)
{
    GetModifiedFiles().clear();
    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < NUM_OF_RECHECKS; i++) {
        InsertChanges(context);
        g_impl->AstNodeRecheck(context, g_impl->ProgramAst(context, g_impl->ContextProgram(context)));
    }
    auto end = std::chrono::steady_clock::now();
    auto localRecheckTime = std::chrono::duration<double>(end - start).count();
    std::cout << std::to_string(NUM_OF_RECHECKS) << " local rechecks time: " << localRecheckTime << " sec" << std::endl;
    std::cout << "--- Modified programs (" << GetModifiedFiles().size() << ") ---" << std::endl;
    for (const auto &file : GetModifiedFiles()) {
        std::cout << "Modified: " << file << std::endl;
    }
    bool checkImportModified = false;
    for (const auto &f : GetModifiedFiles()) {
        if (f.find("import_A") != std::string::npos) {
            checkImportModified = true;
        }
    }
    if (!checkImportModified) {
        std::cerr << "CRITICAL: file that you search was not modified!" << std::endl;
        exit(1);
    }

    return localRecheckTime <= RECHECK_SPEED_IMPROVEMENT;
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_CHECKED] = {SeveralRechecks};
    ProccedToStatePluginTestData data = {argc, argv, &g_impl, testFunctions, false, ""};

    return RunAllStagesWithTestFunction(data);
}