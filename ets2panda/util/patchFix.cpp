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

    if (IsColdReload() || IsHotReload()) {
        HandleFunction(cg, func);
        return;
    }
}

void PatchFix::ProcessModule(const parser::Program *program)
{
    if (IsDumpSymbolTable()) {
        DumpModuleInfo(program);
        return;
    }

    if (IsColdReload()) {
        ValidateModuleInfo(program);
        return;
    }

    if (IsHotReload()) {
        return;
    }
}

void PatchFix::ProcessExports(const parser::Program *program, const std::vector<std::string> &exportedNames)
{
    std::stringstream info;
    for (const auto &name : exportedNames) {
        info << name << SymbolTable::SECOND_LEVEL_SEPERATOR;
    }
    std::string hash = Helpers::GetHashString(info.str());
    std::string key = GetModuleKey(program) + ":exports";

    if (IsDumpSymbolTable()) {
        std::stringstream ss;
        ss << key << SymbolTable::SECOND_LEVEL_SEPERATOR << hash << std::endl;
        symbolTable_->FillSymbolTable(ss);
        return;
    }

    if (IsColdReload()) {
        auto it = originModuleInfo_->find(key);
        if (it == originModuleInfo_->end() && exportedNames.empty()) {
            return;  // No exports in either version.
        }
        if (it != originModuleInfo_->end() && it->second == hash) {
            return;  // Export set unchanged.
        }
        patchError_ = true;
        errMsg_ << "[Patch] Found export expression changed in " << key << ", not supported!" << std::endl;
        return;
    }
}

