/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "declgenEts2Ts.h"
#include <cstdint>

#include "checker/ETSchecker.h"
#include "isolatedDeclgenChecker.h"
#include "checker/types/ets/etsTupleType.h"
#include "compiler/lowering/phase.h"
#include "generated/diagnostic.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "ir/ets/etsTuple.h"
#include "ir/ets/etsUnionType.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/statements/classDeclaration.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "ir/ts/tsTypeParameter.h"
#include "compiler/lowering/util.h"
#include "parser/program/program.h"
#include "public/public.h"

#define DEBUG_PRINT 0

namespace ark::es2panda::declgen_ets2ts {

namespace {

constexpr std::string_view TS_DECL_SUFFIX = ".d.ts";

[[nodiscard]] bool IsExplicitVoidTypeNode(const ir::TypeNode *typeAnnotation)
{
    if (typeAnnotation == nullptr) {
        return false;
    }

    if (typeAnnotation->IsTSVoidKeyword()) {
        return true;
    }

    if (typeAnnotation->IsETSPrimitiveType() &&
        typeAnnotation->AsETSPrimitiveType()->GetPrimitiveType() == ir::PrimitiveType::VOID) {
        return true;
    }

    if (typeAnnotation->IsTSParenthesizedType()) {
        const auto *innerType = typeAnnotation->AsTSParenthesizedType()->Type();
        return innerType != nullptr && innerType->IsTypeNode() && IsExplicitVoidTypeNode(innerType->AsTypeNode());
    }

    return false;
}

[[nodiscard]] bool HasExplicitVoidAnnotation(const ir::TypeNode *typeAnnotation)
{
    std::unordered_set<const ir::TypeNode *> visited;
    auto *current = typeAnnotation;
    while (current != nullptr) {
        if (!visited.insert(current).second) {
            return false;
        }

        if (IsExplicitVoidTypeNode(current)) {
            return true;
        }

        auto *originalNode = current->OriginalNode();
        if (originalNode == nullptr || !originalNode->IsExpression() || !originalNode->AsExpression()->IsTypeNode()) {
            return false;
        }

        auto *next = originalNode->AsExpression()->AsTypeNode();
        current = next;
    }

    return false;
}

[[nodiscard]] bool IsTypeScriptDeclarationOutput(const DeclgenOptions &options)
{
    const auto &outputPath = options.outputDeclEts;
    return outputPath.size() >= TS_DECL_SUFFIX.size() &&
           outputPath.compare(outputPath.size() - TS_DECL_SUFFIX.size(), TS_DECL_SUFFIX.size(), TS_DECL_SUFFIX) == 0;
}

[[nodiscard]] std::string ExportSpecifierName(const ir::Identifier *identifier)
{
    ES2PANDA_ASSERT(identifier != nullptr);
    const auto name = identifier->Name();
    return name.Is(compiler::Signatures::REEXPORT_DEFAULT_ANONYMOUSLY) ? std::string(compiler::Signatures::DEFAULT)
                                                                       : name.Mutf8();
}

[[nodiscard]] bool IsInternalDefaultExportSpecifier(const ir::ExportSpecifier *specifier)
{
    return specifier->Local()->Name().Is(compiler::Signatures::REEXPORT_DEFAULT_ANONYMOUSLY) ||
           specifier->Exported()->Name().Is(compiler::Signatures::REEXPORT_DEFAULT_ANONYMOUSLY);
}

}  // namespace

// Jsdoc parser
namespace jsdoc {

inline constexpr const char *NONINTEROP_COMMAND = "noninterop";
inline constexpr const char *INTEROP_COMMAND = "interop";
inline constexpr const char *INTEROP_SUBCOMMAND_ANY = "any";
inline constexpr const char *INTEROP_SUBCOMMAND_RET = "ret";
inline constexpr const char *INTEROP_SUBCOMMAND_PARAM = "param";
inline constexpr const char *INTEROP_SUBCOMMAND_BREAK_EXTENDS = "break-extends";
inline constexpr const char *INTEROP_SUBCOMMAND_BREAK_IMPLEMENTS = "break-implements";

struct JsdocTag {
    std::string tag;      // tag name without leading '@'; empty for the description block
    std::string comment;  // tag body / description text; lines joined with '\n'
};

bool IsJsdocSpace(char ch)
{
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

std::string_view TrimLeft(std::string_view s)
{
    while (!s.empty() && IsJsdocSpace(s.front())) {
        s.remove_prefix(1);
    }
    return s;
}

std::string_view TrimRight(std::string_view s)
{
    while (!s.empty() && IsJsdocSpace(s.back())) {
        s.remove_suffix(1);
    }
    return s;
}

// Strip per-line decoration: leading whitespace and an optional `* ` continuation marker.
std::string_view StripLinePrefix(std::string_view line)
{
    line = TrimLeft(line);
    if (!line.empty() && line.front() == '*') {
        line.remove_prefix(1);
        if (!line.empty() && line.front() == ' ') {
            line.remove_prefix(1);
        }
    }
    return TrimRight(line);
}

// Pull the next physical line (LF-terminated, CR tolerated) out of `source`,
// advancing `cursor` past the consumed newline.
std::string_view NextLine(std::string_view source, std::size_t &cursor)
{
    auto eol = source.find('\n', cursor);
    std::size_t end = (eol == std::string_view::npos) ? source.size() : eol;
    auto line = source.substr(cursor, end - cursor);
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }
    cursor = (eol == std::string_view::npos) ? source.size() : eol + 1;
    return line;
}

// Locate each `/** ... */` block inside the raw jsdoc text. The frontend usually
// returns a single block, but be defensive against concatenated runs.
std::vector<std::string_view> ExtractBlocks(std::string_view source)
{
    constexpr std::string_view OPEN = "/**";
    constexpr std::string_view CLOSE = "*/";
    std::vector<std::string_view> blocks;
    std::size_t pos = 0;
    while (pos < source.size()) {
        auto open = source.find(OPEN, pos);
        if (open == std::string_view::npos) {
            break;
        }
        auto close = source.find(CLOSE, open + OPEN.size());
        if (close == std::string_view::npos) {
            break;
        }
        // Keep content only, without delimiters.
        blocks.emplace_back(source.substr(open + OPEN.size(), close - (open + OPEN.size())));
        pos = close + CLOSE.size();
    }
    if (blocks.empty()) {
        blocks.emplace_back(source);
    }
    return blocks;
}

// Split "@name remainder" into (name, remainder). `name` is the longest run of
// non-whitespace characters following '@'.
void SplitTagHeader(std::string_view line, std::string &name, std::string_view &rest)
{
    line.remove_prefix(1);  // drop '@'
    auto ws = line.find_first_of(" \t");
    if (ws == std::string_view::npos) {
        name.assign(line.data(), line.size());
        rest = {};
        return;
    }
    name.assign(line.data(), ws);
    rest = TrimLeft(line.substr(ws + 1));
}

void AppendCommentLine(std::string &comment, std::string_view line)
{
    if (!comment.empty()) {
        comment.push_back('\n');
    }
    comment.append(line.data(), line.size());
}

std::vector<JsdocTag> ParseJsdoc(std::string_view raw)
{
    std::vector<JsdocTag> result;
    if (raw.empty()) {
        return result;
    }

    JsdocTag current {};  // description bucket: tag == ""
    bool hasCurrent = false;

    auto flush = [&hasCurrent, &current, &result]() {
        if (!hasCurrent) {
            return;
        }
        auto trimmed = TrimRight(std::string_view(current.comment));
        current.comment.assign(trimmed.data(), trimmed.size());
        if (!current.tag.empty() || !current.comment.empty()) {
            result.push_back(std::move(current));
        }
        current = JsdocTag {};
        hasCurrent = false;
    };

    for (auto block : ExtractBlocks(raw)) {
        std::size_t cursor = 0;
        while (cursor <= block.size()) {
            if (cursor == block.size()) {
                // Consume a virtual final empty line so trailing content is flushed naturally.
                break;
            }
            auto line = StripLinePrefix(NextLine(block, cursor));
            if (!line.empty() && line.front() == '@') {
                flush();
                std::string_view rest;
                SplitTagHeader(line, current.tag, rest);
                current.comment.assign(rest.data(), rest.size());
                hasCurrent = true;
            } else {
                hasCurrent = true;
                AppendCommentLine(current.comment, line);
            }
        }
        flush();
    }
    return result;
}

struct InteropTags {
    bool noninterop = false;
    bool any = false;
    bool breakExtends = false;
    bool breakImplements = false;
    std::string retOverride;
    std::unordered_map<std::size_t, std::string> paramOverrides;
};

std::vector<std::string_view> SplitTokens(std::string_view text)
{
    std::vector<std::string_view> out;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && IsJsdocSpace(text[i])) {
            ++i;
        }
        std::size_t start = i;
        while (i < text.size() && !IsJsdocSpace(text[i])) {
            ++i;
        }
        if (start != i) {
            out.emplace_back(text.substr(start, i - start));
        }
    }
    return out;
}

std::string JoinFrom(const std::vector<std::string_view> &tokens, std::size_t start)
{
    std::string joined;
    for (std::size_t i = start; i < tokens.size(); ++i) {
        if (i > start) {
            joined.push_back(' ');
        }
        joined.append(tokens[i].data(), tokens[i].size());
    }
    return joined;
}

bool TryParseIndex(std::string_view s, std::size_t &out)
{
    if (s.empty()) {
        return false;
    }
    std::size_t v = 0;
    constexpr uint32_t decimalBase = 10;
    for (char c : s) {
        if (c < '0' || c > '9') {
            return false;
        }
        v = v * decimalBase + static_cast<std::size_t>(c - '0');
    }
    out = v;
    return true;
}

InteropTags CollectInteropTags(std::string_view raw)
{
    // Token layout for `@interop` directives:
    //   ret   <type-tokens...>       -> [sub, type...]
    //   param <index> <type-tokens...>-> [sub, index, type...]
    constexpr std::size_t retMinTokens = 2;    // sub + at least one type token
    constexpr std::size_t paramMinTokens = 3;  // sub + index + at least one type token
    constexpr std::size_t retTypeStart = 1;    // type tokens start after sub
    constexpr std::size_t paramIndexPos = 1;   // index token position
    constexpr std::size_t paramTypeStart = 2;  // type tokens start after index
    InteropTags tags;
    for (const auto &entry : ParseJsdoc(raw)) {
        if (entry.tag == NONINTEROP_COMMAND) {
            tags.noninterop = true;
            continue;
        }
        if (entry.tag != INTEROP_COMMAND) {
            continue;
        }
        const auto tokens = SplitTokens(entry.comment);
        if (tokens.empty()) {
            continue;
        }
        const auto sub = tokens[0];
        if (sub == INTEROP_SUBCOMMAND_ANY) {
            tags.any = true;
        } else if (sub == INTEROP_SUBCOMMAND_BREAK_EXTENDS) {
            tags.breakExtends = true;
        } else if (sub == INTEROP_SUBCOMMAND_BREAK_IMPLEMENTS) {
            tags.breakImplements = true;
        } else if (sub == INTEROP_SUBCOMMAND_RET && tokens.size() >= retMinTokens) {
            tags.retOverride = JoinFrom(tokens, retTypeStart);
        } else if (sub == INTEROP_SUBCOMMAND_PARAM && tokens.size() >= paramMinTokens) {
            std::size_t idx = 0;
            if (TryParseIndex(tokens[paramIndexPos], idx)) {
                tags.paramOverrides[idx] = JoinFrom(tokens, paramTypeStart);
            }
        }
    }
    return tags;
}

InteropTags CollectInteropTagsFromNode(const ir::AstNode *node)
{
    if (node == nullptr) {
        return {};
    }
    auto raw = compiler::JsdocStringFromDeclaration(node);
    return CollectInteropTags(raw.Utf8());
}

}  // namespace jsdoc

static void DebugPrint([[maybe_unused]] const std::string &msg)
{
#if DEBUG_PRINT
    std::cerr << msg << std::endl;
#endif
}

bool TSDeclGen::Generate()
{
    auto ctx = checker_->VarBinder()->GetContext();
    if (ctx->lazyCheck) {
        ctx->lazyCheck = false;
        checker_->StartChecker(ctx->parserProgram->VarBinder(), *ctx->config->options);
        ctx->lazyCheck = true;
    }
    if (!GenGlobalDescriptor()) {
        return false;
    }
    CollectDependencies();
    CollectGlueCodeImportSet();
    GenDeclarations();
    return true;
}

bool TSDeclGen::GenGlobalDescriptor()
{
    if (program_->GlobalClass() == nullptr) {
        const auto loc = lexer::SourcePosition();
        LogError(diagnostic::UNSUPPORTED_ENCODING_SPECIFICATIONS, {}, loc);
        return false;
    }
    globalDesc_ =
        checker::ETSObjectType::NameToDescriptor(program_->GlobalClass()->TsType()->AsETSObjectType()->AssemblerName());
    OutTs("let ETSGLOBAL = (globalThis as any).Panda.getClass('", globalDesc_, "');");
    OutEndlTs();
    OutTs("export {};");
    OutEndlTs();
    return true;
}

void TSDeclGen::CollectGlueCodeImportSet()
{
    for (auto *globalStatement : program_->Ast()->Statements()) {
        if (globalStatement->IsETSImportDeclaration()) {
            auto importDeclaration = globalStatement->AsETSImportDeclaration();
            if (importDeclaration->IsPureDynamic()) {
                return;
            }
            const auto &specifiers = importDeclaration->Specifiers();
            if (specifiers.empty()) {
                return;
            }
            const auto specifierFirst = specifiers[0];
            if (importDeclaration->IsTypeKind()) {
                continue;
            }
            if (specifierFirst->IsImportDefaultSpecifier()) {
                CollectDefaultImport(specifierFirst);
            } else if (specifierFirst->IsImportSpecifier()) {
                CollectNamedImports(specifiers);
            }
        }
    }
}

void TSDeclGen::CollectDefaultImport(const ir::AstNode *specifier)
{
    auto importDefaultSpecifier = specifier->AsImportDefaultSpecifier();
    auto variable = importDefaultSpecifier->Local()->Variable();
    const auto local = importDefaultSpecifier->Local()->Name().Mutf8();
    bool isTypeDeclaration = false;
    if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
        auto *node = variable->Declaration()->Node();
        isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
    }
    if (!isTypeDeclaration) {
        glueCodeImportSet_.insert(local);
    }
}

void TSDeclGen::CollectNamedImports(const ArenaVector<ir::AstNode *> &specifiers)
{
    if (specifiers.empty()) {
        return;
    }
    for (auto *specifier : specifiers) {
        if (!specifier->IsImportSpecifier()) {
            continue;
        }
        auto importSpecifier = specifier->AsImportSpecifier();
        auto variable = importSpecifier->Imported()->Variable();
        bool isTypeDeclaration = false;
        if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
            auto *node = variable->Declaration()->Node();
            isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
        }
        if (!isTypeDeclaration) {
            glueCodeImportSet_.insert(importSpecifier->Local()->Name().Mutf8());
        }
    }
}

void TSDeclGen::CollectDependencies()
{
    for (auto *stmt : program_->Ast()->Statements()) {
        if (stmt->IsExported() || stmt->IsDefaultExported() ||
            (stmt->IsClassDeclaration() && stmt->AsClassDeclaration()->Definition()->IsGlobal())) {
            CollectDependencies(stmt);
        }
        CollectTypeAliasAsDependencies(stmt);
    }
}

void TSDeclGen::CollectDependencies(const ir::AstNode *node)
{
    if (node->IsTSTypeAliasDeclaration()) {
        CollectTypeAliasDependencies(node->AsTSTypeAliasDeclaration());
    } else if (node->IsClassDeclaration()) {
        CollectClassDependencies(node->AsClassDeclaration());
    } else if (node->IsClassDefinition()) {
        CollectClassDependencies(node->AsClassDefinition());
    } else if (node->IsTSInterfaceDeclaration()) {
        CollectInterfaceDependencies(node->AsTSInterfaceDeclaration());
    }
}

