/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <optional>
#include "refactors/convert_import.h"
#include "ir/astNode.h"
#include "refactor_provider.h"
#include "internal_api.h"
#include "services/text_change/change_tracker.h"
#include <string_view>
#include <set>
#include <utility>
#include "public/es2panda_lib.h"
#include <string>
#include <vector>
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/services/text_change/text_change_context.h"
namespace ark::es2panda::lsp {

ConvertImportRefactor::ConvertImportRefactor()
{
    AddKind(std::string(TO_NAMED_IMPORT_ACTION.kind));
}

static inline std::string GetModuleText(ir::ETSImportDeclaration *decl)
{
    return decl->Source()->Str().Mutf8();
}

static std::string JoinAsCommaSeparated(const std::vector<std::string> &names)
{
    std::string out;
    for (size_t i = 0; i < names.size(); ++i) {
        out += names[i];
        if (i + 1 < names.size()) {
            out += ", ";
        }
    }
    return out;
}

static std::string RemoveDefaultKeywordAfterExport(std::string src)
{
    const size_t p = src.find("export");
    if (p == std::string::npos) {
        return src;
    }

    size_t i = p + 6;
    while (i < src.size() && (std::isspace(static_cast<unsigned char>(src[i])) != 0)) {
        ++i;
    }
    static constexpr std::string_view K_DEFAULT = "default";
    const size_t kLen = K_DEFAULT.size();
    if (i + kLen <= src.size() && src.compare(i, kLen, K_DEFAULT) == 0) {
        size_t j = i + kLen;
        if (j < src.size() && (std::isspace(static_cast<unsigned char>(src[j])) != 0)) {
            ++j;
        }
        src.erase(i, j - i);
    }

    return src;
}

static ir::Identifier *GetImmediateIdentifierChild(ir::AstNode *node)
{
    ir::Identifier *out = nullptr;
    if (node == nullptr) {
        return nullptr;
    }

    node->FindChild([&](ir::AstNode *n) {
        if (n->IsIdentifier() && n->Parent() == node) {
            out = n->AsIdentifier();
            return true;
        }
        return false;
    });

    return out;
}

std::string GetNameFromIdent(ir::Identifier *id)
{
    if (id == nullptr) {
        return {};
    }
    return id->Name().Mutf8();
}

static std::optional<std::string> TryGetExportedNameFromDefaultExport(ir::AstNode *node)
{
    if (node == nullptr) {
        return std::nullopt;
    }

    if (node->IsClassDefinition()) {
        auto *cls = node->AsClassDefinition();
        if (cls->Ident() == nullptr || !cls->Ident()->IsIdentifier()) {
            return std::nullopt;
        }
        return GetNameFromIdent(cls->Ident()->AsIdentifier());
    }

    if (node->IsFunctionDeclaration()) {
        auto *fn = node->AsFunctionDeclaration();
        if (fn->Function() == nullptr || fn->Function()->Id() == nullptr || !fn->Function()->Id()->IsIdentifier()) {
            return std::nullopt;
        }
        return GetNameFromIdent(fn->Function()->Id()->AsIdentifier());
    }

    if (node->IsMethodDefinition()) {
        auto *fn = node->AsMethodDefinition();
        if (fn->Function() == nullptr || fn->Function()->Id() == nullptr || !fn->Function()->Id()->IsIdentifier()) {
            return std::nullopt;
        }
        return GetNameFromIdent(fn->Function()->Id()->AsIdentifier());
    }

    if (node->IsClassProperty()) {
        auto *fn = node->AsClassProperty();
        if (fn->Key() == nullptr || !fn->Key()->IsIdentifier()) {
            return std::nullopt;
        }
        return GetNameFromIdent(fn->Key()->AsIdentifier());
    }
    if (node->Type() == ir::AstNodeType::EXPORT_DEFAULT_DECLARATION) {
        auto *id = GetImmediateIdentifierChild(node);
        if (id == nullptr) {
            return std::nullopt;
        }
        return id->Name().Mutf8();
    }

    return std::nullopt;
}

template <class Func>
static void ForEachNsMember(ir::AstNode *root, const util::StringView &nsName, const Func &action)
{
    root->FindChild([&](ir::AstNode *n) {
        if (!n->IsMemberExpression()) {
            return false;
        }
        auto *me = n->AsMemberExpression();
        auto *obj = me->Object();
        if (obj == nullptr || !obj->IsIdentifier()) {
            return false;
        }
        if (obj->AsIdentifier()->Name() != nsName) {
            return false;
        }
        auto *prop = me->Property();
        if (prop == nullptr || !prop->IsIdentifier()) {
            return false;
        }

        action(me, prop->AsIdentifier()->Name().Mutf8());
        return false;  // continue traversal
    });
}

static void CollectNsMembers(ir::AstNode *root, const util::StringView &nsName, std::set<std::string> &out)
{
    ForEachNsMember(root, nsName, [&](ir::MemberExpression *, const std::string &propName) { out.insert(propName); });
}

static void CollectNsMemberReplaceTargets(ir::AstNode *root, const util::StringView &nsName,
                                          std::vector<std::pair<ir::MemberExpression *, std::string>> &out)
{
    ForEachNsMember(root, nsName,
                    [&](ir::MemberExpression *me, const std::string &propName) { out.emplace_back(me, propName); });
}

std::vector<ApplicableRefactorInfo> ConvertImportRefactor::GetAvailableActions(const RefactorContext &refContext) const
{
    ApplicableRefactorInfo applicableRef;
    std::vector<ApplicableRefactorInfo> res;
    if (!IsKind(refContext.kind)) {
        return res;
    }

    auto *node = GetTouchingToken(refContext.context, refContext.span.pos, false);
    if (node == nullptr) {
        return res;
    }

    auto onlyImportDecl = [](ir::AstNode *n) { return n->IsETSImportDeclaration(); };
    auto *importNode = FindAncestor(node, onlyImportDecl);
    if (importNode == nullptr) {
        return res;
    }

    auto *decl = importNode->AsETSImportDeclaration();
    if (decl == nullptr || decl->Specifiers().empty()) {
        return res;
    }

    int named = 0;
    int ns = 0;
    int deflt = 0;
    for (auto *s : decl->Specifiers()) {
        if (s == nullptr) {
            continue;
        }
        if (s->IsImportSpecifier()) {
            ++named;
        } else if (s->IsImportNamespaceSpecifier()) {
            ++ns;
        } else if (s->IsImportDefaultSpecifier()) {
            ++deflt;
        }
    }

    const bool isPureNamespace = (ns > 0) && (named == 0) && (deflt == 0);
    const bool isPureDefault = (deflt > 0) && (named == 0) && (ns == 0);
    if (!(isPureNamespace || isPureDefault)) {
        return res;
    }
    applicableRef.name = refactor_name::CONVERT_IMPORT_REFACTOR_NAME;
    applicableRef.description = refactor_description::CONVERT_IMPORT_REFACTOR_DESC;
    applicableRef.action.kind = std::string(TO_NAMED_IMPORT_ACTION.kind);
    applicableRef.action.name = std::string(TO_NAMED_IMPORT_ACTION.name);
    applicableRef.action.description = std::string(TO_NAMED_IMPORT_ACTION.description);
    res.push_back(applicableRef);
    return res;
}

struct ImportSpecInfo {
    bool hasNamespace {false}, hasNamed {false}, hasDefault {false};
    ir::Identifier *nsIdent {nullptr};
    ir::ImportDefaultSpecifier *defaultSpec {nullptr};
};

static std::optional<ImportSpecInfo> AnalyzeImport(ir::ETSImportDeclaration *decl)
{
    if (decl == nullptr) {
        return std::nullopt;
    }
    ImportSpecInfo info {};
    for (auto *s : decl->Specifiers()) {
        if (s == nullptr) {
            continue;
        }
        if (s->IsImportNamespaceSpecifier()) {
            info.hasNamespace = true;
            if (info.nsIdent == nullptr && s->AsImportNamespaceSpecifier()->Local() != nullptr &&
                s->AsImportNamespaceSpecifier()->Local()->IsIdentifier()) {
                info.nsIdent = s->AsImportNamespaceSpecifier()->Local()->AsIdentifier();
            }
        } else if (s->IsImportSpecifier()) {
            info.hasNamed = true;
        } else if (s->IsImportDefaultSpecifier()) {
            info.hasDefault = true;
            if (info.defaultSpec == nullptr) {
                info.defaultSpec = s->AsImportDefaultSpecifier();
            }
        }
    }
    return info;
}

static std::unique_ptr<RefactorEditInfo> HandleNamespaceCase(const RefactorContext &ctx, ir::ETSImportDeclaration *decl,
                                                             ir::Identifier *nsIdent)
{
    auto *pub = reinterpret_cast<public_lib::Context *>(ctx.context);
    if (pub == nullptr || pub->parserProgram == nullptr || pub->parserProgram->Ast() == nullptr || nsIdent == nullptr) {
        return nullptr;
    }
    auto *program = pub->parserProgram->Ast();

    std::set<std::string> members;
    CollectNsMembers(reinterpret_cast<ir::AstNode *>(program), nsIdent->Name(), members);
    if (members.empty()) {
        return nullptr;
    }

    std::vector<std::pair<ir::MemberExpression *, std::string>> targets;
    CollectNsMemberReplaceTargets(reinterpret_cast<ir::AstNode *>(program), nsIdent->Name(), targets);

    TextChangesContext tcc {ctx.textChangesContext->host, ctx.textChangesContext->formatContext,
                            ctx.textChangesContext->preferences};
    const std::string importTxt = "import { " + JoinAsCommaSeparated({members.begin(), members.end()}) + " } from \"" +
                                  GetModuleText(decl) + "\";\n";

    auto fileChanges = ChangeTracker::With(tcc, [&](ChangeTracker &tr) {
        tr.ReplaceNodeWithText(ctx.context, decl, importTxt);
        for (auto &t : targets) {
            tr.ReplaceNodeWithText(ctx.context, t.first, t.second);
        }
    });
    if (fileChanges.empty()) {
        return nullptr;
    }
    return std::make_unique<RefactorEditInfo>(RefactorEditInfo {fileChanges});
}

static ir::AstNode *FindExportNodeInTarget(es2panda_Context *targetCtx, ir::AstNode *origExportNode,
                                           const std::string &exportedName)
{
    if (targetCtx == nullptr || origExportNode == nullptr) {
        return nullptr;
    }

    auto *tpub = reinterpret_cast<public_lib::Context *>(targetCtx);
    if (tpub == nullptr) {
        return nullptr;
    }

    const std::string &full = tpub->input;
    const size_t nameLen = exportedName.size();

    for (size_t pos = full.find(exportedName, 0); pos != std::string::npos;
         pos = full.find(exportedName, pos + nameLen)) {
        ir::AstNode *id = GetTouchingToken(targetCtx, pos, false);
        if (id == nullptr) {
            continue;
        }

        ir::AstNode *parent = id->Parent();
        if (parent == nullptr) {
            continue;
        }

        if (parent->DumpEtsSrc() == origExportNode->DumpEtsSrc()) {
            return parent;
        }
    }

    return nullptr;
}

static std::string MakeExporterReplacement(ir::AstNode *exportNode, const std::string &name)
{
    using T = ir::AstNodeType;
    if (exportNode == nullptr) {
        return {};
    }
    if (exportNode->Type() == T::EXPORT_DEFAULT_DECLARATION) {
        return "export { " + name + " };";
    }
    if (exportNode->Type() == T::CLASS_PROPERTY) {
        return "export " + exportNode->DumpEtsSrc();
    }
    return RemoveDefaultKeywordAfterExport(exportNode->DumpEtsSrc());
}

static std::unique_ptr<RefactorEditInfo> HandleDefaultCase(const RefactorContext &ctx, ir::ETSImportDeclaration *decl,
                                                           ir::ImportDefaultSpecifier *defSpec)
{
    if (defSpec == nullptr || defSpec->Local() == nullptr || !defSpec->Local()->IsIdentifier()) {
        return nullptr;
    }
    auto *localId = defSpec->Local()->AsIdentifier();
    auto *var = localId->Variable();
    if (var == nullptr || var->Declaration() == nullptr || var->Declaration()->Node() == nullptr) {
        return nullptr;
    }
    ir::AstNode *exportNode = var->Declaration()->Node();

    auto nameOpt = TryGetExportedNameFromDefaultExport(exportNode);
    if (!nameOpt.has_value()) {
        return nullptr;
    }
    const std::string exported = *nameOpt;
    const std::string local = localId->Name().Mutf8();
    const std::string importerTxt =
        (exported == local) ? "import { " + exported + " } from \"" + GetModuleText(decl) + "\";\n"
                            : "import { " + exported + " as " + local + " } from \"" + GetModuleText(decl) + "\";\n";

    Initializer init;
    es2panda_Context *tctx = init.CreateContext(std::string(decl->ResolvedSource()).c_str(), ES2PANDA_STATE_BOUND);
    if (tctx == nullptr) {
        return nullptr;
    }

    ir::AstNode *targetExport = FindExportNodeInTarget(tctx, exportNode, exported);
    if (targetExport == nullptr) {
        return nullptr;
    }

    const std::string exporterTxt = MakeExporterReplacement(targetExport, exported);
    TextChangesContext tcc {ctx.textChangesContext->host, ctx.textChangesContext->formatContext,
                            ctx.textChangesContext->preferences};

    auto fileChanges = ChangeTracker::With(tcc, [&](ChangeTracker &tr) {
        tr.ReplaceNodeWithText(ctx.context, decl, importerTxt);
        tr.ReplaceNodeWithText(tctx, targetExport, exporterTxt);
    });
    if (fileChanges.empty()) {
        return nullptr;
    }
    return std::make_unique<RefactorEditInfo>(RefactorEditInfo {fileChanges});
}

std::unique_ptr<RefactorEditInfo> ConvertImportRefactor::GetEditsForAction(const RefactorContext &context,
                                                                           const std::string &actionName) const
{
    if (std::string_view {actionName} != TO_NAMED_IMPORT_ACTION.name || context.context == nullptr ||
        context.textChangesContext == nullptr) {
        return nullptr;
    }

    auto *node = GetTouchingToken(context.context, context.span.pos, false);
    if (node == nullptr) {
        return nullptr;
    }

    auto *importNode = FindAncestor(node, [](ir::AstNode *n) { return n->IsETSImportDeclaration(); });
    if (importNode == nullptr) {
        return nullptr;
    }

    auto *decl = importNode->AsETSImportDeclaration();
    if (decl == nullptr) {
        return nullptr;
    }

    auto infoOpt = AnalyzeImport(decl);
    if (!infoOpt.has_value()) {
        return nullptr;
    }
    const auto &info = *infoOpt;

    if (info.hasNamespace && !info.hasNamed && !info.hasDefault) {
        return HandleNamespaceCase(context, decl, info.nsIdent);
    }

    if (!info.hasNamespace && !info.hasNamed && info.hasDefault) {
        return HandleDefaultCase(context, decl, info.defaultSpec);
    }

    return nullptr;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ConvertImportRefactor> g_convertImportRefactorRegister("ConvertImportRefactor");

}  // namespace ark::es2panda::lsp