void PatchFix::ProcessClassInfo(const parser::Program *program,
                                const std::vector<std::tuple<std::string, std::string, std::string>> &classInfos)
{
    std::stringstream info;
    for (const auto &[name, parent, ifaces] : classInfos) {
        info << name << SymbolTable::SECOND_LEVEL_SEPERATOR << parent << SymbolTable::SECOND_LEVEL_SEPERATOR << ifaces
             << SymbolTable::FIRST_LEVEL_SEPERATOR;
    }
    std::string hash = Helpers::GetHashString(info.str());
    std::string key = GetModuleKey(program) + ":classinfo";

    if (IsDumpSymbolTable()) {
        std::stringstream ss;
        ss << key << SymbolTable::SECOND_LEVEL_SEPERATOR << hash << std::endl;
        symbolTable_->FillSymbolTable(ss);
        return;
    }

    if (IsColdReload()) {
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

void PatchFix::DumpModuleInfo(const parser::Program *program)
{
    std::stringstream ss;
    ss << GetModuleKey(program) << SymbolTable::SECOND_LEVEL_SEPERATOR;
    ss << ComputeModuleHash(program) << std::endl;
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

void PatchFix::ValidateModuleInfo(const parser::Program *program)
{
    std::string key = GetModuleKey(program);
    auto it = originModuleInfo_->find(key);
    if (it == originModuleInfo_->end()) {
        patchError_ = true;
        errMsg_ << "[Patch] Found new import/export expression in " << key << ", not supported!" << std::endl;
        return;
    }

    std::string currentHash = ComputeModuleHash(program);
    if (currentHash != it->second) {
        patchError_ = true;
        errMsg_ << "[Patch] Found import/export expression changed in " << key << ", not supported!" << std::endl;
    }
}

// ============================================================================
// Module helpers
// ============================================================================

std::string PatchFix::GetModuleKey(const parser::Program *program)
{
    return std::string(program->GetImportInfo().Key());
}

static const ir::ImportDeclaration *GetImportDecl(const ir::Statement *stmt)
{
    return (stmt->Type() == ir::AstNodeType::ETS_IMPORT_DECLARATION)
               ? static_cast<const ir::ImportDeclaration *>(stmt->AsETSImportDeclaration())
               : stmt->AsImportDeclaration();
}

static void CollectImportSpecifiers(const ir::ImportDeclaration *importDecl, std::stringstream &info)
{
    if (importDecl->Source() != nullptr) {
        info << "from:" << importDecl->Source()->Str() << ";";
    }
    for (const auto *spec : importDecl->Specifiers()) {
        if (spec->IsImportSpecifier()) {
            info << "i:" << spec->AsImportSpecifier()->Imported()->Name() << ":"
                 << spec->AsImportSpecifier()->Local()->Name() << ";";
        } else if (spec->IsImportDefaultSpecifier()) {
            info << "id:" << spec->AsImportDefaultSpecifier()->Local()->Name() << ";";
        } else if (spec->IsImportNamespaceSpecifier()) {
            info << "ins:" << spec->AsImportNamespaceSpecifier()->Local()->Name() << ";";
        }
    }
}

static void CollectExportSpecifiers(const ir::ExportNamedDeclaration *exportDecl, std::stringstream &info)
{
    for (const auto *spec : exportDecl->Specifiers()) {
        info << "e:" << spec->Local()->Name();
        if (spec->Exported() != nullptr && spec->Exported()->Name() != spec->Local()->Name()) {
            info << ":" << spec->Exported()->Name();
        }
        info << ";";
    }
    if (exportDecl->Decl() != nullptr && exportDecl->Decl()->IsFunctionDeclaration()) {
        info << "ex:" << exportDecl->Decl()->AsFunctionDeclaration()->Function()->Id()->Name() << ";";
    }
}

static void CollectExportAll(const ir::ExportAllDeclaration *exportAll, std::stringstream &info)
{
    if (exportAll->Source() != nullptr) {
        info << "ea:" << exportAll->Source()->Str();
        if (exportAll->Exported() != nullptr) {
            info << ":" << exportAll->Exported()->Name();
        }
        info << ";";
    }
}

static void CollectReExport(const ir::ETSReExportDeclaration *reExport, std::stringstream &info)
{
    auto *etsImport = reExport->GetETSImportDeclarations();
    if (etsImport->Source() != nullptr) {
        info << "refrom:" << etsImport->Source()->Str() << ";";
    }
    for (const auto *spec : etsImport->Specifiers()) {
        if (spec->IsImportSpecifier()) {
            auto *importSpec = spec->AsImportSpecifier();
            info << "re:" << importSpec->Imported()->Name();
            if (importSpec->Local()->Name() != importSpec->Imported()->Name()) {
                info << ":" << importSpec->Local()->Name();
            }
            info << ";";
        } else if (spec->IsImportNamespaceSpecifier()) {
            info << "rens:" << spec->AsImportNamespaceSpecifier()->Local()->Name() << ";";
        }
    }
}

static void CollectStatementInfo(const ir::Statement *stmt, std::stringstream &info)
{
    switch (stmt->Type()) {
        case ir::AstNodeType::ETS_IMPORT_DECLARATION:
        case ir::AstNodeType::IMPORT_DECLARATION:
            CollectImportSpecifiers(GetImportDecl(stmt), info);
            break;
        case ir::AstNodeType::EXPORT_NAMED_DECLARATION:
            CollectExportSpecifiers(stmt->AsExportNamedDeclaration(), info);
            break;
        case ir::AstNodeType::EXPORT_ALL_DECLARATION:
            CollectExportAll(stmt->AsExportAllDeclaration(), info);
            break;
        case ir::AstNodeType::REEXPORT_STATEMENT:
            CollectReExport(stmt->AsETSReExportDeclaration(), info);
            break;
        default:
            break;
    }
}

static void CollectImportExportInfo(const parser::Program *program, std::stringstream &info)
{
    for (const auto *stmt : program->Ast()->Statements()) {
        CollectStatementInfo(stmt, info);
    }
}

std::string PatchFix::ComputeModuleHash(const parser::Program *program)
{
    std::stringstream info;
    info << program->GetImportInfo().ModuleName() << ";";

    const auto *externalDecls = program->GetExternalDecls();
    if (externalDecls != nullptr) {
        // Direct() returns an unordered_map whose iteration order is non-deterministic across
        // processes. Sort the module names so the hash is stable regardless of iteration order.
        std::vector<std::string> extModuleNames;
        extModuleNames.reserve(externalDecls->Direct().size());
        for (const auto &[key, extProg] : externalDecls->Direct()) {
            extModuleNames.emplace_back(extProg->ModuleName());
        }
        std::sort(extModuleNames.begin(), extModuleNames.end());
        for (const auto &name : extModuleNames) {
            info << name << ";";
        }
    }

    CollectImportExportInfo(program, info);
    return Helpers::GetHashString(info.str());
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

    if (IsColdReload()) {
        DetectSignatureChanges();
        return;
    }

    if (IsHotReload()) {
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