void TSDeclGen::CollectTypeAliasDependencies(const ir::TSTypeAliasDeclaration *typeAliasDecl)
{
    CollectTypeAnnotationDependencies(typeAliasDecl->TypeAnnotation());
}

void TSDeclGen::CollectTypeAnnotationDependencies(const ir::TypeNode *typeAnnotation)
{
    if (typeAnnotation->IsETSTypeReference()) {
        CollectETSTypeReferenceDependencies(typeAnnotation->AsETSTypeReference());
    } else if (typeAnnotation->IsETSUnionType()) {
        GenSeparated(
            typeAnnotation->AsETSUnionType()->Types(),
            [this](ir::TypeNode *arg) { CollectTypeAnnotationDependencies(arg); }, "");
    }
}

void TSDeclGen::CollectETSTypeReferenceDependencies(const ir::ETSTypeReference *typeReference)
{
    const auto part = typeReference->Part();
    if (part->TypeParams() != nullptr && part->TypeParams()->IsTSTypeParameterInstantiation()) {
        GenSeparated(
            part->TypeParams()->Params(), [this](ir::TypeNode *param) { CollectTypeAnnotationDependencies(param); },
            "");
    }
    AddDependency(typeReference);
}

void TSDeclGen::CollectClassDependencies(const ir::ClassDeclaration *classDecl)
{
    CollectClassDependencies(classDecl->Definition());
}

void TSDeclGen::CollectClassDependencies(const ir::ClassDefinition *classDef)
{
    if (classDef->Ident()->Name().Mutf8().find('#') != std::string::npos) {
        return;
    }

    AddDependency(classDef->Super());

    if (classDef->TsType() != nullptr && classDef->TsType()->IsETSObjectType()) {
        CollectInterfacesDependencies(classDef->TsType()->AsETSObjectType()->Interfaces());
    }

    if (classDef->TypeParams() != nullptr) {
        GenSeparated(
            classDef->TypeParams()->Params(),
            [this](ir::TSTypeParameter *param) {
                if (param->Constraint() == nullptr) {
                    return;
                }
                AddDependency(param->Constraint());
            },
            "");
    }

    CollectClassPropDependencies(classDef);
}

void TSDeclGen::CollectClassPropDependencies(const ir::ClassDefinition *classDef)
{
    for (const auto *prop : classDef->Body()) {
        if (state_.inNamespace && !prop->IsExported() && !prop->IsExportedType() && !prop->IsDefaultExported()) {
            continue;
        }
        if (prop->IsClassProperty()) {
            auto value = prop->AsClassProperty()->Value();
            if (value != nullptr && value->IsETSNewClassInstanceExpression() &&
                value->AsETSNewClassInstanceExpression()->GetTypeRef() != nullptr &&
                value->AsETSNewClassInstanceExpression()->GetTypeRef()->IsETSTypeReference()) {
                auto typeReference = value->AsETSNewClassInstanceExpression()->GetTypeRef()->AsETSTypeReference();
                CollectETSTypeReferenceDependencies(typeReference);
                continue;
            }
            if (prop->AsClassProperty()->TypeAnnotation() != nullptr) {
                CollectTypeAnnotationDependencies(prop->AsClassProperty()->TypeAnnotation());
                continue;
            }
        } else if (prop->IsMethodDefinition()) {
            CollectClassMethodDependencies(prop->AsMethodDefinition());
        } else if (classDef->IsNamespaceTransformed()) {
            CollectDependencies(prop);
        }
    }
}

void TSDeclGen::CollectClassMethodDependencies(const ir::MethodDefinition *methodDef)
{
    if (methodDef->Function() == nullptr || methodDef->Function()->Signature() == nullptr) {
        return;
    }
    auto sig = methodDef->Function()->Signature();
    GenSeparated(
        sig->Params(), [this](varbinder::LocalVariable *param) { AddDependency(param->TsType()); }, "");

    AddDependency(sig->ReturnType());
}

void TSDeclGen::CollectInterfaceDependencies(const ir::TSInterfaceDeclaration *interfaceDecl)
{
    if (interfaceDecl->Id()->Name().Mutf8().find('#') != std::string::npos) {
        return;
    }

    if (interfaceDecl->TsType() != nullptr && interfaceDecl->TsType()->IsETSObjectType()) {
        CollectInterfacesDependencies(interfaceDecl->TsType()->AsETSObjectType()->Interfaces());
    }

    if (interfaceDecl->TypeParams() != nullptr) {
        GenSeparated(
            interfaceDecl->TypeParams()->Params(),
            [this](ir::TSTypeParameter *param) {
                if (param->Constraint() == nullptr) {
                    return;
                }
                AddDependency(param->Constraint());
            },
            "");
    }

    CollectInterfacePropDependencies(interfaceDecl);
}

void TSDeclGen::CollectInterfacePropDependencies(const ir::TSInterfaceDeclaration *interfaceDecl)
{
    for (const auto *prop : interfaceDecl->Body()->Body()) {
        if (prop->IsMethodDefinition()) {
            CollectInterfaceMethodDependencies(prop->AsMethodDefinition());
        }
    }
}

void TSDeclGen::CollectInterfaceMethodDependencies(const ir::MethodDefinition *methodDef)
{
    if (methodDef->Function() == nullptr || methodDef->Function()->Signature() == nullptr) {
        return;
    }
    auto sig = methodDef->Function()->Signature();
    GenSeparated(
        sig->Params(), [this](varbinder::LocalVariable *param) { AddDependency(param->TsType()); }, "");

    AddDependency(sig->ReturnType());
}

void TSDeclGen::CollectInterfacesDependencies(const ArenaVector<checker::ETSObjectType *> &interfaces)
{
    GenSeparated(
        interfaces,
        [this](checker::ETSObjectType *interface) {
            if (checker::ETSChecker::ETSType(interface) == checker::TypeFlag::ETS_OBJECT) {
                AddDependency(interface);
            }
        },
        "");
}

void TSDeclGen::CollectTypeAliasAsDependencies(const ir::AstNode *node)
{
    if (node->IsTSTypeAliasDeclaration()) {
        const auto parent = node->AsTSTypeAliasDeclaration()->Parent();
        if (parent->IsClassDefinition()) {
            AddDependency(parent->AsClassDefinition()->TsType());
        }
    } else if (node->IsClassDeclaration() && node->AsClassDeclaration()->Definition()->IsNamespaceTransformed()) {
        for (const auto *prop : node->AsClassDeclaration()->Definition()->Body()) {
            CollectTypeAliasAsDependencies(prop);
        }
    }
}

void TSDeclGen::GenDeclarations()
{
    for (auto *globalStatement : program_->Ast()->Statements()) {
        ResetState();
        ResetClassNode();
        if (jsdoc::CollectInteropTagsFromNode(globalStatement).noninterop) {
            continue;
        }
        if (globalStatement->IsClassDeclaration()) {
            GenClassDeclaration(globalStatement->AsClassDeclaration());
        } else if (globalStatement->IsTSInterfaceDeclaration()) {
            GenInterfaceDeclaration(globalStatement->AsTSInterfaceDeclaration());
        } else if (globalStatement->IsTSTypeAliasDeclaration()) {
            GenTypeAliasDeclaration(globalStatement->AsTSTypeAliasDeclaration());
        } else if (globalStatement->IsETSReExportDeclaration()) {
            GenReExportDeclaration(globalStatement->AsETSReExportDeclaration());
        }
    }
}

void TSDeclGen::GenExportNamedDeclarations()
{
    for (auto *globalStatement : program_->Ast()->Statements()) {
        if (globalStatement->IsExportNamedDeclaration()) {
            GenExportNamedDeclaration(globalStatement->AsExportNamedDeclaration());
        }
    }
}

void TSDeclGen::GenInitModuleGlueCode()
{
    for (auto *stmt : program_->Ast()->Statements()) {
        if (!stmt->IsExpressionStatement()) {
            continue;
        }
        if (!stmt->AsExpressionStatement()->GetExpression()->IsCallExpression()) {
            continue;
        }
        auto *callExpr = stmt->AsExpressionStatement()->GetExpression()->AsCallExpression();
        if (callExpr->Callee()->IsIdentifier() &&
            callExpr->Callee()->AsIdentifier()->Name() == compiler::Signatures::INIT_MODULE_METHOD) {
            OutTs("import \"", callExpr->Arguments()[0]->ToString(), "\"");
            OutEndlTs();
        }
    }
}

bool TSDeclGen::IsInteropImport(const ir::ETSImportDeclaration *importDeclaration)
{
    const auto source = importDeclaration->Source()->Str().Utf8();
    return source == RemoveModuleExtensionName(interopSdkName_);
}

void TSDeclGen::GenInteropImport()
{
    OutDts("import st from \"", interopSdkName_, "\";");
    OutEndlDts();
}

void TSDeclGen::GenImportDeclarations()
{
    GenInteropImport();
    for (auto *globalStatement : program_->Ast()->Statements()) {
        if (globalStatement->IsETSImportDeclaration()) {
            if (IsInteropImport(globalStatement->AsETSImportDeclaration())) {
                continue;
            }
            GenImportDeclaration(globalStatement->AsETSImportDeclaration());
        }
    }
}

void TSDeclGen::GenImportRecordDeclarations(const std::string &source)
{
    const std::string recordKey = "escompat.Record";
    if (IsDependency(recordKey)) {
        OutDts("import type { Record } from \"", source, "\";");
        OutEndlDts();
    }
}

template <class T, class CB>
void TSDeclGen::GenSeparated(const T &container, const CB &cb, const char *separator, bool isReExport, bool isDtsExport)
{
    if (container.empty()) {
        return;
    }

    cb(container[0]);
    for (std::size_t i = 1; i < container.size(); ++i) {
        if (isReExport) {
            OutTs(separator);
        }
        if (isDtsExport) {
            OutDts(separator);
        }
        cb(container[i]);
    }
}

void TSDeclGen::LogError(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params = {},
                         const lexer::SourcePosition &pos = lexer::SourcePosition())
{
    diagnosticEngine_.LogDiagnostic(kind, params, pos);
}

void TSDeclGen::LogWarning(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params = {},
                           const lexer::SourcePosition &pos = lexer::SourcePosition())
{
    ES2PANDA_ASSERT(kind.Type() == util::DiagnosticType::DECLGEN_ETS2TS_WARNING);
    LogError(kind, params, pos);
}

const ir::Identifier *TSDeclGen::GetKeyIdent(const ir::Expression *key)
{
    if (!key->IsIdentifier()) {
        LogError(diagnostic::IDENT_KEY_SUPPORT, {}, key->Start());
    }

    return key->AsIdentifier();
}

static char const *GetDebugTypeName(const checker::Type *checkerType)
{
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPE_CHECKS(type_flag, typeName)                                                    \
    if (checkerType->Is##typeName()) {                                                      \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed*/ \
        return #typeName;                                                                   \
    }
    TYPE_MAPPING(TYPE_CHECKS)
#undef TYPE_CHECKS
    return "unknown type";
}

void TSDeclGen::GenType(const checker::Type *checkerType)
{
    DebugPrint("  GenType: ");
#if DEBUG_PRINT
    const auto var_name = checkerType->Variable() == nullptr ? "" : checkerType->Variable()->Name().Mutf8();
    DebugPrint(std::string("  Converting type: ") + GetDebugTypeName(checkerType) + " (" + var_name + ")");
#endif

    AddImport(checkerType->ToString());

    if (HandleBasicTypes(checkerType)) {
        return;
    }

    if (checkerType->IsETSFunctionType()) {
        HandleFunctionType(checkerType);
        return;
    }

    if (HandleETSSpecificTypes(checkerType)) {
        return;
    }

    LogError(diagnostic::UNSUPPORTED_TYPE, {GetDebugTypeName(checkerType)});
}

bool TSDeclGen::HandleBasicTypes(const checker::Type *checkerType)
{
    if (checkerType->IsETSEnumType()) {
        return false;
    }
    if (checkerType->HasTypeFlag(checker::TypeFlag::CHAR)) {
        OutDts("string");
        return true;
    }
    if (checkerType->HasTypeFlag(checker::TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC)) {
        OutDts("number");
        return true;
    }
    return false;
}

void TSDeclGen::HandleFunctionType(const checker::Type *checkerType)
{
    if (!state_.inUnionBodyStack.empty() && state_.inUnionBodyStack.top()) {
        OutDts("(");
        GenFunctionType(checkerType->AsETSFunctionType());
        OutDts(")");
    } else {
        GenFunctionType(checkerType->AsETSFunctionType());
    }
}

bool TSDeclGen::HandleETSSpecificTypes(const checker::Type *checkerType)
{
    switch (checker::ETSChecker::ETSType(checkerType)) {
        case checker::TypeFlag::ETS_VOID:
        case checker::TypeFlag::ETS_NULL:
        case checker::TypeFlag::ETS_UNDEFINED:
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::ETS_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NONNULLISH:
        case checker::TypeFlag::ETS_PARTIAL_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_READONLY:
            OutDts(checkerType->ToString());
            return true;

        case checker::TypeFlag::ETS_OBJECT:
            return HandleObjectType(checkerType);

        case checker::TypeFlag::ETS_ARRAY:
            HandleArrayType(checkerType);
            return true;

        case checker::TypeFlag::ETS_UNION:
            GenUnionType(checkerType->AsETSUnionType());
            return true;
        case checker::TypeFlag::ETS_TUPLE:
            GenTupleType(checkerType->AsETSTupleType());
            return true;
        case checker::TypeFlag::ETS_ANY:
            OutDts("ESObject");
            return true;
        default:
            LogError(diagnostic::UNSUPPORTED_TYPE, {GetDebugTypeName(checkerType)});
    }
    return false;
}

bool TSDeclGen::HandleObjectType(const checker::Type *checkerType)
{
    std::string typeStr = checkerType->ToString();
    if (typeStr == "Boolean") {
        OutDts("boolean");
    } else if (stringTypes_.count(typeStr) != 0U) {
        OutDts("string");
    } else if (numberTypes_.count(typeStr) != 0U) {
        OutDts("number");
    } else if (typeStr == "BigInt") {
        OutDts("bigint");
    } else if (typeStr == "ESValue") {
        OutDts("ESObject");
    } else {
        GenObjectType(checkerType->AsETSObjectType());
    }
    return true;
}

void TSDeclGen::HandleArrayType(const checker::Type *checkerType)
{
    GenArrayType(checkerType->AsETSArrayType()->ElementType());
}

void TSDeclGen::GenLiteral(const ir::Literal *literal)
{
    if (literal->IsNumberLiteral()) {
        const auto number = literal->AsNumberLiteral()->Number();
        if (number.IsByte()) {
            OutDts(std::to_string(number.GetByte()));
            return;
        }
        if (number.IsShort()) {
            OutDts(std::to_string(number.GetShort()));
            return;
        }
        if (number.IsInt()) {
            OutDts(std::to_string(number.GetInt()));
            return;
        }
        if (number.IsLong()) {
            OutDts(std::to_string(number.GetLong()));
            return;
        }
        if (number.IsFloat()) {
            OutDts(std::to_string(number.GetFloat()));
            return;
        }
        if (number.IsDouble()) {
            OutDts(std::to_string(number.GetDouble()));
            return;
        }
        LogError(diagnostic::UNEXPECTED_NUMBER_LITERAL_TYPE, {}, literal->Start());
    } else if (literal->IsStringLiteral()) {
        const auto string = literal->AsStringLiteral()->ToString();
        AddImport(string);
        OutDts("\"" + string + "\"");
    } else if (literal->IsBooleanLiteral()) {
        OutDts(literal->AsBooleanLiteral()->ToString());
    } else {
        LogError(diagnostic::UNSUPPORTED_LITERAL_TYPE, {}, literal->Start());
    }
}

