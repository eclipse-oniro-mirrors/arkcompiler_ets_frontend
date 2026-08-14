/**
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_UTIL_SYMBOL_TABLE_H
#define ES2PANDA_UTIL_SYMBOL_TABLE_H

#include "util/eheap.h"

#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace ark::es2panda::util {

class SymbolTable {
public:
    static const std::string FIRST_LEVEL_SEPERATOR;
    // Tab character — avoids collision with ';' used in ETS pandasm type signatures
    // (e.g. "funcName:i32;i32;i32"). The es2panda symbol table format uses ';' but
    // ETS function names contain ';' as part of their type-qualified names.
    static const std::string SECOND_LEVEL_SEPERATOR;

    struct OriginFunctionInfo {
        std::string recordName;
        std::string funcInternalName;
        std::string funcHash;
    };

    SymbolTable(const std::string &inputSymbolTable, const std::string &dumpSymbolTable)
        : symbolTable_(inputSymbolTable),
          dumpSymbolTable_(dumpSymbolTable),
          allocator_(SpaceType::SPACE_TYPE_COMPILER, nullptr, true),
          originFunctionInfo_(allocator_.Adapter()),
          originModuleInfo_(allocator_.Adapter())
    {
    }

    bool Initialize(int targetApiVersion, std::string targetApiSubVersion);
    void FillSymbolTable(const std::stringstream &content);
    void WriteSymbolTable();

    SArenaUnorderedMap<std::string, OriginFunctionInfo> *GetOriginFunctionInfo()
    {
        return &originFunctionInfo_;
    }

    SArenaUnorderedMap<std::string, std::string> *GetOriginModuleInfo()
    {
        return &originModuleInfo_;
    }

private:
    bool ReadSymbolTable(const std::string &symbolTable);
    bool ParseSymbolTableLine(const std::string &line);
    std::vector<std::string_view> GetStringItems(std::string_view input, const std::string &separator);

    std::mutex m_;
    std::string symbolTable_;
    std::string dumpSymbolTable_;
    SArenaAllocator allocator_;
    SArenaUnorderedMap<std::string, OriginFunctionInfo> originFunctionInfo_;
    SArenaUnorderedMap<std::string, std::string> originModuleInfo_;

    std::stringstream symbolTableContent_;
};

}  // namespace ark::es2panda::util

#endif  // ES2PANDA_UTIL_SYMBOL_TABLE_H
