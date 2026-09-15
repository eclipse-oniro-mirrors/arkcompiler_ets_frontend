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

#include "patchFix.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "compiler/core/codeGen.h"
#include "ir/astNode.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "ir/module/exportAllDeclaration.h"
#include "ir/module/exportNamedDeclaration.h"
#include "ir/module/importDeclaration.h"
#include "ir/module/importSpecifier.h"
#include "ir/statements/blockStatement.h"
#include "parser/program/program.h"
#include "util/helpers.h"
#include "util/importPathManager.h"

namespace ark::es2panda::util {

// Serialized per-class structural list stored under ':classinfolist': each class is
// "name\x1fparent\x1fifaces" and classes are joined with '\x1e'. '-' is the sentinel
// for an empty list.
static constexpr std::string_view CLASS_INFO_FIELD_SEP = "\x1f";
static constexpr std::string_view CLASS_INFO_ENTRY_SEP = "\x1e";
static constexpr std::string_view CLASS_INFO_SENTINEL = "-";

// Prefix of compiler-synthesized lambda entities (methods and classes). Their
// per-file ordinal is assigned in traversal order, so inserting a lambda renumbers
// the subsequent ones — semantically "delete + add", an allowed change — so they
// are excluded from the structural checks; the runtime stays authoritative.
static constexpr std::string_view LAMBDA_INVOKE_PREFIX = "lambda_invoke-";

// ============================================================================
// Constructor
// ============================================================================

PatchFix::PatchFix(PatchFixKind kind, const std::string &recordName, std::unique_ptr<SymbolTable> symbolTable)
    : patchFixKind_(kind), symbolTable_(std::move(symbolTable)), recordName_(recordName)
{
    if (symbolTable_ != nullptr) {
        originFunctionInfo_ = symbolTable_->GetOriginFunctionInfo();
        originModuleInfo_ = symbolTable_->GetOriginModuleInfo();
    }
}

// ============================================================================
// Public entry points (called from compilation pipeline)
// ============================================================================

void PatchFix::ProcessFunction(compiler::CodeGen *cg, pandasm::Function *func)
{
    if (IsDumpSymbolTable()) {
        DumpFunctionInfo(cg, func);
        return;
    }
    if (IsHotReload()) {
        HandleFunction(cg, func);
        return;
    }
}

std::string PatchFix::GetModuleKey(const parser::Program *program)
{
    return std::string(program->GetImportInfo().Key());
}

static bool IsSyntheticLambdaName(std::string_view name)
{
    return name.find(LAMBDA_INVOKE_PREFIX) != std::string_view::npos;
}

static std::string SerializeClassInfoList(
    const std::vector<std::tuple<std::string, std::string, std::string>> &classInfos)
{
    if (classInfos.empty()) {
        return std::string(CLASS_INFO_SENTINEL);
    }
    // Sort by class name so the serialized form is order-stable.
    auto sorted = classInfos;
    std::sort(sorted.begin(), sorted.end(),
              [](const auto &lhs, const auto &rhs) { return std::get<0>(lhs) < std::get<0>(rhs); });
    std::stringstream ss;
    bool first = true;
    for (const auto &[name, parent, ifaces] : sorted) {
        if (!first) {
            ss << CLASS_INFO_ENTRY_SEP;
        }
        ss << name << CLASS_INFO_FIELD_SEP << parent << CLASS_INFO_FIELD_SEP << ifaces;
        first = false;
    }
    return ss.str();
}

// The implemented-interface list is a set semantically: "implements IA, IB" and
// "implements IB, IA" describe the same class structure (the runtime swap check
// compares interfaces as an unordered set too). Compares the semicolon-joined
// lists as a hash set of string_view tokens — no copies, no sorting.
static bool IfaceListsEqual(const std::string &originJoined, const std::string &currentJoined)
{
    std::unordered_set<std::string_view> originIfaces;
    for (size_t pos = 0; pos < originJoined.size();) {
        auto end = originJoined.find(';', pos);
        if (end == std::string::npos) {
            end = originJoined.size();
        }
        auto item = std::string_view(originJoined).substr(pos, end - pos);
        if (!item.empty()) {
            originIfaces.insert(item);
        }
        pos = end + 1;
    }
    for (size_t pos = 0; pos < currentJoined.size();) {
        auto end = currentJoined.find(';', pos);
        if (end == std::string::npos) {
            end = currentJoined.size();
        }
        auto item = std::string_view(currentJoined).substr(pos, end - pos);
        if (!item.empty() && originIfaces.erase(item) == 0) {
            return false;  // current implements an interface absent from the origin list
        }
        pos = end + 1;
    }
    return originIfaces.empty();  // every origin interface was matched
}

static bool ClassStructureChanged(const std::string &originSerialized,
                                  const std::vector<std::tuple<std::string, std::string, std::string>> &current)
{
    // Judge per class: an origin class whose parent/interfaces changed is rejected; newly added classes are allowed.
    // Removed classes are allowed as the deletion counterpart: deleting a function is an allowed change and may drop
    // its synthetic lambda classes; a non-exported class can only be referenced from within its own module, and the
    // module is recompiled as a whole here.
    std::unordered_map<std::string, std::pair<std::string, std::string>> originByName;
    size_t lastPos = 0;
    while (lastPos < originSerialized.size()) {
        auto entryEnd = originSerialized.find(CLASS_INFO_ENTRY_SEP, lastPos);
        if (entryEnd == std::string::npos) {
            entryEnd = originSerialized.size();
        }
        auto entry = std::string_view(originSerialized).substr(lastPos, entryEnd - lastPos);
        auto f1 = entry.find(CLASS_INFO_FIELD_SEP);
        auto f2 = entry.find(CLASS_INFO_FIELD_SEP, f1 + 1);
        if (f1 != std::string_view::npos && f2 != std::string_view::npos) {
            originByName.emplace(
                std::string(entry.substr(0, f1)),
                std::make_pair(std::string(entry.substr(f1 + 1, f2 - f1 - 1)), std::string(entry.substr(f2 + 1))));
        } else {
            std::cerr << "[Patch] Warning: malformed class info entry skipped: " << entry << std::endl;
        }
        lastPos = entryEnd + 1;
    }
    for (const auto &[name, parent, ifaces] : current) {
        auto it = originByName.find(name);
        if (it == originByName.end()) {
            continue;  // Newly added class — allowed.
        }
        if (IsSyntheticLambdaName(name)) {
            continue;  // Renumbered lambda class — old deleted + new added, allowed.
        }
        if (it->second.first != parent || !IfaceListsEqual(it->second.second, ifaces)) {
            return true;  // Existing class restructured — rejected.
        }
    }
    return false;
}

void PatchFix::ProcessClassInfo(const parser::Program *program,
                                const std::vector<std::tuple<std::string, std::string, std::string>> &classInfos)
{
    // Skip the synthetic '<simult>' shell.
    if (program->Is<ModuleKind::SIMULT_MAIN>()) {
        return;
    }

    std::stringstream info;
    for (const auto &[name, parent, ifaces] : classInfos) {
        info << name << SymbolTable::SECOND_LEVEL_SEPERATOR << parent << SymbolTable::SECOND_LEVEL_SEPERATOR << ifaces
             << SymbolTable::FIRST_LEVEL_SEPERATOR;
    }
    std::string hash = Helpers::GetHashString(info.str());
    std::string key = GetModuleKey(program) + ":classinfo";
    std::string listKey = GetModuleKey(program) + ":classinfolist";

    if (IsDumpSymbolTable()) {
        std::stringstream ss;
        ss << key << SymbolTable::SECOND_LEVEL_SEPERATOR << hash << std::endl;
        symbolTable_->FillSymbolTable(ss);
        std::stringstream ss2;
        ss2 << listKey << SymbolTable::SECOND_LEVEL_SEPERATOR << SerializeClassInfoList(classInfos) << std::endl;
        symbolTable_->FillSymbolTable(ss2);
        return;
    }

    if (IsHotReload()) {
        auto listIt = originModuleInfo_->find(listKey);
        if (listIt != originModuleInfo_->end()) {
            // New-format symbol table: judge per class. Allowed: unchanged structures
            // and newly added classes (e.g. lambda classes of an added function).
            // Rejected: an existing class whose parent or implemented interfaces changed.
            if (!ClassStructureChanged(listIt->second, classInfos)) {
                return;
            }
            patchError_ = true;
            errMsg_ << "[Patch] Found class inheritance or interface change in " << key << ", not supported!"
                    << std::endl;
            return;
        }
        // Old-format symbol table (hash only): strict comparison.
        auto it = originModuleInfo_->find(key);
        if (it == originModuleInfo_->end()) {
            return;  // Origin was dumped without class info support — skip.
        }
        if (it->second == hash) {
            return;
        }
        patchError_ = true;
        errMsg_ << "[Patch] Found class inheritance or interface change in " << key << ", not supported!" << std::endl;
        return;
    }
}

// ============================================================================
// Dump mode (Phase 1): write symbol table entries
// ============================================================================

void PatchFix::DumpFunctionInfo(compiler::CodeGen *cg, pandasm::Function *func)
{
    std::stringstream ss;

    // InternalName() returns "record.func:retType;" — strip trailing ';' so
    // it doesn't become part of the stored key. HandleFunction does the same.
    auto internalName = cg->InternalName();
    std::string name(internalName.Mutf8());
    if (!name.empty() && name.back() == ';') {
        name.pop_back();
    }
    ss << name;
    ss << SymbolTable::SECOND_LEVEL_SEPERATOR;
    ss << name;
    ss << SymbolTable::SECOND_LEVEL_SEPERATOR;

    ss << GenerateFunctionHash(func);
    ss << std::endl;

    symbolTable_->FillSymbolTable(ss);
}

// ============================================================================
// ColdReload mode (Phase 2): validate against origin symbol table
// ============================================================================

void PatchFix::HandleFunction(compiler::CodeGen *cg, pandasm::Function *func)
{
    std::string key = cg->InternalName().Mutf8();
    if (!key.empty() && key.back() == ';') {
        key.pop_back();
    }
    auto originFunction = originFunctionInfo_->find(key);
    if (originFunction == originFunctionInfo_->end()) {
        // Record new function base names for Finalize signature-change detection.
        auto colonPos = key.find(':');
        newFunctionBaseNames_.insert((colonPos != std::string::npos) ? key.substr(0, colonPos) : key);
        return;
    }

    // Track matched origin functions for Finalize deletion/signature-change detection.
    matchedFunctions_.insert(key);

    auto &bytecodeInfo = originFunction->second;
    auto funcHash = GenerateFunctionHash(func);
    // Hot Reload allows function body changes — the runtime handles hot-patching.
    if (IsHotReload()) {
        return;
    }
    // Cold Reload: function hash must match the origin symbol table.
    if (funcHash != bytecodeInfo.funcHash) {
        patchError_ = true;
        errMsg_ << "[Patch] Function '" << key << "' hash mismatch with origin symbol table\n";
    }
}

void PatchFix::DetectSignatureChanges()
{
    // Distinguish signature changes from deletions:
    for (const auto &[key, info] : *originFunctionInfo_) {
        if (matchedFunctions_.find(key) != matchedFunctions_.end()) {
            continue;
        }
        auto colonPos = key.find(':');
        std::string baseName = (colonPos != std::string::npos) ? key.substr(0, colonPos) : key;
        if (IsSyntheticLambdaName(baseName)) {
            continue;  // Renumbered lambda — old deleted + new added, allowed.
        }
        if (newFunctionBaseNames_.find(baseName) != newFunctionBaseNames_.end()) {
            patchError_ = true;
            errMsg_ << "[Patch] Function '" << key << "' signature changed — not supported!\n";
        }
    }
}

void PatchFix::Finalize(pandasm::Program ** /*prog*/)
{
    if (IsDumpSymbolTable()) {
        return;
    }

    if (IsHotReload()) {
        DetectSignatureChanges();
        return;
    }
}

std::string PatchFix::GenerateFunctionHash(pandasm::Function *func)
{
    std::stringstream ss;

    ss << ".function any " << func->name << '(';
    for (uint32_t i = 0; i < func->GetParamsNum(); i++) {
        ss << "any a" << std::to_string(i);
        if (i != func->GetParamsNum() - 1) {
            ss << ", ";
        }
    }
    ss << ") {" << std::endl;

    for (const auto &ins : func->ins) {
        ss << (ins.HasLabel() ? "" : "\t") << ins.ToString("", true, func->GetTotalRegs()) << "  ";
    }
    ss << "}" << std::endl;

    for (const auto &ct : func->catchBlocks) {
        ss << ".catchall " << ct.tryBeginLabel << ", " << ct.tryEndLabel << ", " << ct.catchBeginLabel << std::endl;
    }

    return Helpers::GetHashString(ss.str());
}

bool PatchFix::IsDumpSymbolTable() const
{
    return patchFixKind_ == PatchFixKind::DUMPSYMBOLTABLE;
}

bool PatchFix::IsHotFix() const
{
    return patchFixKind_ == PatchFixKind::HOTFIX;
}

bool PatchFix::IsColdFix() const
{
    return patchFixKind_ == PatchFixKind::COLDFIX;
}

bool PatchFix::IsHotReload() const
{
    return patchFixKind_ == PatchFixKind::HOTRELOAD;
}

bool PatchFix::IsColdReload() const
{
    return patchFixKind_ == PatchFixKind::COLDRELOAD;
}

bool ValidateReloadOptions(const Options &options)
{
    bool isReload = options.IsColdReload() || options.IsHotReload();
    if (options.IsColdReload() && options.IsHotReload()) {
        std::cerr << "[Error] --cold-reload and --hot-reload are mutually exclusive" << std::endl;
        return false;
    }
    if (!options.GetDumpSymbolTable().empty() && isReload) {
        std::cerr << "[Error] --dump-symbol-table and reload modes are mutually exclusive" << std::endl;
        return false;
    }
    if (isReload && options.GetInputSymbolTable().empty()) {
        std::cerr << "[Error] --cold-reload/--hot-reload requires --input-symbol-table" << std::endl;
        return false;
    }
    return true;
}

std::pair<bool, std::unique_ptr<PatchFix>> InitPatchFix(const Options &options, const std::string &sourcePath)
{
    if (!options.IsColdReload() && !options.IsHotReload() && options.GetDumpSymbolTable().empty()) {
        return {true, nullptr};
    }
    if (!ValidateReloadOptions(options)) {
        return {false, nullptr};
    }

    // Derive record name from source path (strip directory and extension).
    std::string recordName(sourcePath);
    auto lastSlash = recordName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        recordName = recordName.substr(lastSlash + 1);
    }
    auto lastDot = recordName.find_last_of('.');
    if (lastDot != std::string::npos) {
        recordName = recordName.substr(0, lastDot);
    }

    auto symbolTable = std::make_unique<SymbolTable>(options.GetInputSymbolTable(), options.GetDumpSymbolTable());
    if (!symbolTable->Initialize(0, "")) {
        std::cerr << "[Error] Failed to initialize symbol table" << std::endl;
        return {false, nullptr};
    }

    auto kind = options.IsColdReload()  ? PatchFixKind::COLDRELOAD
                : options.IsHotReload() ? PatchFixKind::HOTRELOAD
                                        : PatchFixKind::DUMPSYMBOLTABLE;
    return {true, std::make_unique<PatchFix>(kind, recordName, std::move(symbolTable))};
}

bool FinalizePatchFix(PatchFix &patchFix, std::string *outErrorMsg)
{
    patchFix.Finalize(nullptr);
    if (patchFix.IsDumpSymbolTable()) {
        auto *st = patchFix.GetSymbolTable();
        if (st != nullptr) {
            st->WriteSymbolTable();
        }
    }
    bool ok = !patchFix.HasError();
    if (!ok) {
        std::cerr << patchFix.GetErrorMessage();
        if (outErrorMsg != nullptr) {
            *outErrorMsg = patchFix.GetErrorMessage();
        }
    }
    return ok;
}

}  // namespace ark::es2panda::util