void TSDeclGen::ProcessParamDefaultToMap(const ir::Statement *stmt)
{
    if (!stmt->IsVariableDeclaration()) {
        return;
    }
    GenSeparated(
        stmt->AsVariableDeclaration()->Declarators(),
        [this](ir::VariableDeclarator *declarator) {
            const auto *init = declarator->Init();
            if (init != nullptr && init->IsConditionalExpression() &&
                init->AsConditionalExpression()->Test()->IsBinaryExpression()) {
                const auto *left = init->AsConditionalExpression()->Test()->AsBinaryExpression()->Left();
                if (left->IsIdentifier()) {
                    const auto varName = GetKeyIdent(declarator->Id())->Name();
                    paramDefaultMap_.insert({left->AsIdentifier()->Name(), varName});
                }
            }
        },
        "");
}

const checker::Signature *TSDeclGen::GetFuncSignature(const checker::ETSFunctionType *etsFunctionType,
                                                      const ir::MethodDefinition *methodDef)
{
    if (etsFunctionType->IsETSArrowType()) {
        return etsFunctionType->ArrowSignature();
    }
    if (methodDef != nullptr) {
        auto methDefFunc = methodDef->Function();
        return methDefFunc != nullptr ? methDefFunc->Signature() : nullptr;
    }
    if (etsFunctionType->CallSignatures().size() != 1) {
        const auto loc = methodDef != nullptr ? methodDef->Start() : lexer::SourcePosition();
        LogError(diagnostic::NOT_OVERLOAD_SUPPORT, {}, loc);
    }
    return etsFunctionType->CallSignatures()[0];
}

void TSDeclGen::ProcessParameterName(varbinder::LocalVariable *param)
{
    const auto *paramDeclNode = param->Declaration()->Node();
    const std::string prefix = "gensym%%_";

    if (!paramDefaultMap_.empty() && paramDefaultMap_.find(param->Name()) != paramDefaultMap_.end()) {
        OutDts(paramDefaultMap_[param->Name()]);
        paramDefaultMap_.erase(param->Name());
    } else if (param->Name().Is("=t")) {
        OutDts("this");
    } else if (paramDeclNode->IsETSParameterExpression() && paramDeclNode->AsETSParameterExpression()->IsOptional() &&
               paramDeclNode->AsETSParameterExpression()->Name().StartsWith(prefix)) {
        OutDts("arg", param->Name().Mutf8().substr(prefix.size()));
    } else {
        OutDts(param->Name());
    }
}

void TSDeclGen::ProcessRestParameterTypeAnnotationType(const ir::TypeNode *typeAnnotation)
{
    if (!typeAnnotation->IsETSTypeReference()) {
        ProcessTypeAnnotationType(typeAnnotation);
        return;
    }
    const auto *typeReference = typeAnnotation->AsETSTypeReference();
    const auto typePart = typeReference->Part();
    auto partName = typePart->GetIdent()->Name().Mutf8();
    AddImport(partName);
    if (typePart->TypeParams() != nullptr && typePart->TypeParams()->IsTSTypeParameterInstantiation()) {
        if (partName == "ReadonlyArray" || partName == "FixedArray" ||
            (typeReference->Parent()->Parent()->IsETSParameterExpression() &&
             typeReference->Parent()->Parent()->AsETSParameterExpression()->TypeAnnotation() != nullptr &&
             typeReference->Parent()->Parent()->AsETSParameterExpression()->TypeAnnotation()->IsReadonlyType() &&
             partName == "Array")) {
            GenArrayType(typePart->TypeParams()->Params()[0]->GetType(checker_));
        } else {
            OutDts(partName);
            OutDts("<");
            GenSeparated(typePart->TypeParams()->Params(),
                         [this](ir::TypeNode *param) { ProcessTypeAnnotationType(param, param->GetType(checker_)); });
            OutDts(">");
        }
    } else {
        GenPartName(partName);
        OutDts(partName);
    }
}

void TSDeclGen::ProcessFuncRestParameter(varbinder::LocalVariable *param)
{
    const auto *paramDeclNode = param->Declaration()->Node();
    const auto *expr = paramDeclNode->AsETSParameterExpression();
    const auto *typeAnnotation = expr->TypeAnnotation();

    ProcessParameterName(param);

    OutDts(": ");
    if (typeAnnotation != nullptr && typeAnnotation->IsETSTypeReference()) {
        ProcessRestParameterTypeAnnotationType(typeAnnotation);
        return;
    }
    OutDts("ESObject");
}

void TSDeclGen::ProcessFuncParameter(varbinder::LocalVariable *param)
{
    const auto *paramType = param->TsType();
    const auto *paramDeclNode = param->Declaration()->Node();
    if (param->Declaration()->Node()->IsProperty()) {
        return;
    }

    ProcessParameterName(param);

    if (!paramDeclNode->IsETSParameterExpression()) {
        if (param->HasFlag(varbinder::VariableFlags::OPTIONAL)) {
            OutDts("?");
        }
        OutDts(": ");
        GenType(paramType);
        return;
    }

    const auto *expr = paramDeclNode->AsETSParameterExpression();
    OutDts(expr->IsOptional() || (expr->OriginalNode() != nullptr && expr->OriginalNode()->IsETSParameterExpression() &&
                                  expr->OriginalNode()->AsETSParameterExpression()->IsOptional())
               ? "?"
               : "");
    OutDts(": ");

    const auto *typeAnnotation = expr->TypeAnnotation();
    if (paramType->IsETSReadonlyArrayType() || (typeAnnotation != nullptr && typeAnnotation->IsReadonlyType())) {
        OutDts("readonly ");
    }

    if (typeAnnotation != nullptr) {
        ProcessTypeAnnotationType(typeAnnotation, expr->IsOptional() ? nullptr : paramType);
        return;
    }
    OutDts("ESObject");
}

void TSDeclGen::GenOptionalFlag(const checker::Signature *sig, const ir::MethodDefinition *methodDef)
{
    if (sig->HasSignatureFlag(checker::SignatureFlags::DEFAULT) ||
        (state_.inInterface && methodDef != nullptr && methodDef->Value()->IsFunctionExpression() &&
         methodDef->Value()->AsFunctionExpression()->Function()->IsScriptFunction() &&
         methodDef->Value()->AsFunctionExpression()->Function()->AsScriptFunction()->HasBody())) {
        OutDts("?");
    }
}

void TSDeclGen::ProcessFuncParameters(const checker::Signature *sig, bool applyOverrides)
{
    if (!applyOverrides || interopParamOverrides_.empty()) {
        GenSeparated(sig->Params(), [this](varbinder::LocalVariable *param) { ProcessFuncParameter(param); });
        return;
    }
    bool first = true;
    std::size_t userIdx = 0;
    for (auto *param : sig->Params()) {
        if (!first) {
            OutDts(", ");
        }
        first = false;
        const bool isThis = param->Name().Is("=t");
        if (isThis) {
            ProcessFuncParameter(param);
            continue;
        }
        const auto it = interopParamOverrides_.find(userIdx);
        ++userIdx;
        if (it == interopParamOverrides_.end()) {
            ProcessFuncParameter(param);
            continue;
        }
        ProcessParameterName(param);
        if (param->HasFlag(varbinder::VariableFlags::OPTIONAL)) {
            OutDts("?");
        }
        OutDts(": ");
        OutDts(it->second);
    }
}

void TSDeclGen::GenFunctionType(const checker::ETSFunctionType *etsFunctionType, const ir::MethodDefinition *methodDef)
{
    const bool isConstructor = methodDef != nullptr ? methodDef->IsConstructor() : false;
    const bool isSetter = methodDef != nullptr ? methodDef->Kind() == ir::MethodDefinitionKind::SET : false;
    const bool isStatic = methodDef != nullptr ? methodDef->IsStatic() : false;
    // CC-OFFNXT(G.FMT.14-CPP) project code style
    const auto *sig = GetFuncSignature(etsFunctionType, methodDef);
    ES2PANDA_ASSERT(sig != nullptr);
    GenOptionalFlag(sig, methodDef);
    if (sig->HasFunction()) {
        GenTypeParameters(sig->Function()->TypeParams(), isStatic, sig->Owner()->AsETSObjectType());
        const auto *funcBody = sig->Function()->Body();
        if (funcBody != nullptr && funcBody->IsBlockStatement() &&
            !funcBody->AsBlockStatement()->Statements().empty()) {
            for (const auto *statement : funcBody->AsBlockStatement()->Statements()) {
                ProcessParamDefaultToMap(statement);
            }
        }
    }
    OutDts("(");

    ProcessFuncParameters(sig, methodDef != nullptr);

    const auto *sigInfo = sig->GetSignatureInfo();
    if (sigInfo->restVar != nullptr) {
        if (!sig->Params().empty()) {
            OutDts(", ");
        }
        OutDts("...");
        ProcessFuncRestParameter(sigInfo->restVar);
    }
    OutDts(")");
    if (!isSetter && !isConstructor) {
        OutDts(methodDef != nullptr ? ": " : " => ");
        if (methodDef != nullptr && !interopRetOverride_.empty()) {
            OutDts(interopRetOverride_);
            return;
        }
        if (!sig->HasFunction()) {
            if (sig->ReturnType()->HasTypeFlag(checker::TypeFlag::ETS_UNDEFINED) &&
                IsTypeScriptDeclarationOutput(declgenOptions_)) {
                OutDts("void");
            } else {
                GenType(sig->ReturnType());
            }
            return;
        }
        ProcessFunctionReturnType(sig);
    }
}

void TSDeclGen::AddUnionTypeImports(std::string &unionTypeString)
{
    std::vector<std::string> result;
    std::string currentType;

    for (char c : unionTypeString) {
        if (std::isspace(c) != 0) {
            continue;
        }
        if (c == '|') {
            if (!currentType.empty()) {
                AddImport(currentType);
                currentType.clear();
            }
        } else {
            currentType += c;
        }
    }
    if (!currentType.empty()) {
        AddImport(currentType);
    }
}

void TSDeclGen::ProcessFunctionReturnType(const checker::Signature *sig)
{
    const auto returnStatements = sig->Function()->ReturnStatements();
    if (!returnStatements.empty() && returnStatements.size() == 1 && returnStatements.at(0)->Argument() != nullptr &&
        returnStatements.at(0)->Argument()->IsETSNewClassInstanceExpression()) {
        auto newExpr = returnStatements.at(0)->Argument()->AsETSNewClassInstanceExpression();
        if (newExpr->GetTypeRef() != nullptr && newExpr->GetTypeRef()->IsETSTypeReference()) {
            ProcessETSTypeReferenceType(newExpr->GetTypeRef()->AsETSTypeReference(), sig->ReturnType());
            return;
        }
    }

    const auto returnTypeAnnotation = sig->Function()->ReturnTypeAnnotation();
    if (returnTypeAnnotation != nullptr) {
        ProcessTypeAnnotationType(returnTypeAnnotation, sig->ReturnType());
        return;
    }

    if (sig->HasSignatureFlag(checker::SignatureFlags::SETTER)) {
        const auto param = sig->Function()->Params();
        if (!param.empty() && param.size() == 1 && param.at(0)->IsETSParameterExpression() &&
            param.at(0)->AsETSParameterExpression()->Ident()->TypeAnnotation() != nullptr) {
            ProcessTypeAnnotationType(param.at(0)->AsETSParameterExpression()->Ident()->TypeAnnotation(),
                                      sig->Params()[0]->TsType());
            return;
        }
        if (!sig->Params().empty() && sig->Params().size() == 1) {
            GenType(sig->Params()[0]->TsType());
            return;
        }
    }

    if (sig->ReturnType()->HasTypeFlag(checker::TypeFlag::ETS_UNDEFINED) &&
        IsTypeScriptDeclarationOutput(declgenOptions_)) {
        OutDts("void");
        return;
    }

    std::string typeStr = sig->ReturnType()->ToString();
    if (declgenOptions_.isolated && typeStr.find(ERROR_TYPE) != std::string::npos) {
        typeStr = isolatedDeclgenChecker_->Check(const_cast<ir::ScriptFunction *>(sig->Function()));
        OutDts(typeStr);
        AddUnionTypeImports(typeStr);
        return;
    }
    GenType(sig->ReturnType());
}

void TSDeclGen::GenUnionType(const checker::ETSUnionType *unionType)
{
    state_.inUnionBodyStack.push(true);
    std::vector<checker::Type *> filteredTypes = FilterUnionTypes(unionType->ConstituentTypes());
    GenSeparated(
        filteredTypes, [this](checker::Type *arg) { GenType(arg); }, " | ");
    state_.inUnionBodyStack.pop();
}

template <class UnionType>
std::vector<UnionType *> TSDeclGen::FilterUnionTypes(const ArenaVector<UnionType *> &originTypes)
{
    if (originTypes.empty()) {
        return {};
    }
    bool hasNumber = false;
    bool hasString = false;
    std::vector<UnionType *> filteredTypes;
    for (auto originType : originTypes) {
        std::string typeStr = originType->ToString();
        if constexpr (std::is_same_v<UnionType, ir::TypeNode>) {
            if (originType->IsTSThisType()) {
                filteredTypes.push_back(originType);
                continue;
            }
            auto type = originType->GetType(checker_);
            if (type == nullptr) {
                continue;
            }
            typeStr = type->ToString();
            typeStr[0] = std::toupper(typeStr[0]);
        }
        if (stringTypes_.count(typeStr) != 0U) {
            if (hasString) {
                continue;
            }
            hasString = true;
        } else if (numberTypes_.count(typeStr) != 0U) {
            if (hasNumber) {
                continue;
            }
            hasNumber = true;
        }
        filteredTypes.push_back(originType);
    }
    return filteredTypes;
}

void TSDeclGen::GenTupleType(const checker::ETSTupleType *tupleType)
{
    OutDts("[");
    GenSeparated(
        tupleType->GetTupleTypesList(), [this](checker::Type *arg) { GenType(arg); }, " , ");
    OutDts("]");
}

bool TSDeclGen::HandleSpecificObjectTypes(const checker::ETSObjectType *objectType)
{
    if (objectType->IsETSStringType()) {
        OutDts("string");
        return true;
    }
    if (objectType->IsETSBigIntType()) {
        OutDts("bigint");
        return true;
    }
    if (objectType->IsETSUnboxableObject()) {
        OutDts("number");  // NOTE(ivagin): create precise builtin type
        return true;
    }
    if (objectType->HasObjectFlag(checker::ETSObjectFlags::FUNCTIONAL)) {
        const auto *invoke = objectType->GetFunctionalInterfaceInvokeType();
        ES2PANDA_ASSERT(invoke && invoke->IsETSFunctionType());
        GenType(invoke);
        return true;
    }
    return false;
}

void TSDeclGen::HandleTypeArgument(checker::Type *arg, const std::string &typeStr)
{
    if (typeStr == "Promise" && arg != nullptr && arg->HasTypeFlag(checker::TypeFlag::ETS_UNDEFINED)) {
        OutDts("void");
    } else if (arg != nullptr) {
        if (!state_.currentTypeAliasName.empty() && !arg->HasTypeFlag(checker::TypeFlag::ETS_TYPE_PARAMETER) &&
            (arg->IsETSObjectType() && !arg->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_TYPE))) {
            OutDts(state_.currentTypeAliasName);
            if (state_.currentTypeParams != nullptr) {
                AddImport(state_.currentTypeParams->Params()[0]->Name()->Name().Mutf8());
                OutDts("<");
                OutDts(state_.currentTypeParams->Params()[0]->Name()->Name());
                OutDts(">");
            }
        } else {
            GenType(arg);
        }
    }
}

void TSDeclGen::GenObjectType(const checker::ETSObjectType *objectType)
{
    if (HandleSpecificObjectTypes(objectType)) {
        return;
    }
    std::string typeStr = objectType->Name().Mutf8();
    if (objectType->Name().Empty()) {
        LogWarning(diagnostic::EMPTY_TYPE_NAME);
        OutDts("ESObject");
    } else {
        if (typeStr == "Exception" || typeStr == "NullPointerError") {
            OutDts("Error");
        } else if (size_t partialPos = typeStr.find("%%partial-"); partialPos != std::string::npos) {
            OutDts("Partial<", typeStr.substr(0, partialPos), ">");
        } else {
            OutDts(ConvertInteropTypeName(typeStr));
        }
        AddImport(typeStr);
    }

    const auto &typeArgs = objectType->TypeArguments();
    if (typeArgs.empty()) {
        return;
    }

    OutDts("<");
    GenSeparated(typeArgs, [this, typeStr](checker::Type *arg) { HandleTypeArgument(arg, typeStr); });
    OutDts(">");
}

