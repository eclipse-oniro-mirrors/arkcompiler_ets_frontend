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

#include "symbolTable.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#include "util/helpers.h"

namespace ark::es2panda::util {

const std::string SymbolTable::FIRST_LEVEL_SEPERATOR = "|";
const std::string SymbolTable::SECOND_LEVEL_SEPERATOR = "\t";
const size_t FUNC_FIELD_COUNT = 3;    // name, internalName, hash
const size_t MODULE_FIELD_COUNT = 2;  // key, value
const size_t FUNC_IDX_NAME = 0;
const size_t FUNC_IDX_INTERNAL = 1;
const size_t FUNC_IDX_HASH = 2;

static std::string GetExtendedFilePath(const std::string &path)
{
#if defined(PANDA_TARGET_WINDOWS)
    static const std::string PREFIX_FOR_LONG_PATH = "\\\\?\\";

    if (path.length() < _MAX_PATH) {
        return path;
    }

    std::string extendedPath = PREFIX_FOR_LONG_PATH + path;
    std::replace(extendedPath.begin(), extendedPath.end(), '/', '\\');
    return extendedPath;
#else
    return path;
#endif
}

bool SymbolTable::Initialize(int /*targetApiVersion*/, std::string /*targetApiSubVersion*/)
{
    if (!symbolTable_.empty() && !ReadSymbolTable(symbolTable_)) {
        std::cerr << "Failed to read the symbol table file '" << symbolTable_ << "'." << std::endl;
        return false;
    }

    if (!dumpSymbolTable_.empty()) {
        std::fstream fs;
        fs.open(GetExtendedFilePath(dumpSymbolTable_), std::ios_base::out | std::ios_base::trunc);
        if (!fs.is_open()) {
            std::cerr << "Failed to create or open the output symbol table file '" << dumpSymbolTable_
                      << "' during symbol table initialization." << std::endl;
            std::cerr << "This error could be due to invalid file path, lack of write permissions, "
                      << "or the file being in use by another process." << std::endl;
            return false;
        }
        fs.close();
    }

    return true;
}

bool SymbolTable::ParseSymbolTableLine(const std::string &line)
{
    auto items = GetStringItems(line, SECOND_LEVEL_SEPERATOR);
    if (items.size() >= FUNC_FIELD_COUNT) {
        OriginFunctionInfo info;
        info.recordName = items[FUNC_IDX_NAME].substr(0, items[FUNC_IDX_NAME].find_last_of("."));
        info.funcInternalName = items[FUNC_IDX_INTERNAL];
        info.funcHash = items[FUNC_IDX_HASH];
        auto [it, inserted] =
            originFunctionInfo_.insert(std::pair<std::string, OriginFunctionInfo>(info.funcInternalName, info));
        if (!inserted) {
            std::cerr << "Warning: duplicate function entry '" << info.funcInternalName
                      << "' in symbol table, using first occurrence." << std::endl;
        }
        return true;
    }
    if (items.size() == MODULE_FIELD_COUNT) {
        auto [it, inserted] = originModuleInfo_.insert(std::pair<std::string, std::string>(items[0], items[1]));
        if (!inserted) {
            std::cerr << "Warning: duplicate module entry '" << items[0] << "' in symbol table, using first occurrence."
                      << std::endl;
        }
        return true;
    }
    std::cerr << "Failed to read the symbol table line: '" << line << "' due to unrecognized format." << std::endl;
    std::cerr << "Please verify the format of the symbol table." << std::endl;
    return false;
}

bool SymbolTable::ReadSymbolTable(const std::string &symbolTable)
{
    std::ifstream ifs;
    std::string line;
    ifs.open(GetExtendedFilePath(symbolTable));
    if (!ifs.is_open()) {
        std::cerr << "Failed to open the symbol table file '" << symbolTable << "' during symbol table reading."
                  << std::endl;
        std::cerr << "Please check if the file exists, the path is correct, "
                  << "and your program has the necessary permissions to access the file." << std::endl;
        return false;
    }

    bool processedAny = false;
    while (std::getline(ifs, line)) {
        if (!ParseSymbolTableLine(line)) {
            std::cerr << "Failed to read the symbol file due to one or more malformed lines." << std::endl;
            return false;
        }
        processedAny = true;
    }
    if (!processedAny) {
        std::cerr << "Symbol table file '" << symbolTable << "' is empty." << std::endl;
        return false;
    }
    return true;
}

void SymbolTable::FillSymbolTable(const std::stringstream &content)
{
    std::lock_guard<std::mutex> lock(m_);
    symbolTableContent_ << content.rdbuf();
}

void SymbolTable::WriteSymbolTable()
{
    std::fstream fs;
    fs.open(GetExtendedFilePath(dumpSymbolTable_), std::ios_base::app | std::ios_base::in);
    if (fs.is_open()) {
        fs << symbolTableContent_.str();
        fs.close();
    }
}

std::vector<std::string_view> SymbolTable::GetStringItems(std::string_view input, const std::string &separator)
{
    std::vector<std::string_view> items;
    size_t curPos = 0;
    size_t lastPos = 0;

    while ((curPos = input.find(separator, lastPos)) != std::string_view::npos) {
        auto token = input.substr(lastPos, curPos - lastPos);
        if (!token.empty()) {
            items.push_back(token);
        }
        lastPos = curPos + separator.size();
    }

    auto tail = input.substr(lastPos);
    if (!tail.empty()) {
        items.push_back(tail);
    }

    return items;
}

}  // namespace ark::es2panda::util