void TSDeclGen::GenTypeParameters(const ir::TSTypeParameterDeclaration *typeParams, bool isStatic,
                                  const checker::ETSObjectType *ownerObj)
{
    if (typeParams != nullptr) {
        OutDts("<");
        GenSeparated(typeParams->Params(), [this](ir::TSTypeParameter *param) {
            OutDts(param->Name()->Name());
            auto *constraint = param->Constraint();
            if (constraint != nullptr) {
                OutDts(" extends ");
                ProcessTypeAnnotationType(constraint, constraint->GetType(checker_));
            }
            auto *defaultType = param->DefaultType();
            if (defaultType != nullptr) {
                OutDts(" = ");
                ProcessTypeAnnotationType(defaultType, defaultType->GetType(checker_));
            }
        });
        OutDts(">");
        return;
    }
    if (ownerObj == nullptr || !isStatic) {
        return;
    }
    auto typeArguments = ownerObj->TypeArguments();
    if (!typeArguments.empty()) {
        OutDts("<");
        for (auto arg : typeArguments) {
            OutDts(arg->ToString());
            if (arg != typeArguments.back()) {
                OutDts(",");
            }
        }
        OutDts(">");
    }
}

void TSDeclGen::GenArrayType(const checker::Type *elementType)
{
    bool needParentheses = elementType->IsETSUnionType() || elementType->IsETSFunctionType();
    OutDts(needParentheses ? "(" : "");
    GenType(elementType);
    OutDts(needParentheses ? ")" : "");
    OutDts("[]");
}

void TSDeclGen::GenExport(const ir::Identifier *symbol)
{
    const auto symbolName = symbol->Name().Mutf8();
    OutDts("export {", symbolName, "};");
    OutEndlDts();
    if (!symbol->Parent()->IsTSTypeAliasDeclaration() && !symbol->Parent()->IsTSInterfaceDeclaration()) {
        OutDts("exports.", symbolName, " = ", symbolName, ";");
    }
    OutEndlDts();
}

void TSDeclGen::GenExport(const ir::Identifier *symbol, const std::string &alias)
{
    const auto symbolName = symbol->Name().Mutf8();
    OutDts("export {", symbolName, " as ", alias, "};");
    OutEndlDts();
    if (!symbol->Parent()->IsTSTypeAliasDeclaration() && !symbol->Parent()->IsTSInterfaceDeclaration()) {
        OutDts("exports.", alias, " = ", symbolName, ";");
    }
    OutEndlDts();
}

void TSDeclGen::GenDefaultExport(const ir::Identifier *symbol)
{
    const auto symbolName = symbol->Name().Mutf8();
    OutDts("export default ", symbolName, ";");
    OutEndlDts();
    if (!symbol->Parent()->IsTSTypeAliasDeclaration() && !symbol->Parent()->IsTSInterfaceDeclaration()) {
        OutDts("exports.default = ", symbolName, ";");
    }
    OutEndlDts();
}

bool TSDeclGen::ShouldEmitDeclaration(const ir::AstNode *decl)
{
    if (declgenOptions_.exportAll) {
        return true;
    }
    if (decl->IsExported() || decl->IsDefaultExported()) {
        return true;
    }
    if (state_.isDeclareNamespace) {
        return true;
    }
    if (IsDependency(decl)) {
        return true;
    }
    if (decl->IsTSTypeAliasDeclaration()) {
        return true;
    }

    return false;
}

template <class T>
void TSDeclGen::GenModifier(const T *node, bool isProp)
{
    if (state_.inInterface) {
        return;
    }

    if (state_.inNamespace && isProp && !state_.isClassInNamespace) {
        OutDts(node->IsConst() ? "const " : "let ");
        return;
    }
    if (state_.inNamespace && !isProp && !state_.isClassInNamespace) {
        OutDts("function ");
        return;
    }
    if (node->IsPublic()) {
        OutDts("public ");
    } else if (node->IsPrivate()) {
        OutDts("private ");
    } else if (node->IsProtected()) {
        OutDts("protected ");
    }
    if (node->IsStatic()) {
        OutDts("static ");
    }
    if (node->IsReadonly() && isProp) {
        OutDts("readonly ");
    }
}

std::string TSDeclGen::RemoveModuleExtensionName(const std::string &filepath)
{
    for (const auto &extension : extensions_) {
        auto pos = filepath.rfind(extension);
        if (pos != std::string::npos && pos == filepath.length() - extension.length()) {
            return filepath.substr(0, pos);
        }
    }
    return filepath;
}

template <class T>
void TSDeclGen::GenAnnotations(const ir::AnnotationAllowed<T> *node)
{
    if (!declgenOptions_.genAnnotations || node == nullptr ||
        (!node->HasAnnotations() && node->Annotations().size() == 0U)) {
        return;
    }
    GenSeparated(
        node->Annotations(),
        [this](ir::AnnotationUsage *anno) {
            if (annotationList_.count(anno->GetBaseName()->Name().Mutf8()) == 0U) {
                return;
            }
            if (!state_.inGlobalClass && (state_.inClass || state_.inInterface)) {
                bool inClass = state_.inClass;
                bool inInterface = state_.inInterface;
                DebugPrint(inClass ? "true" : "false");
                DebugPrint(inInterface ? "true" : "false");
                ProcessIndent();
            }
            AddImport(anno->GetBaseName()->Name().Mutf8());
            OutDts("@", anno->GetBaseName()->Name());
            GenAnnotationProperties(anno);
            OutEndlDts();
        },
        "");
}

void TSDeclGen::GenAnnotationProperties(const ir::AnnotationUsage *anno)
{
    if (anno->Properties().empty()) {
        return;
    }

    const auto &properties = anno->Properties();
    if (properties.size() == 1 && properties.at(0)->IsClassProperty() &&
        properties.at(0)->AsClassProperty()->Id() != nullptr &&
        properties.at(0)->AsClassProperty()->Id()->Name() == compiler::Signatures::ANNOTATION_KEY_VALUE) {
        OutDts("(");
        if (properties.at(0)->AsClassProperty()->Value() != nullptr) {
            GenAnnotationPropertyValue(properties.at(0)->AsClassProperty()->Value());
        }
        OutDts(")");
        return;
    }

    OutDts("({");
    OutEndlDts();
    for (auto *prop : properties) {
        ProcessIndent();
        ES2PANDA_ASSERT(prop->AsClassProperty()->Id() != nullptr);
        OutDts(prop->AsClassProperty()->Id()->Name());
        OutDts(": ");
        if (prop->AsClassProperty()->Value() != nullptr) {
            GenAnnotationPropertyValue(prop->AsClassProperty()->Value());
        }
        if (prop != properties.back()) {
            OutDts(",");
        }
        OutEndlDts();
    }
    OutDts("})");
}

void TSDeclGen::GenAnnotationPropertyValue(ir::Expression *propValue)
{
    if (propValue->IsLiteral()) {
        GenLiteral(propValue->AsLiteral());
    } else if (propValue->IsArrayExpression()) {
        OutDts("[");
        GenSeparated(propValue->AsArrayExpression()->Elements(),
                     [this](ir::Expression *element) { GenAnnotationPropertyValue(element); });
        OutDts("]");
    } else {
        GenType(propValue->Check(checker_));
    }
}

void TSDeclGen::GenExportNamedDeclaration(const ir::ExportNamedDeclaration *exportDeclaration)
{
    DebugPrint("GenExportNamedDeclaration");
    const auto &specifiers = exportDeclaration->Specifiers();
    if (specifiers.empty()) {
        return;
    }
    GenNamedExports(exportDeclaration, specifiers);
}

void TSDeclGen::GenImportDeclaration(const ir::ETSImportDeclaration *importDeclaration)
{
    DebugPrint("GenImportDeclaration");
    if (importDeclaration->IsPureDynamic()) {
        return;
    }
    const auto &specifiers = importDeclaration->Specifiers();
    if (specifiers.empty()) {
        return;
    }
    auto source = importDeclaration->Source()->Str().Mutf8();
    source = RemoveModuleExtensionName(source);
    bool isTypeKind = importDeclaration->IsTypeKind();
    // There are cases like `import A, {a}`, where we need to reclassify each specifier independently.
    bool hasNamedImport = false;
    for (auto *specifier : specifiers) {
        if (specifier->IsImportNamespaceSpecifier()) {
            GenNamespaceImport(specifier, source);
        } else if (specifier->IsImportDefaultSpecifier()) {
            GenDefaultImport(specifier, source, isTypeKind);
        } else if (specifier->IsImportSpecifier()) {
            hasNamedImport = true;
        }
    }
    if (hasNamedImport) {
        GenNamedImports(importDeclaration, specifiers, isTypeKind);
    }
}

void TSDeclGen::GenNamespaceImport(const ir::AstNode *specifier, const std::string &source)
{
    const auto local = specifier->AsImportNamespaceSpecifier()->Local()->Name().Mutf8();
    OutTs("import * as ", local, " from \"", source, "\";");
    OutEndlTs();
    if (!IsImport(local)) {
        return;
    }
    OutDts("import * as ", local, " from \"", source, "\";");
    OutEndlDts();
}

void TSDeclGen::GenDefaultImport(const ir::AstNode *specifier, const std::string &source, bool isTypeKind)
{
    auto importDefaultSpecifier = specifier->AsImportDefaultSpecifier();
    auto variable = importDefaultSpecifier->Local()->Variable();
    const auto local = importDefaultSpecifier->Local()->Name().Mutf8();
    bool isTypeDeclaration = false;
    if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
        auto *node = variable->Declaration()->Node();
        isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
    }
    if (!isTypeKind && !isTypeDeclaration) {
        OutTs("import ", local, " from \"", source, "\";");
        OutEndlTs();
    }

    if (!IsImport(local)) {
        return;
    }
    OutDts(isTypeKind ? "import type " : "import ", local, " from \"", source, "\";");
    OutEndlDts();
}

void TSDeclGen::GenNamedImports(const ir::ETSImportDeclaration *importDeclaration,
                                const ArenaVector<ir::AstNode *> &specifiers, bool isTypeKind)
{
    if (specifiers.empty()) {
        return;
    }
    std::vector<ir::AstNode *> interfaceSpecifiers;
    std::vector<ir::AstNode *> normalSpecifiers;
    SeparateInterfaceSpecifiers(specifiers, interfaceSpecifiers, normalSpecifiers);

    if (!isTypeKind) {
        GenTsImportStatement(normalSpecifiers, importDeclaration);
    }

    auto importSpecifiers = FilterValidImportSpecifiers(specifiers);
    GenDtsImportStatement(importSpecifiers, importDeclaration, isTypeKind);
}

void TSDeclGen::GenTsImportStatement(std::vector<ir::AstNode *> &specifiers,
                                     const ir::ETSImportDeclaration *importDeclaration, bool isInterface)
{
    if (specifiers.empty()) {
        return;
    }

    auto source = importDeclaration->Source()->Str().Mutf8();
    source = RemoveModuleExtensionName(source);
    OutTs(isInterface ? "import type" : "import", " { ");

    GenSeparated(
        specifiers,
        [this, importDeclaration](ir::AstNode *specifier) { GenSingleNamedImport(specifier, importDeclaration, true); },
        ", ", true, false);

    OutTs(" } from \"", source, "\";");
    OutEndlTs();
}

void TSDeclGen::GenDtsImportStatement(std::vector<ir::AstNode *> &specifiers,
                                      const ir::ETSImportDeclaration *importDeclaration, bool isTypeKind)
{
    if (specifiers.empty()) {
        return;
    }

    auto source = importDeclaration->Source()->Str().Mutf8();
    source = RemoveModuleExtensionName(source);
    OutDts(isTypeKind ? "import type" : "import", " { ");

    GenSeparated(
        specifiers,
        [this, importDeclaration](ir::AstNode *specifier) { GenSingleNamedImport(specifier, importDeclaration); },
        ", ");

    OutDts(" } from \"", source, "\";");
    OutEndlDts();
}

void TSDeclGen::GenNamedExports(const ir::ExportNamedDeclaration *exportDeclaration,
                                const ArenaVector<ir::ExportSpecifier *> &specifiers)
{
    auto exportSpecifiers = FilterValidExportSpecifiers(specifiers);
    if (exportSpecifiers.empty()) {
        return;
    }
    if ((exportDeclaration->Modifiers() & (ir::ModifierFlags::DEFAULT_EXPORT)) != 0U) {
        const auto local = specifiers[0]->Local()->Name().Mutf8();
        AddImport(local);
        OutDts("export default ", local, ";");
        OutEndlDts();
        if (glueCodeImportSet_.find(local) != glueCodeImportSet_.end()) {
            OutTs("export default ", local, ";");
            OutEndlTs();
        }
        return;
    }

    if ((exportDeclaration->Modifiers() & (ir::ModifierFlags::EXPORT_TYPE)) != 0U) {
        OutDts("export type { ");
    } else {
        OutDts("export { ");
    }

    GenSeparated(
        exportSpecifiers, [this](ir::AstNode *specifier) { GenSingleNamedExport(specifier); }, ", ");
    OutDts(" };");
    OutEndlDts();

    if ((exportDeclaration->Modifiers() & (ir::ModifierFlags::EXPORT_TYPE)) != 0U) {
        return;
    }
    auto guleCodeExportSpecifiers = FilterGlueCodeExportSpecifiers(exportSpecifiers);
    if (guleCodeExportSpecifiers.empty()) {
        return;
    }
    OutTs("export { ");
    GenSeparated(
        guleCodeExportSpecifiers, [this](ir::AstNode *specifier) { GenSingleNamedExport(specifier, true); }, ", ", true,
        false);
    OutTs(" };");
    OutEndlTs();
}

void TSDeclGen::GenSingleNamedImport(ir::AstNode *specifier, const ir::ETSImportDeclaration *importDeclaration,
                                     bool isGlueCode)
{
    if (!specifier->IsImportSpecifier()) {
        LogError(diagnostic::IMPORT_SPECIFIERS_SUPPORT, {}, importDeclaration->Start());
    }
    const auto local = ExportSpecifierName(specifier->AsImportSpecifier()->Local());
    const auto imported = ExportSpecifierName(specifier->AsImportSpecifier()->Imported());
    if (local != imported) {
        isGlueCode ? OutTs(imported, " as ", local) : OutDts(imported, " as ", local);
    } else {
        isGlueCode ? OutTs(local) : OutDts(local);
    }
}

void TSDeclGen::GenSingleNamedExport(ir::AstNode *specifier, bool isGlueCode)
{
    const auto local = ExportSpecifierName(specifier->AsExportSpecifier()->Local());
    const auto imported = ExportSpecifierName(specifier->AsExportSpecifier()->Exported());
    AddImport(imported);
    if (local != imported) {
        isGlueCode ? OutTs(imported, " as ", local) : OutDts(imported, " as ", local);
    } else {
        isGlueCode ? OutTs(local) : OutDts(local);
    }
}

std::vector<ir::AstNode *> TSDeclGen::FilterValidExportSpecifiers(const ArenaVector<ir::ExportSpecifier *> &specifiers)
{
    std::vector<ir::AstNode *> exportSpecifiers;
    for (auto specifier : specifiers) {
        if (IsInternalDefaultExportSpecifier(specifier)) {
            continue;
        }
        const auto local = specifier->AsExportSpecifier()->Local()->Name().Mutf8();
        if (exportSet_.find(local) == exportSet_.end()) {
            exportSpecifiers.push_back(specifier);
        }
    }
    return exportSpecifiers;
}

std::vector<ir::AstNode *> TSDeclGen::FilterGlueCodeExportSpecifiers(const std::vector<ir::AstNode *> &specifiers)
{
    std::vector<ir::AstNode *> glueCodeExportSpecifiers;
    for (auto specifier : specifiers) {
        const auto local = specifier->AsExportSpecifier()->Local()->Name().Mutf8();
        if (glueCodeImportSet_.find(local) != glueCodeImportSet_.end()) {
            glueCodeExportSpecifiers.push_back(specifier);
        }
    }
    return glueCodeExportSpecifiers;
}

std::vector<ir::AstNode *> TSDeclGen::FilterValidImportSpecifiers(const ArenaVector<ir::AstNode *> &specifiers)
{
    std::vector<ir::AstNode *> importSpecifiers;
    for (auto specifier : specifiers) {
        if (!specifier->IsImportSpecifier()) {
            continue;
        }
        if (specifier->AsImportSpecifier()->IsRemovable()) {
            continue;
        }
        const auto local = specifier->AsImportSpecifier()->Local()->Name().Mutf8();
        if (IsImport(local)) {
            importSpecifiers.push_back(specifier);
        }
    }
    return importSpecifiers;
}

void TSDeclGen::GenReExportDeclaration(const ir::ETSReExportDeclaration *reExportDeclaration)
{
    DebugPrint("GenReExportDeclaration");
    auto importDeclaration = reExportDeclaration->GetETSImportDeclarations();
    if (importDeclaration->IsPureDynamic()) {
        return;
    }
    const auto &specifiers = importDeclaration->Specifiers();

    if (specifiers.size() == 1 && GenNamespaceReExportDeclaration(specifiers[0], importDeclaration)) {
        return;
    }

    bool isTypeKind = reExportDeclaration->IsExportedType();
    std::vector<ir::AstNode *> interfaceSpecifiers;
    std::vector<ir::AstNode *> normalSpecifiers;
    SeparateInterfaceSpecifiers(specifiers, interfaceSpecifiers, normalSpecifiers);

    GenDtsReExportStatement(specifiers, importDeclaration, isTypeKind);

    if (!isTypeKind) {
        GenTsReExportStatement(normalSpecifiers, importDeclaration);
    }
}

bool TSDeclGen::GenNamespaceReExportDeclaration(const ir::AstNode *specifier,
                                                const ir::ETSImportDeclaration *importDeclaration)
{
    if (specifier->IsImportNamespaceSpecifier()) {
        const auto local = specifier->AsImportNamespaceSpecifier()->Local()->Name();
        if (local.Empty()) {
            auto source = importDeclaration->Source()->Str().Mutf8();
            source = RemoveModuleExtensionName(source);
            OutDts("export * from \"", source, "\";");
            OutEndlDts();
            OutTs("export * from \"", source, "\";");
            OutEndlTs();
            return true;
        }
    }
    return false;
}

void TSDeclGen::SeparateInterfaceSpecifiers(const ArenaVector<ir::AstNode *> &specifiers,
                                            std::vector<ir::AstNode *> &interfaceSpecifiers,
                                            std::vector<ir::AstNode *> &normalSpecifiers)
{
    for (auto *specifier : specifiers) {
        if (!specifier->IsImportSpecifier()) {
            continue;
        }
        auto importSpecifier = specifier->AsImportSpecifier();
        auto variable = importSpecifier->Imported()->Variable();
        bool isTypeDeclaration = false;
        if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
            auto *node = variable->Declaration()->Node();
            isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
        }
        if (isTypeDeclaration) {
            interfaceSpecifiers.push_back(specifier);
        } else {
            normalSpecifiers.push_back(specifier);
        }
    }
}

void TSDeclGen::GenSingleNamedReExport(ir::AstNode *specifier, const ir::ETSImportDeclaration *importDeclaration,
                                       bool isGlueCode)
{
    if (specifier->IsImportSpecifier()) {
        const auto local = ExportSpecifierName(specifier->AsImportSpecifier()->Local());
        const auto imported = ExportSpecifierName(specifier->AsImportSpecifier()->Imported());
        AddImport(local);
        if (local != imported) {
            isGlueCode ? OutTs(imported, " as ", local) : OutDts(imported, " as ", local);
        } else {
            isGlueCode ? OutTs(local) : OutDts(local);
        }
    } else if (specifier->IsImportNamespaceSpecifier()) {
        const auto local = specifier->AsImportNamespaceSpecifier()->Local()->Name().Mutf8();
        AddImport(local);
        isGlueCode ? OutTs(local) : OutDts(local);
    } else {
        LogError(diagnostic::IMPORT_SPECIFIERS_SUPPORT, {}, importDeclaration->Start());
    }
}

void TSDeclGen::GenDtsReExportStatement(const ArenaVector<ir::AstNode *> &specifiers,
                                        const ir::ETSImportDeclaration *importDeclaration, bool isTypeKind)
{
    if (specifiers.empty()) {
        return;
    }

    auto source = importDeclaration->Source()->Str().Mutf8();
    source = RemoveModuleExtensionName(source);
    OutDts(isTypeKind ? "export type" : "export", " { ");

    GenSeparated(
        specifiers,
        [this, importDeclaration](ir::AstNode *specifier) { GenSingleNamedReExport(specifier, importDeclaration); },
        ", ");

    OutDts(" } from \"", source, "\";");
    OutEndlDts();
}

void TSDeclGen::GenTsReExportStatement(const std::vector<ir::AstNode *> &specifiers,
                                       const ir::ETSImportDeclaration *importDeclaration, bool isInterface)
{
    if (specifiers.empty()) {
        return;
    }
    auto source = importDeclaration->Source()->Str().Mutf8();
    source = RemoveModuleExtensionName(source);
    OutTs(isInterface ? "export type" : "export", " { ");

    GenSeparated(
        specifiers,
        [this, importDeclaration](ir::AstNode *specifier) {
            GenSingleNamedReExport(specifier, importDeclaration, true);
        },
        ", ", true, false);

    OutTs(" } from \"", source, "\";");
    OutEndlTs();
}

std::string TSDeclGen::ReplaceETSGLOBAL(const std::string &typeName)
{
    if (typeName.empty()) {
        return globalDesc_;
    }
    const std::string target = "ETSGLOBAL";
    std::size_t pos = globalDesc_.find(target);
    if (pos != std::string::npos) {
        return globalDesc_.substr(0, pos) + typeName + globalDesc_.substr(pos + target.length());
    }
    return globalDesc_;
}

std::string TSDeclGen::ConvertInteropTypeName(const std::string &typeName)
{
    if (typeName == "Array") {
        return "st.Array";
    } else if (typeName == "Map") {
        return "st.Map";
    } else if (typeName == "Set") {
        return "st.Set";
    } else if (typeName == "Class") {
        return "ESObject";
    } else if (typeName == "es.Array") {
        return "Array";
    } else if (typeName == "es.Map") {
        return "Map";
    } else if (typeName == "es.Set") {
        return "Set";
    } else {
        return typeName;
    }
}

bool TSDeclGen::ProcessTSQualifiedName(const ir::ETSTypeReference *typeReference)
{
    if (typeReference->Part()->Name()->IsTSQualifiedName() &&
        typeReference->Part()->Name()->AsTSQualifiedName()->Name() != nullptr) {
        const auto qualifiedName = typeReference->Part()->Name()->AsTSQualifiedName()->Name().Mutf8();
        AddImport(qualifiedName);
        std::istringstream stream(qualifiedName);
        std::string firstSegment;
        if (std::getline(stream, firstSegment, '.') && stdlibNamespaceList_.count(firstSegment) != 0U) {
            OutDts("ESObject");
            return true;
        }
        OutDts(ConvertInteropTypeName(qualifiedName));
        auto typeParams = typeReference->Part()->TypeParams();
        if (typeParams != nullptr && typeParams->IsTSTypeParameterInstantiation()) {
            OutDts("<");
            GenSeparated(typeParams->Params(),
                         [this](ir::TypeNode *param) { ProcessTypeAnnotationType(param, param->GetType(checker_)); });
            OutDts(">");
        }
        return true;
    }
    return false;
}

void TSDeclGen::ProcessETSTypeReferenceType(const ir::ETSTypeReference *typeReference, const checker::Type *checkerType)
{
    auto typePart = typeReference->Part();
    auto partName = typePart->GetIdent()->Name().Mutf8();
    if (partName == "Type" || partName == "Function0") {
        OutDts("ESObject");
        return;
    }
    AddImport(partName);
    if (typePart->TypeParams() != nullptr && typePart->TypeParams()->IsTSTypeParameterInstantiation()) {
        if (partName == "ReadonlyArray" || partName == "FixedArray" ||
            (typeReference->Parent()->Parent()->IsETSParameterExpression() &&
             typeReference->Parent()->Parent()->AsETSParameterExpression()->TypeAnnotation() != nullptr &&
             typeReference->Parent()->Parent()->AsETSParameterExpression()->TypeAnnotation()->IsReadonlyType() &&
             partName == "Array")) {
            GenArrayType(typePart->TypeParams()->Params()[0]->GetType(checker_));
        } else {
            OutDts(ConvertInteropTypeName(partName));
            OutDts("<");
            GenSeparated(typePart->TypeParams()->Params(),
                         [this](ir::TypeNode *param) { ProcessTypeAnnotationType(param, param->GetType(checker_)); });
            OutDts(">");
        }
    } else if (ProcessTSQualifiedName(typeReference)) {
        return;
    } else if (checkerType != nullptr && checkerType->IsETSFunctionType()) {
        OutDts(partName);
    } else {
        GenPartName(partName);
        OutDts(ConvertInteropTypeName(partName));
    }
}

bool TSDeclGen::ProcessTypeAnnotationSpecificTypes(const checker::Type *checkerType)
{
    if (checkerType == nullptr) {
        return false;
    }

    AddImport(checkerType->ToString());
    if (HandleBasicTypes(checkerType)) {
        return true;
    }
    switch (checker::ETSChecker::ETSType(checkerType)) {
        case checker::TypeFlag::ETS_VOID:
        case checker::TypeFlag::ETS_NULL:
        case checker::TypeFlag::ETS_UNDEFINED:
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::ETS_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NONNULLISH:
        case checker::TypeFlag::ETS_PARTIAL_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_READONLY:
            OutDts(checkerType->ToString());
            return true;
        case checker::TypeFlag::ETS_ANY:
            OutDts("ESObject");
            return true;
        default:
            return false;
    }
    return false;
}

void TSDeclGen::ProcessTypeAnnotationType(const ir::TypeNode *typeAnnotation, const checker::Type *checkerType)
{
    auto *aliasedType = const_cast<ir::TypeNode *>(typeAnnotation)->GetType(checker_);

    if (HasExplicitVoidAnnotation(typeAnnotation)) {
        OutDts("void");
        return;
    }
    if (typeAnnotation->IsTSThisType()) {
        OutDts("this");
        return;
    }
    if (typeAnnotation->IsETSPrimitiveType() &&
        typeAnnotation->AsETSPrimitiveType()->GetPrimitiveType() == ir::PrimitiveType::VOID) {
        OutDts("void");
        return;
    }
    if (typeAnnotation->IsETSStringLiteralType() && aliasedType != nullptr) {
        AddImport(aliasedType->ToString());
        OutDts(aliasedType->ToString());
        return;
    }
    if (typeAnnotation->IsETSTypeReference()) {
        ProcessETSTypeReference(typeAnnotation, checkerType);
        return;
    }
    if (typeAnnotation->IsETSTuple()) {
        ProcessETSTuple(typeAnnotation->AsETSTuple());
        return;
    }
    if (typeAnnotation->IsETSUnionType()) {
        ProcessETSUnionType(typeAnnotation->AsETSUnionType());
        return;
    }
    if (typeAnnotation->IsTSArrayType() && typeAnnotation->AsTSArrayType()->ElementType() != nullptr) {
        ProcessTSArrayType(typeAnnotation->AsTSArrayType());
        return;
    }
    if (typeAnnotation->IsETSFunctionType()) {
        ProcessETSFunctionType(typeAnnotation->AsETSFunctionType());
        return;
    }
    checkerType != nullptr ? GenType(checkerType) : GenType(aliasedType);
}

void TSDeclGen::ProcessETSTypeReference(const ir::TypeNode *typeAnnotation, const checker::Type *checkerType)
{
    if (ProcessTSQualifiedName(typeAnnotation->AsETSTypeReference())) {
        return;
    }
    if (ProcessTypeAnnotationSpecificTypes(checkerType)) {
        return;
    }
    if (checkerType != nullptr && typeAnnotation->AsETSTypeReference()->Part()->GetIdent()->Name().Is("Any")) {
        OutDts(typeAnnotation->Parent()->IsTSArrayType() ? "(" : "");
        GenType(checkerType);
        OutDts(typeAnnotation->Parent()->IsTSArrayType() ? ")" : "");
        return;
    }
    ProcessETSTypeReferenceType(typeAnnotation->AsETSTypeReference(), checkerType);
}

void TSDeclGen::ProcessETSTuple(const ir::ETSTuple *etsTuple)
{
    OutDts("[");
    GenSeparated(
        etsTuple->GetTupleTypeAnnotationsList(),
        [this](ir::TypeNode *arg) { ProcessTypeAnnotationType(arg, arg->GetType(checker_)); }, " , ");
    OutDts("]");
}

void TSDeclGen::ProcessETSUnionType(const ir::ETSUnionType *etsUnionType)
{
    state_.inUnionBodyStack.push(true);
    std::vector<ir::TypeNode *> filteredTypes = FilterUnionTypes(etsUnionType->Types());
    GenSeparated(
        filteredTypes, [this](ir::TypeNode *arg) { ProcessTypeAnnotationType(arg, arg->GetType(checker_)); }, " | ");
    state_.inUnionBodyStack.pop();
}

void TSDeclGen::ProcessTSArrayType(const ir::TSArrayType *tsArrayType)
{
    GenArrayType(const_cast<ir::TypeNode *>(tsArrayType->ElementType())->GetType(checker_));
}

void TSDeclGen::ProcessETSFunctionType(const ir::ETSFunctionType *etsFunction)
{
    if (etsFunction->TypeParams() != nullptr) {
        GenTypeParameters(etsFunction->TypeParams());
    }
    bool inUnionBody = !state_.inUnionBodyStack.empty() && state_.inUnionBodyStack.top();
    OutDts(inUnionBody ? "((" : "(");
    GenSeparated(etsFunction->Params(), [this](ir::Expression *param) {
        const auto paramExpr = param->AsETSParameterExpression();
        const auto paramName = paramExpr->Name();
        const bool isRestParam = paramExpr->IsRestParameter();
        const bool isOptional = paramExpr->IsOptional();
        OutDts(isRestParam ? "..." : "", paramName.Is("=t") ? "this" : paramName, isOptional ? "?: " : ": ");
        if (isRestParam) {
            ProcessRestParameterTypeAnnotationType(paramExpr->TypeAnnotation());
        } else {
            ProcessTypeAnnotationType(paramExpr->TypeAnnotation(), paramExpr->TypeAnnotation()->TsType());
        }
    });
    OutDts(") => ");
    ProcessTypeAnnotationType(etsFunction->ReturnType(), etsFunction->ReturnType()->TsType());
    OutDts(inUnionBody ? ")" : "");
}

void TSDeclGen::GenTypeAliasDeclaration(const ir::TSTypeAliasDeclaration *typeAlias)
{
    const auto name = typeAlias->Id()->Name().Mutf8();
    state_.currentTypeAliasName = name;
    state_.currentTypeParams = typeAlias->TypeParams();
    DebugPrint("GenTypeAliasDeclaration: " + name);
    if (!ShouldEmitDeclaration(typeAlias)) {
        return;
    }
    if (state_.inClass) {
        auto indent = GetIndent();
        OutDts(indent);
        OutTs(indent);
    }
    GenAnnotations(typeAlias);
    if (typeAlias->IsDefaultExported() || state_.inNamespace) {
        OutDts("type ", name);
    } else if (typeAlias->IsExported() || declgenOptions_.exportAll) {
        exportSet_.insert(name);
        OutDts("export type ", name);
    } else {
        OutDts("type ", name);
    }
    GenTypeParameters(typeAlias->TypeParams());
    OutDts(" = ");
    ProcessTypeAnnotationType(typeAlias->TypeAnnotation(), typeAlias->TypeAnnotation()->GetType(checker_));
    OutDts(";");
    OutEndlDts();
    if (typeAlias->IsDefaultExported()) {
        exportSet_.insert(name);
        OutDts("export default ", name, ";");
        OutEndlDts();
    }
}

void TSDeclGen::GenEnumDeclaration(const ir::ClassProperty *enumMember)
{
    const auto *originEnumMember = enumMember->OriginEnumMember();
    if (originEnumMember == nullptr) {
        return;
    }

    ProcessIndent();

    OutDts(GetKeyIdent(enumMember->Key())->Name());

    const auto *init = originEnumMember->Init();
    if (init != nullptr) {
        OutDts(" = ");
        if (!init->IsLiteral()) {
            LogError(diagnostic::NOT_LITERAL_ENUM_INITIALIZER, {}, init->Start());
        }

        GenLiteral(init->AsLiteral());
    }

    OutDts(",");
    OutEndlDts();
}

void TSDeclGen::GenInteropAnyInterface(const ir::TSInterfaceDeclaration *interfaceDecl,
                                       const std::string &interfaceName)
{
    const bool isDefault = interfaceDecl->IsDefaultExported();
    const bool isExported = interfaceDecl->IsExported() || declgenOptions_.exportAll;
    if (isDefault) {
        OutDts("type ", interfaceName, " = ESObject;");
        OutEndlDts();
        OutDts("export default ", interfaceName, ";");
        OutEndlDts();
        exportSet_.insert(interfaceName);
    } else if (isExported) {
        exportSet_.insert(interfaceName);
        OutDts("export type ", interfaceName, " = ESObject;");
        OutEndlDts();
    } else {
        OutDts("type ", interfaceName, " = ESObject;");
        OutEndlDts();
    }
}

void TSDeclGen::EmitInterfaceHeader(const ir::TSInterfaceDeclaration *interfaceDecl, const std::string &interfaceName)
{
    if (interfaceDecl->IsDefaultExported()) {
        if (state_.isInterfaceInNamespace) {
            OutDts("interface ", interfaceName);
        } else {
            exportSet_.insert(interfaceName);
            OutDts("export default interface ", interfaceName);
        }
    } else if (interfaceDecl->IsExported() || declgenOptions_.exportAll) {
        if (state_.isInterfaceInNamespace) {
            OutDts("interface ", interfaceName);
        } else {
            exportSet_.insert(interfaceName);
            OutDts("export declare interface ", interfaceName);
        }
    } else {
        OutDts(state_.isInterfaceInNamespace ? "interface " : "declare interface ", interfaceName);
    }
}

void TSDeclGen::EmitInterfaceExtends(const ir::TSInterfaceDeclaration *interfaceDecl)
{
    if (interfaceDecl->Extends().empty()) {
        return;
    }
    OutDts(" extends ");
    GenSeparated(interfaceDecl->Extends(), [this](ir::TSInterfaceHeritage *param) {
        if (param->Expr()->IsETSTypeReference()) {
            ProcessETSTypeReferenceType(param->Expr()->AsETSTypeReference());
        }
    });
}

void TSDeclGen::GenInterfaceDeclaration(const ir::TSInterfaceDeclaration *interfaceDecl)
{
    const auto interfaceName = interfaceDecl->Id()->Name().Mutf8();
    DebugPrint("GenInterfaceDeclaration: " + interfaceName);
    if (interfaceName.find("%%partial-") != std::string::npos) {
        return;
    }
    if (!ShouldEmitDeclaration(interfaceDecl)) {
        return;
    }
    const auto interfaceTags = jsdoc::CollectInteropTagsFromNode(interfaceDecl);
    if (interfaceTags.any) {
        GenInteropAnyInterface(interfaceDecl, interfaceName);
        return;
    }
    GenAnnotations(interfaceDecl);
    state_.inInterface = true;
    EmitInterfaceHeader(interfaceDecl, interfaceName);
    GenTypeParameters(interfaceDecl->TypeParams());
    if (!interfaceTags.breakExtends) {
        EmitInterfaceExtends(interfaceDecl);
    }
    OutDts(" {");
    OutEndlDts();
    ProcessInterfaceBody(interfaceDecl->Body());
    if (state_.isInterfaceInNamespace) {
        classNode_.indentLevel--;
        OutDts(GetIndent());
    }
    OutDts("}");
    OutEndlDts();
}

void TSDeclGen::ProcessInterfaceBody(const ir::TSInterfaceBody *body)
{
    for (auto *prop : body->Body()) {
        if (prop->IsMethodDefinition()) {
            ProcessInterfaceMethodDefinition(prop->AsMethodDefinition());
        }
    }
}

void TSDeclGen::ProcessInterfaceMethodDefinition(const ir::MethodDefinition *methodDef)
{
    if (GenInterfaceProp(methodDef)) {
        return;
    }

    if (methodDef->IsGetter() || methodDef->IsSetter()) {
        GenMethodDeclaration(methodDef);
    }
    if (!methodDef->Overloads().empty()) {
        for (const auto *overloadMethd : methodDef->Overloads()) {
            if (overloadMethd->IsGetter() || overloadMethd->IsSetter()) {
                GenMethodDeclaration(overloadMethd);
            }
        }
        return;
    }
    if (!methodDef->IsGetter() && !methodDef->IsSetter()) {
        GenMethodDeclaration(methodDef);
    }
}

bool TSDeclGen::GenInterfaceProp(const ir::MethodDefinition *methodDef)
{
    if (!methodDef->IsGetter()) {
        return false;
    }
    if (methodDef->OriginalNode() == nullptr) {
        return false;
    }
    if (!methodDef->OriginalNode()->IsClassProperty()) {
        return false;
    }

    GenAnnotations(methodDef->Function());
    const auto methodName = GetKeyIdent(methodDef->Key())->Name().Mutf8();
    const auto classProp = methodDef->OriginalNode()->AsClassProperty();
    ProcessIndent();
    if (classProp->IsReadonly()) {
        OutDts("readonly ");
    }
    OutDts(methodName);
    if (classProp->IsOptionalDeclaration()) {
        OutDts("?");
    }
    OutDts(": ");
    if (methodDef->TsType()->IsETSFunctionType()) {
        const auto *sig = GetFuncSignature(methodDef->TsType()->AsETSFunctionType(), methodDef);
        ProcessFunctionReturnType(sig);
    } else {
        ES2PANDA_ASSERT(methodDef->Function() != nullptr);
        GenType(methodDef->Function()->Signature()->ReturnType());
    }

    OutDts(";");
    OutEndlDts();
    return true;
}

void TSDeclGen::ProcessMethodDefinition(const ir::MethodDefinition *methodDef,
                                        std::unordered_set<std::string> &processedMethods)
{
    const auto methodName = GetKeyIdent(methodDef->Key())->Name().Mutf8();
    if (processedMethods.find(methodName) != processedMethods.end()) {
        return;
    }
    if (methodDef->IsGetter() || methodDef->IsSetter()) {
        GenMethodDeclaration(methodDef);
        processedMethods.insert(methodName);
    }
    if (!methodDef->Overloads().empty() && !methodDef->IsConstructor()) {
        for (const auto *overloadMethd : methodDef->Overloads()) {
            if (overloadMethd->IsGetter() || overloadMethd->IsSetter()) {
                GenMethodDeclaration(overloadMethd);
            }
        }
        return;
    }
    if (!methodDef->IsGetter() && !methodDef->IsSetter()) {
        GenMethodDeclaration(methodDef);
        processedMethods.insert(methodName);
    }
}

void TSDeclGen::PrepareClassDeclaration(const ir::ClassDefinition *classDef)
{
    std::string classDescriptor = "L" + classDef->InternalName().Mutf8() + ";";
    std::replace(classDescriptor.begin(), classDescriptor.end(), '.', '/');
    state_.currentClassDescriptor = classDescriptor;
    state_.inGlobalClass = classDef->IsGlobal();
    if (classDef->IsNamespaceTransformed()) {
        state_.inNamespace = true;
        state_.isClassInNamespace = false;
        state_.isDeclareNamespace = classDef->IsDeclare();
    } else {
        state_.isClassInNamespace = true;
    }
    classNode_.isStruct = classDef->IsFromStruct();
}

bool TSDeclGen::ShouldSkipClassDeclaration(const std::string_view &className) const
{
    return className.find("%%partial-") != std::string::npos;
}

void TSDeclGen::EmitDeclarationPrefix(const ir::ClassDefinition *classDef, const std::string &typeName,
                                      const std::string_view &className)
{
    if (classDef->IsDefaultExported()) {
        OutDts(classNode_.indentLevel > 1 ? typeName : "declare " + typeName, className);
    } else if (classDef->IsExported() || declgenOptions_.exportAll) {
        if (classNode_.indentLevel > 1) {
            OutDts(typeName, className);
        } else {
            exportSet_.insert(std::string(className));
            OutDts("export declare " + typeName, className);
        }
    } else {
        OutDts(classNode_.indentLevel > 1 ? typeName : "declare " + typeName, className);
    }
}

void TSDeclGen::EmitClassDeclaration(const ir::ClassDefinition *classDef, const std::string_view &className)
{
    if (classDef->IsNamespaceTransformed()) {
        EmitDeclarationPrefix(classDef, "namespace ", className);
        classDef->IsDefaultExported() ? OutTs("namespace ", className, " {")
                                      : OutTs("export namespace ", className, " {");
        OutEndlTs();
    } else if (classDef->IsEnumTransformed()) {
        EmitDeclarationPrefix(classDef, "enum ", className);
    } else if (classDef->IsFromStruct()) {
        EmitDeclarationPrefix(classDef, "struct ", className);
    } else if (classDef->IsAbstract()) {
        EmitDeclarationPrefix(classDef, "abstract class ", className);
    } else {
        EmitDeclarationPrefix(classDef, "class ", className);
    }
}

std::string TSDeclGen::GetIndent() const
{
    return std::string(classNode_.indentLevel * INDENT.size(), ' ');
}

void TSDeclGen::GenPartName(std::string &partName)
{
    if (partName == "Boolean") {
        partName = "boolean";
    } else if (stringTypes_.count(partName) != 0U) {
        partName = "string";
    } else if (numberTypes_.count(partName) != 0U) {
        partName = "number";
    } else if (partName == "ESValue") {
        partName = "ESObject";
    } else if (partName == "BigInt") {
        partName = "bigint";
    } else if (partName == "Exception" || partName == "NullPointerError") {
        partName = "Error";
    } else if (partName == "Any") {
        partName = "ESObject";
    } else if (partName == "Floating" || partName == "Integral") {
        partName = "number";
    } else if (partName == "Class") {
        partName = "ESObject";
    }
}

void TSDeclGen::ProcessIndent()
{
    if (state_.isInterfaceInNamespace || state_.inEnum) {
        auto indent = GetIndent();
        OutDts(GetIndent());
    } else if (classNode_.hasNestedClass || state_.inNamespace) {
        auto indent = GetIndent();
        OutDts(indent);
        OutTs(indent);
    } else {
        OutDts(INDENT);
    }
}

void TSDeclGen::HandleClassDeclarationTypeInfo(const ir::ClassDefinition *classDef, const std::string_view &className)
{
    if (classNode_.hasNestedClass) {
        classNode_.indentLevel--;
        ES2PANDA_ASSERT(classNode_.indentLevel != static_cast<decltype(classNode_.indentLevel)>(-1));
    }
    GenAnnotations(classDef);
    if (classNode_.hasNestedClass) {
        OutDts(GetIndent());
        classNode_.indentLevel++;
    }
    EmitClassDeclaration(classDef, className);
    GenTypeParameters(classDef->TypeParams());

    const auto classTags = jsdoc::CollectInteropTagsFromNode(classDef->Parent());
    const auto *super = classDef->Super();
    if (!classTags.breakExtends && super != nullptr && !classDef->IsEnumTransformed()) {
        OutDts(" extends ");
        HandleClassInherit(super);
    }

    if (classTags.breakImplements) {
        // skip implements per @interop break-implements
    } else if (!classDef->Implements().empty()) {
        OutDts(" implements ");
        GenSeparated(classDef->Implements(), [this](ir::TSClassImplements *impl) { HandleClassInherit(impl->Expr()); });
    } else if (classDef->TsType() != nullptr && classDef->TsType()->IsETSObjectType() &&
               !classDef->TsType()->AsETSObjectType()->Interfaces().empty()) {
        OutDts(" implements ");
        const auto &interfaces = classDef->TsType()->AsETSObjectType()->Interfaces();
        GenSeparated(interfaces, [this](checker::ETSObjectType *interface) { GenType(interface); });
    }

    OutDts(" {");
    OutEndlDts();
}

void TSDeclGen::HandleClassInherit(const ir::Expression *expr)
{
    if (expr->IsETSTypeReference()) {
        ProcessETSTypeReferenceType(expr->AsETSTypeReference());
    } else if (!expr->TsType()->IsTypeError()) {
        GenType(expr->TsType());
    }
}

void TSDeclGen::EmitClassGlueCode(const ir::ClassDefinition *classDef, const std::string &className)
{
    if (!classDef->IsExported() && !classDef->IsDefaultExported() && !declgenOptions_.exportAll) {
        return;
    }
    if (classDef->IsExportedType()) {
        return;
    }
    const std::string exportPrefix = classDef->Parent()->IsDefaultExported() ? "const " : "export const ";
    OutTs(exportPrefix, className, " = (globalThis as any).Panda.getClass('", state_.currentClassDescriptor, "');");
    OutEndlTs();

    if (classDef->Parent()->IsDefaultExported()) {
        OutTs("export default ", className, ";");
        OutEndlTs();
    }
}

void TSDeclGen::ProcessClassBody(const ir::ClassDefinition *classDef)
{
    state_.inClass = true;
    std::unordered_set<std::string> processedStaticMethods;
    std::unordered_set<std::string> processedInstanceMethods;
    for (const auto *prop : classDef->Body()) {
        if (jsdoc::CollectInteropTagsFromNode(prop).noninterop) {
            continue;
        }
        if (classDef->IsEnumTransformed()) {
            if (prop->IsClassProperty()) {
                state_.inEnum = true;
                GenPropDeclaration(prop->AsClassProperty());
            }
        } else if (prop->IsTSInterfaceDeclaration()) {
            state_.isInterfaceInNamespace = true;
            OutDts(GetIndent());
            classNode_.indentLevel++;
            GenInterfaceDeclaration(prop->AsTSInterfaceDeclaration());
            state_.inInterface = false;
            state_.isInterfaceInNamespace = false;
        } else if (prop->IsTSTypeAliasDeclaration()) {
            GenTypeAliasDeclaration(prop->AsTSTypeAliasDeclaration());
        } else if (prop->IsMethodDefinition()) {
            const ir::MethodDefinition *methodDef = prop->AsMethodDefinition();
            ProcessMethodDefinition(methodDef,
                                    methodDef->IsStatic() ? processedStaticMethods : processedInstanceMethods);
        } else if (prop->IsClassProperty()) {
            const auto classProp = prop->AsClassProperty();
            if (classProp->Modifiers() & ir::ModifierFlags::GETTER_SETTER) {
                continue;
            }
            GenPropDeclaration(classProp);
        } else if (prop->IsClassDeclaration() && classDef->IsNamespaceTransformed()) {
            classNode_.hasNestedClass = true;
            OutTs(GetIndent());
            classNode_.indentLevel++;
            GenClassDeclaration(prop->AsClassDeclaration());
            state_.isClassInNamespace = false;
        } else if (prop->IsClassDeclaration() && classDef->IsFromStruct()) {
            GenClassDeclaration(prop->AsClassDeclaration());
        }
    }
}

void TSDeclGen::CloseClassBlock(const bool isDts)
{
    auto indent = GetIndent();
    if (isDts) {
        OutDts(indent, "}");
        OutEndlDts();
    } else {
        OutTs(indent, "}");
        OutEndlTs();
    }
}

void TSDeclGen::EmitInteropAnyClass(const ir::ClassDefinition *classDef, const std::string &className)
{
    const bool isDefault = classDef->IsDefaultExported();
    const bool isExported = classDef->IsExported() || declgenOptions_.exportAll;
    if (isDefault) {
        OutDts("type ", className, " = ESObject;");
        OutEndlDts();
        OutDts("export default ", className, ";");
        OutEndlDts();
        exportSet_.insert(className);
    } else if (isExported) {
        exportSet_.insert(className);
        OutDts("export type ", className, " = ESObject;");
        OutEndlDts();
    } else {
        OutDts("type ", className, " = ESObject;");
        OutEndlDts();
    }
}

void TSDeclGen::EmitNonGlobalClassDeclaration(const ir::ClassDefinition *classDef, const std::string &className)
{
    HandleClassDeclarationTypeInfo(classDef, className);
    if (!classDef->IsNamespaceTransformed()) {
        EmitClassGlueCode(classDef, className);
    }
    ProcessClassBody(classDef);
    std::size_t originalIndentLevel = classNode_.indentLevel;
    classNode_.indentLevel > 0 ? classNode_.indentLevel-- : classNode_.indentLevel = 0;
    CloseClassBlock(true);
    classNode_.indentLevel = originalIndentLevel;
}

void TSDeclGen::CloseNestedClassBlock(const ir::ClassDefinition *classDef)
{
    classNode_.indentLevel > 1 ? classNode_.indentLevel-- : classNode_.indentLevel = 1;
    if (!ShouldEmitDeclaration(classDef)) {
        return;
    }
    ES2PANDA_ASSERT(classNode_.indentLevel != static_cast<decltype(classNode_.indentLevel)>(-1));
    if (!state_.isClassInNamespace) {
        CloseClassBlock(false);
    }
    if (state_.inEnum) {
        state_.inEnum = false;
    }
}

void TSDeclGen::EmitDefaultExportedClass(const ir::ClassDefinition *classDef, const std::string &className)
{
    exportSet_.insert(className);
    OutDts("export default ", className, ";");
    OutEndlDts();
    if (classDef->IsNamespaceTransformed()) {
        OutTs("export default ", className, ";");
        OutEndlTs();
    }
}

void TSDeclGen::GenClassDeclaration(const ir::ClassDeclaration *classDecl)
{
    const auto *classDef = classDecl->Definition();
    PrepareClassDeclaration(classDef);
    const auto className = classDef->Ident()->Name().Mutf8();
    DebugPrint("GenClassDeclaration: " + className);
    if (ShouldSkipClassDeclaration(className)) {
        return;
    }
    if (!state_.inGlobalClass && ShouldEmitDeclaration(classDef) && jsdoc::CollectInteropTagsFromNode(classDecl).any) {
        EmitInteropAnyClass(classDef, className);
        return;
    }
    if (state_.inGlobalClass) {
        classNode_.indentLevel = 1;
        ProcessClassBody(classDef);
    }
    if (!state_.inGlobalClass && ShouldEmitDeclaration(classDef)) {
        EmitNonGlobalClassDeclaration(classDef, className);
    }
    if (classNode_.hasNestedClass || state_.inNamespace || state_.inEnum) {
        CloseNestedClassBlock(classDef);
    }
    if (classDef->IsDefaultExported()) {
        EmitDefaultExportedClass(classDef, className);
    }
}

bool TSDeclGen::ShouldSkipMethodDeclaration(const ir::MethodDefinition *methodDef)
{
    const auto methodIdent = GetKeyIdent(methodDef->Key());
    const auto methodName = methodIdent->Name().Mutf8();
    if (methodName.find('#') != std::string::npos || methodName.find("%%async-") != std::string::npos ||
        (!state_.inGlobalClass && (methodName == compiler::Signatures::INIT_METHOD ||
                                   methodName == compiler::Signatures::INITIALIZER_BLOCK_INIT))) {
        return true;
    }
    if (methodDef->IsPrivate() && (methodDef->IsConstructor() || state_.inInterface)) {
        return true;
    }
    if (methodName == compiler::Signatures::INIT_METHOD) {
        return true;
    }
    if (classNode_.isStruct && methodDef->IsConstructor()) {
        return true;
    }
    return false;
}

void TSDeclGen::EmitMethodGlueCode(const std::string &methodName, const ir::Identifier *methodIdentifier)
{
    if (!state_.inGlobalClass && (!state_.inNamespace || state_.isClassInNamespace || state_.isInterfaceInNamespace)) {
        return;
    }
    if (!ShouldEmitDeclaration(methodIdentifier->Parent())) {
        return;
    }
    if (state_.inNamespace) {
        OutTs("export const ", methodName,
              " = (globalThis as any).Panda.getClass('" + state_.currentClassDescriptor + "')." + methodName + ";");
        OutEndlTs();
        return;
    }
    if (methodIdentifier->Parent()->IsDefaultExported()) {
        OutTs("const ", methodName, " = (globalThis as any).Panda.getFunction('", state_.currentClassDescriptor, "', '",
              methodName, "');");
        OutEndlTs();
        OutTs("export default ", methodName, ";");
        OutEndlTs();
    } else {
        OutTs("export const ", methodName, " = (globalThis as any).Panda.getFunction('", state_.currentClassDescriptor,
              "', '", methodName, "');");
        OutEndlTs();
    }
}

void TSDeclGen::GenMethodDeclaration(const ir::MethodDefinition *methodDef)
{
    if (ShouldSkipMethodDeclaration(methodDef)) {
        return;
    }
    GenAnnotations(methodDef->Function());
    const auto methodIdent = GetKeyIdent(methodDef->Key());
    auto methodName = methodIdent->Name().Mutf8();
    if (methodName == "$_iterator") {
        methodName = "[Symbol.iterator]";
    }

    auto savedRet = std::move(interopRetOverride_);
    auto savedParams = std::move(interopParamOverrides_);
    auto methodTags = jsdoc::CollectInteropTagsFromNode(methodDef);
    interopRetOverride_ = std::move(methodTags.retOverride);
    interopParamOverrides_ = std::move(methodTags.paramOverrides);
    auto restore = [this, &savedRet, &savedParams]() {
        interopRetOverride_ = std::move(savedRet);
        interopParamOverrides_ = std::move(savedParams);
    };

    if (GenMethodDeclarationPrefix(methodDef, methodIdent, methodName)) {
        restore();
        return;
    }
    GenMethodSignature(methodDef, methodIdent, methodName);

    OutDts(";");
    OutEndlDts();

    if (methodDef->IsDefaultExported()) {
        exportSet_.insert(methodName);
        OutDts("export default ", methodName, ";");
        OutEndlDts();
    }
    restore();
}

bool TSDeclGen::GenMethodDeclarationPrefix(const ir::MethodDefinition *methodDef, const ir::Identifier *methodIdent,
                                           const std::string &methodName)
{
    if (state_.inGlobalClass) {
        if (!ShouldEmitDeclaration(methodDef)) {
            return true;
        }
        GenAnnotations(methodDef->Function());
        if (methodDef->IsDefaultExported()) {
            OutDts("declare function ");
        } else {
            exportSet_.insert(methodName);
            OutDts("export declare function ");
        }
    } else {
        if (state_.inNamespace && !state_.isClassInNamespace && !state_.isInterfaceInNamespace &&
            !ShouldEmitDeclaration(methodDef) && !methodDef->IsConstructor()) {
            return true;
        }
        GenAnnotations(methodDef->Function());
        ProcessIndent();
        GenModifier(methodDef);
    }
    EmitMethodGlueCode(methodName, methodIdent);

    ES2PANDA_ASSERT(methodDef->Function() != nullptr);
    if (methodDef->Function()->IsAbstract() && !state_.inInterface &&
        !(methodDef->Parent()->IsTSInterfaceBody() ||
          (methodDef->BaseOverloadMethod() != nullptr &&
           methodDef->BaseOverloadMethod()->Parent()->IsTSInterfaceBody()))) {
        OutDts("abstract ");
    }
    if (methodDef->IsGetter()) {
        OutDts("get ");
    }
    if (methodDef->IsSetter()) {
        OutDts("set ");
    }
    return false;
}

void TSDeclGen::GenMethodSignature(const ir::MethodDefinition *methodDef, const ir::Identifier *methodIdent,
                                   const std::string &methodName)
{
    if (methodDef->IsSetter()) {
        OutDts(methodName, "(value: ");
        const checker::Signature *sig = nullptr;
        if (methodDef->TsType() != nullptr && methodDef->TsType()->IsETSFunctionType()) {
            sig = GetFuncSignature(methodDef->TsType()->AsETSFunctionType(), methodDef);
        } else if (methodDef->Function() != nullptr) {
            sig = methodDef->Function()->Signature();
        }

        if (sig == nullptr) {
            LogWarning(diagnostic::UNTYPED_METHOD, {methodName}, methodIdent->Start());
            OutDts("ESObject");
        } else if (sig->HasFunction()) {
            ProcessFunctionReturnType(sig);
        } else if (!sig->Params().empty()) {
            GenType(sig->Params()[0]->TsType());
        } else {
            OutDts("ESObject");
        }
        OutDts(")");
    } else {
        DebugPrint("  GenMethodDeclaration: " + methodName);
        OutDts(methodName);

        if (methodDef->TsType() == nullptr) {
            LogWarning(diagnostic::UNTYPED_METHOD, {methodName}, methodIdent->Start());
            OutDts(": ESObject");
            return;
        }
        if (methodDef->TsType()->IsETSFunctionType()) {
            GenFunctionType(methodDef->TsType()->AsETSFunctionType(), methodDef);
        }
    }
}

void TSDeclGen::EmitPropGlueCode(const ir::ClassProperty *classProp, const std::string &propName)
{
    std::string propAccess;
    if (state_.inGlobalClass) {
        propAccess = " = (globalThis as any).Panda.getClass('" + globalDesc_ + "')." + propName + ";";
    } else {
        propAccess = " = (globalThis as any).Panda.getClass('" + state_.currentClassDescriptor + "')." + propName + ";";
    }
    const bool isConst = classProp->IsConst();
    const bool CheckIsDefaultExported = classProp->IsDefaultExported();
    if (CheckIsDefaultExported) {
        OutTs(isConst ? "const " : "let ", propName, propAccess);
        OutEndlTs();
        OutTs("export default ", propName, ";");
    } else {
        OutTs(isConst ? "export const " : "export let ", propName, propAccess);
    }
    OutEndlTs();
}

void TSDeclGen::ProcessClassPropertyType(const ir::ClassProperty *classProp)
{
    auto value = classProp->AsClassProperty()->Value();
    if (value != nullptr && value->IsETSNewClassInstanceExpression() &&
        value->AsETSNewClassInstanceExpression()->GetTypeRef() != nullptr &&
        value->AsETSNewClassInstanceExpression()->GetTypeRef()->IsETSTypeReference()) {
        auto typeReference = classProp->Value()->AsETSNewClassInstanceExpression()->GetTypeRef()->AsETSTypeReference();
        ProcessETSTypeReferenceType(typeReference);
        return;
    }
    if (classProp->TypeAnnotation() != nullptr) {
        ProcessTypeAnnotationType(classProp->TypeAnnotation(), classProp->TsType());
        return;
    }
    if (value != nullptr && value->IsArrowFunctionExpression() &&
        value->AsArrowFunctionExpression()->Function() != nullptr &&
        value->AsArrowFunctionExpression()->Function()->TypeParams() != nullptr) {
        GenTypeParameters(value->AsArrowFunctionExpression()->Function()->TypeParams());
    }
    GenType(classProp->TsType());
}

void TSDeclGen::GenPropDeclaration(const ir::ClassProperty *classProp)
{
    if (state_.inGlobalClass) {
        GenGlobalVarDeclaration(classProp);
        return;
    }
    if (classProp->IsPrivate()) {
        return;
    }

    const auto propName = GetKeyIdent(classProp->Key())->Name().Mutf8();
    // The class property generated for enums starts with "#" are invalid properties, and should not be generated.
    if (propName.find('#') != std::string::npos) {
        DebugPrint("  Skip Generate enum PropDeclaration: " + propName);
        return;
    }

    if (state_.inEnum) {
        GenEnumDeclaration(classProp);
        return;
    }

    DebugPrint("  GenPropDeclaration: " + propName);

    ProcessClassPropDeclaration(classProp);

    if (classNode_.hasNestedClass || state_.inNamespace) {
        EmitPropGlueCode(classProp, propName);
    }
}

bool TSDeclGen::HasUIAnnotation(const ir::ClassProperty *classProp) const
{
    if (!declgenOptions_.genAnnotations || classProp == nullptr) {
        return false;
    }

    if (!classProp->HasAnnotations() && classProp->Annotations().empty()) {
        return false;
    }

    const auto &annotations = classProp->Annotations();
    return std::any_of(annotations.begin(), annotations.end(), [this](const auto &annotation) {
        return annotationList_.count(annotation->GetBaseName()->Name().Mutf8()) > 0;
    });
}

void TSDeclGen::ProcessClassPropDeclaration(const ir::ClassProperty *classProp)
{
    if (!state_.inInterface && (!state_.inNamespace || state_.isClassInNamespace) && !classNode_.isStruct &&
        !HasUIAnnotation(classProp)) {
        GenPropAccessor(classProp, "get ");
        if (!classProp->IsReadonly()) {
            GenPropAccessor(classProp, "set ");
        }
    } else {
        GenAnnotations(classProp);
        ProcessIndent();
        if (!classNode_.isStruct) {
            GenModifier(classProp, true);
        }
        const auto propName = GetKeyIdent(classProp->Key())->Name().Mutf8();
        OutDts(propName);
        OutDts(": ");
        if (!state_.inNamespace) {
            auto typeAnnotation = classProp->TypeAnnotation();
            classProp->IsStatic() ? OutDts("ESObject") : ProcessTypeAnnotationType(typeAnnotation, classProp->TsType());
        } else {
            ProcessClassPropertyType(classProp);
        }
        OutDts(";");
        OutEndlDts();
    }
}

void TSDeclGen::GenPropAccessor(const ir::ClassProperty *classProp, const std::string &accessorKind)
{
    if (accessorKind != "set " && accessorKind != "get ") {
        return;
    }
    GenAnnotations(classProp);
    ProcessIndent();
    GenModifier(classProp);

    const auto propName = GetKeyIdent(classProp->Key())->Name().Mutf8();
    OutDts(accessorKind, propName, accessorKind == "set " ? "(value: " : "(): ");
    auto typeAnnotation = classProp->TypeAnnotation();
    auto tsType = classProp->TsType();
    if (typeAnnotation != nullptr) {
        ProcessTypeAnnotationType(typeAnnotation, tsType);
    } else {
        GenType(tsType);
    }
    OutDts(accessorKind == "set " ? ");" : ";");
    OutEndlDts();
}

void TSDeclGen::GenGlobalVarDeclaration(const ir::ClassProperty *globalVar)
{
    if (!globalVar->IsExported() && !globalVar->IsDefaultExported() && !declgenOptions_.exportAll) {
        return;
    }

    const auto symbol = GetKeyIdent(globalVar->Key());
    auto varName = symbol->Name().Mutf8();
    const std::string prefix = "gensym%%_";
    if (varName.find(prefix, 0U) == 0U) {
        varName = varName.substr(prefix.size());
    }
    const bool isConst = globalVar->IsConst();
    const bool CheckIsDefaultExported = globalVar->IsDefaultExported();
    DebugPrint("GenGlobalVarDeclaration: " + varName);

    GenAnnotations(globalVar);

    exportSet_.insert(varName);
    if (CheckIsDefaultExported) {
        OutDts(isConst ? "declare const " : "declare let ", varName, ": ");
    } else {
        OutDts(isConst ? "export declare const " : "export declare let ", varName, ": ");
    }
    ProcessClassPropertyType(globalVar);
    OutDts(";");
    OutEndlDts();
    if (CheckIsDefaultExported) {
        OutDts("export default ", varName, ";");
        OutEndlDts();
    }

    EmitPropGlueCode(globalVar, varName);
}

void TSDeclGen::AddImport(const std::string &qualifiedName)
{
    // only top level can be imported
    importSet_.insert(qualifiedName.substr(0, qualifiedName.find('.')));
}

bool TSDeclGen::IsImport(const ir::AstNode *specifier)
{
    if (specifier->IsImportNamespaceSpecifier()) {
        return IsImport(specifier->AsImportNamespaceSpecifier()->Local());
    }
    if (specifier->IsImportDefaultSpecifier()) {
        return IsImport(specifier->AsImportDefaultSpecifier()->Local());
    }
    if (specifier->IsImportSpecifier()) {
        return IsImport(specifier->AsImportSpecifier()->Local());
    }
    return false;
}

bool TSDeclGen::IsImport(const ir::Identifier *identifier)
{
    return IsImport(identifier->Name().Mutf8());
}

bool TSDeclGen::IsImport(const std::string &name)
{
    return importSet_.find(name) != importSet_.end();
}

void TSDeclGen::AddDependency(const ir::AstNode *astNode)
{
    if (astNode == nullptr) {
        return;
    }

    if (astNode->IsETSTypeReference()) {
        AddDependency(astNode->AsETSTypeReference()->TsType());
    }
}

void TSDeclGen::AddDependency(const checker::Type *tsType)
{
    if (tsType == nullptr || IsDependency(tsType)) {
        return;
    }

    if (tsType->IsETSObjectType()) {
        if (tsType->IsETSResizableArrayType()) {
            AddDependency(tsType->AsETSResizableArrayType()->ElementType());
            return;
        }
        const auto objectType = tsType->AsETSObjectType();
        const auto typeName = objectType->AssemblerName().Mutf8();
        if (typeName.empty() || typeName.find("std.core.") != std::string::npos) {
            return;
        }
        AddDependency(typeName);

        const auto node = objectType->GetDeclNode();
        if (node == nullptr) {
            return;
        }
        CollectDependencies(node);
    } else if (tsType->IsETSUnionType()) {
        const auto unionType = tsType->AsETSUnionType();
        const auto filteredTypes = FilterUnionTypes(unionType->ConstituentTypes());
        GenSeparated(
            filteredTypes, [this](checker::Type *filteredType) { AddDependency(filteredType); }, "");
    }
}

void TSDeclGen::AddDependency(const std::string &assemblerName)
{
    // ensure declarations of all levels will be generated
    size_t partStart = 0;
    size_t partEnd = 0;
    while (partStart < assemblerName.size()) {
        partEnd = assemblerName.find('.', partStart);
        if (partEnd == std::string::npos) {
            partEnd = assemblerName.size();
        }
        std::string str = assemblerName.substr(0, partEnd);
        dependencySet_.insert(str);
        partStart = partEnd + 1;
    }
}

bool TSDeclGen::IsDependency(const ir::AstNode *decl)
{
    if (decl == nullptr) {
        return false;
    }

    if (decl->IsTSTypeAliasDeclaration()) {
        return IsDependency(decl->AsTSTypeAliasDeclaration()->TypeAnnotation()->TsType());
    }
    if (decl->IsClassDeclaration()) {
        return IsDependency(decl->AsClassDeclaration()->Definition()->TsType());
    }
    if (decl->IsClassDefinition()) {
        return IsDependency(decl->AsClassDefinition()->TsType());
    }
    if (decl->IsTSInterfaceDeclaration()) {
        return IsDependency(decl->AsTSInterfaceDeclaration()->TsType());
    }

    return false;
}

bool TSDeclGen::IsDependency(const checker::Type *tsType)
{
    if (tsType == nullptr) {
        return false;
    }

    if (tsType->IsETSObjectType()) {
        const auto objectType = tsType->AsETSObjectType();
        const auto typeName = objectType->AssemblerName().Mutf8();
        if (typeName.empty()) {
            return false;
        }
        return IsDependency(typeName);
    }
    if (tsType->IsETSUnionType()) {
        const auto unionType = tsType->AsETSUnionType();
        bool isDependency = false;
        GenSeparated(
            unionType->ConstituentTypes(),
            [this, &isDependency](checker::Type *constituentType) {
                isDependency = isDependency || IsDependency(constituentType);
            },
            "");
        return isDependency;
    }

    return false;
}

bool TSDeclGen::IsDependency(const std::string &assemblerName)
{
    return dependencySet_.find(assemblerName) != dependencySet_.end();
}

void TSDeclgenContent::PushImports(const std::string &importStr)
{
    imports_ += importStr;
}

void TSDeclgenContent::PushExports(const std::string &exportStr)
{
    exports_ += exportStr;
}

void TSDeclgenContent::PushStatements(const std::string &statementStr)
{
    statements_ += statementStr;
}

void TSDeclgenContent::PushInitModuleGlues(const std::string &initModuleGlueStr)
{
    initModuleGlues_ += initModuleGlueStr;
}

void TSDeclgenContent::PushRecordImports(const std::string &recordImportStr)
{
    recordImports_ += recordImportStr;
}

bool TSDeclgenContent::WriteToFile(const std::string &path)
{
    std::ofstream outStream(path);
    if (outStream.fail()) {
        return false;
    }
    outStream << recordImports_ << imports_ << initModuleGlues_ << statements_ << exports_;
    outStream.close();
    if (outStream.good()) {
        return true;
    } else {
        return false;
    }
}

void TSDeclgenContent::RemoveDuplicateExports(const std::string &checkExports)
{
    std::string result;
    std::string line;
    std::istringstream iss(exports_);

    while (std::getline(iss, line)) {
        bool shouldRemove = false;

        if (line.find("export default") != std::string::npos) {
            shouldRemove = true;
        } else if ((line.find("export {") != std::string::npos || line.find("export type {") != std::string::npos) &&
                   line.find("}") != std::string::npos) {
            shouldRemove = HasLocalDeclaredExport(line, checkExports);
        }

        if (!shouldRemove) {
            result += line + "\n";
        }
    }

    exports_ = result;
}

bool TSDeclgenContent::HasLocalDeclaredExport(const std::string &line, const std::string &checkExports)
{
    size_t start = line.find("{");
    size_t end = line.find("}");
    if (start == std::string::npos || end == std::string::npos) {
        return false;
    }

    std::string inner = line.substr(start + 1, end - start - 1);
    std::istringstream nameStream(inner);
    std::string token;

    while (std::getline(nameStream, token, ',')) {
        std::string name = token;
        name.erase(0, name.find_first_not_of(" \t\n\r"));
        name.erase(name.find_last_not_of(" \t\n\r") + 1);

        if (checkExports.find("export declare ") != std::string::npos && checkExports.find(name) != std::string::npos) {
            return true;
        }
    }

    return false;
}

TSDeclGenerator::TSDeclGenerator(public_lib::Context *ctx)
    : context_(ctx),
      tsContent_(),
      dtsContent_(),
      inputFileIndexMap_(),
      outputDeclEts_(),
      outputEts_(),
      isSimultaneousMode_(false),
      fileContents_()
{
    auto checker = reinterpret_cast<checker::ETSChecker *>(ctx->GetChecker());
    auto program = ctx->parserProgram;
    isolatedDeclgenChecker_ = std::make_unique<declgen::IsolatedDeclgenChecker>(checker->DiagnosticEngine(), *program);
    this->declBuilder_ = std::make_unique<TSDeclGen>(checker, isolatedDeclgenChecker_.get(), program);
}

TSDeclGenerator::TSDeclGenerator(public_lib::Context *ctx,
                                 const std::unordered_map<std::string, size_t> &inputFileIndexMap,
                                 const char *const *outputDeclEts, const char *const *outputEts, size_t fileNamesCount)
    : context_(ctx),
      tsContent_(),
      dtsContent_(),
      inputFileIndexMap_(inputFileIndexMap),
      outputDeclEts_(),
      outputEts_(),
      isSimultaneousMode_(ctx->config->options->IsSimultaneous()),
      fileContents_()
{
    auto checker = reinterpret_cast<checker::ETSChecker *>(ctx->GetChecker());
    for (size_t i = 0; i < fileNamesCount; ++i) {
        outputDeclEts_.push_back(outputDeclEts[i] ? outputDeclEts[i] : "");
        outputEts_.push_back(outputEts[i] ? outputEts[i] : "");
    }
    if (!isSimultaneousMode_) {
        auto program = ctx->parserProgram;
        isolatedDeclgenChecker_ =
            std::make_unique<declgen::IsolatedDeclgenChecker>(checker->DiagnosticEngine(), *program);
        this->declBuilder_ = std::make_unique<TSDeclGen>(checker, isolatedDeclgenChecker_.get(), program);
    }
}

checker::ETSChecker *TSDeclGenerator::GetChecker() const
{
    return reinterpret_cast<checker::ETSChecker *>(context_->GetChecker());
}

bool ValidateDeclgenOptions(const DeclgenOptions &options, checker::ETSChecker *checker)
{
    if ((options.outputDeclEts.empty() && !options.outputEts.empty()) ||
        (!options.outputDeclEts.empty() && options.outputEts.empty())) {
        checker->DiagnosticEngine().LogDiagnostic(diagnostic::GENERATE_DYNAMIC_DECLARATIONS,
                                                  util::DiagnosticMessageParams {});
        return false;
    }
    if (options.outputDeclEts.empty() && options.outputEts.empty()) {
        checker->DiagnosticEngine().LogDiagnostic(diagnostic::MISSING_OUTPUT_FILE, util::DiagnosticMessageParams {});
        return false;
    }
    return true;
}

bool TSDeclGenerator::SetDeclgenOptions(const DeclgenOptions &declgenOptions)
{
    auto checker = GetChecker();
    if (!ValidateDeclgenOptions(declgenOptions, checker)) {
        return false;
    }
    globalOptions_ = declgenOptions;
    if (isSimultaneousMode_) {
        return true;
    }
    declBuilder_->SetDeclgenOptions(declgenOptions);
    return true;
}

bool TSDeclGenerator::GenerateTsDeclarationsAfterParsedPhase()
{
    if (isSimultaneousMode_) {
        bool result = true;
        context_->parserProgram->GetExternalDecls()->Visit([&](parser::Program *prog) {
            if (!prog->IsBuiltSimultaneously()) {
                return;
            }
            if (!ProcessProgram(prog, true)) {
                result = false;
            }
        });
        return result;
    }
    return ProcessSingleFile(true);
}

bool TSDeclGenerator::GenerateTsDeclarationsAfterCheckPhase()
{
    if (isSimultaneousMode_) {
        bool result = true;
        context_->parserProgram->GetExternalDecls()->Visit([&](parser::Program *prog) {
            if (!prog->IsBuiltSimultaneously()) {
                return;
            }
            if (!ProcessProgram(prog, false)) {
                result = false;
            }
        });
        return result;
    }
    return ProcessSingleFile(false);
}
bool TSDeclGenerator::GenerateExportsAfterParsed(TSDeclGen *declBuilder, TSDeclgenContent *tsContent,
                                                 TSDeclgenContent *dtsContent)
{
    declBuilder->ResetDtsOutput();
    declBuilder->GenExportNamedDeclarations();
    tsContent->PushExports(declBuilder->GetTsOutput());
    dtsContent->PushExports(declBuilder->GetDtsOutput());
    return true;
}
bool TSDeclGenerator::GenerateDeclarationsAfterCheck(TSDeclGen *declBuilder, TSDeclgenContent *tsContent,
                                                     TSDeclgenContent *dtsContent, const DeclgenOptions &options)
{
    declBuilder->ResetTsOutput();
    declBuilder->ResetDtsOutput();
    if (!declBuilder->Generate()) {
        return false;
    }
    std::string checkTsOutput = declBuilder->GetTsOutput();
    std::string checkDtsOutput = declBuilder->GetDtsOutput();
    tsContent->RemoveDuplicateExports(checkTsOutput);
    dtsContent->RemoveDuplicateExports(checkDtsOutput);
    tsContent->PushStatements(checkTsOutput);
    dtsContent->PushStatements(checkDtsOutput);
    declBuilder->ResetTsOutput();
    declBuilder->ResetDtsOutput();
    declBuilder->GenInitModuleGlueCode();
    tsContent->PushInitModuleGlues(declBuilder->GetTsOutput());
    declBuilder->ResetTsOutput();
    declBuilder->GenImportDeclarations();
    tsContent->PushImports(declBuilder->GetTsOutput());
    dtsContent->PushImports(declBuilder->GetDtsOutput());
    if (!options.recordFile.empty()) {
        declBuilder->ResetDtsOutput();
        declBuilder->GenImportRecordDeclarations(options.recordFile);
        dtsContent->PushRecordImports(declBuilder->GetDtsOutput());
    }
    return true;
}
bool TSDeclGenerator::ProcessSingleFile(bool afterParsed)
{
    if (afterParsed) {
        return GenerateExportsAfterParsed(declBuilder_.get(), &tsContent_, &dtsContent_);
    }
    return GenerateDeclarationsAfterCheck(declBuilder_.get(), &tsContent_, &dtsContent_,
                                          declBuilder_->GetDeclgenOptions());
}
bool TSDeclGenerator::ProcessProgram(parser::Program *prog, bool afterParsed)
{
    auto checker = GetChecker();
    std::string_view sourcePath = prog->SourceFilePath().Utf8();
    auto it = inputFileIndexMap_.find(std::string(sourcePath));
    if (it == inputFileIndexMap_.end()) {
        return true;
    }

    size_t idx = it->second;
    if (afterParsed) {
        if (parsedIdx_.find(idx) != parsedIdx_.end()) {
            return true;
        }
        parsedIdx_.insert(idx);
    } else {
        if (checkedIdx_.find(idx) != checkedIdx_.end()) {
            return true;
        }
        checkedIdx_.insert(idx);
    }

    auto &fileContent = fileContents_[idx];
    if (fileContent.declBuilder == nullptr) {
        fileContent.isolatedDeclgenChecker =
            std::make_unique<declgen::IsolatedDeclgenChecker>(checker->DiagnosticEngine(), *prog);
        fileContent.declBuilder = std::make_unique<TSDeclGen>(checker, fileContent.isolatedDeclgenChecker.get(), prog);
        fileContent.options = globalOptions_;
        fileContent.options.outputDeclEts = outputDeclEts_[idx];
        fileContent.options.outputEts = outputEts_[idx];
        fileContent.declBuilder->SetDeclgenOptions(fileContent.options);
    }
    if (afterParsed) {
        return GenerateExportsAfterParsed(fileContent.declBuilder.get(), &fileContent.tsContent,
                                          &fileContent.dtsContent);
    }
    return GenerateDeclarationsAfterCheck(fileContent.declBuilder.get(), &fileContent.tsContent,
                                          &fileContent.dtsContent, fileContent.options);
}

bool WriteSingleFile(const std::string &path, TSDeclgenContent &content, checker::ETSChecker *checker)
{
    if (path.empty()) {
        return true;
    }
    if (!content.WriteToFile(path)) {
        checker->DiagnosticEngine().LogDiagnostic(diagnostic::OPEN_FAILED, util::DiagnosticMessageParams {path});
        return false;
    }
    return true;
}
bool TSDeclGenerator::Write()
{
    auto checker = GetChecker();
    if (isSimultaneousMode_) {
        for (auto &[idx, fileContent] : fileContents_) {
            if (!WriteSingleFile(fileContent.options.outputDeclEts, fileContent.dtsContent, checker) ||
                !WriteSingleFile(fileContent.options.outputEts, fileContent.tsContent, checker)) {
                return false;
            }
        }
        return true;
    }
    const auto &options = declBuilder_->GetDeclgenOptions();
    return WriteSingleFile(options.outputDeclEts, dtsContent_, checker) &&
           WriteSingleFile(options.outputEts, tsContent_, checker);
}

}  // namespace ark::es2panda::declgen_ets2ts
