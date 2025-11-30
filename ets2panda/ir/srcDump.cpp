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

#include "srcDump.h"
#include "public/public.h"

#include "util/helpers.h"
#include "public/public.h"
#include "varbinder/ETSBinder.h"

namespace ark::es2panda::ir {

SrcDumper::SrcDumper(Declgen *dg) : dg_(dg) {}
SrcDumper::SrcDumper(const ir::AstNode *node, bool enableJsdocDump, Declgen *dg) : dg_(dg)
{
    if (enableJsdocDump) {
        jsdocGetter_ = std::make_unique<parser::JsdocHelper>(node);
    }
}

void SrcDumper::IncrIndent()
{
    indent_ += "  ";
}

void SrcDumper::DecrIndent()
{
    ES2PANDA_ASSERT(indent_.size() >= 2U);
    indent_.resize(indent_.size() - 2U);
}

// NOTE: `num` argument is unsed, should be deleted once bindings are no longer hardcoded (never?)
void SrcDumper::Endl([[maybe_unused]] size_t num)
{
    ss_ << '\n';
    ss_ << indent_;
}

static bool OnlySpaces(const std::string &s)
{
    for (char c : s) {
        if (std::isspace(c) == 0) {
            return false;
        }
    }
    return true;
}
static std::stringstream NormalizeStream(std::stringstream &iss)
{
    std::stringstream oss;
    std::string line;
    bool lastWasEmpty = false;
    while (std::getline(iss, line)) {
        if (OnlySpaces(line)) {
            if (!lastWasEmpty) {
                oss << std::endl;
                lastWasEmpty = true;
            }
        } else {
            oss << line << std::endl;
            lastWasEmpty = false;
        }
    }
    return oss;
}

std::string SrcDumper::Str() const
{
    // NOTE: at first place we should not print extra indentations and newlines
    // NOTE: should we normalize for all calls?
    if (IsDeclgen()) {
        std::stringstream ss;
        // NOTE: copy stream to save constness of Str() method (used in plugins)
        ss << ss_.rdbuf();
        ss = NormalizeStream(ss);
        return ss.str();
    }
    return ss_.str();
}

void SrcDumper::Add(std::string_view const str)
{
    ss_ << str;
}

void SrcDumper::Add(char const ch)
{
    ss_ << ch;
}

void SrcDumper::Add(int8_t i)
{
    ss_ << static_cast<int32_t>(i);
}

void SrcDumper::Add(int16_t i)
{
    ss_ << i;
}

void SrcDumper::Add(int32_t i)
{
    ss_ << i;
}

void SrcDumper::Add(int64_t l)
{
    ss_ << l;
}

void SrcDumper::Add(float f)
{
    ss_ << f;
}

void SrcDumper::Add(double d)
{
    ss_ << d;
}

bool SrcDumper::IsDeclgen() const
{
    return GetDeclgen() != nullptr;
}

Declgen *SrcDumper::GetDeclgen() const
{
    return dg_;
}

void SrcDumper::DumpJsdocBeforeTargetNode(const ir::AstNode *inputNode)
{
    if (!jsdocGetter_) {
        return;
    }
    jsdocGetter_->InitNode(inputNode);
    auto resJsdoc = jsdocGetter_->GetJsdocBackward();
    if (!resJsdoc.Empty()) {
        ss_ << resJsdoc.Mutf8();
        ss_ << std::endl;
        auto parent = inputNode->Parent();
        if (inputNode->IsClassDefinition() || parent->IsClassDefinition() || parent->IsTSInterfaceBody() ||
            parent->IsScriptFunction()) {
            parent = parent->Parent();
        }
        while (!parent->IsETSModule() || (parent->IsETSModule() && parent->AsETSModule()->IsNamespace())) {
            ss_ << "  ";
            parent = parent->Parent();
            if (parent == nullptr) {
                break;
            }
        }
    }
}

void SrcDumper::SetDefaultExport() noexcept
{
    hasDefaultExport_ = true;
}

bool SrcDumper::HasDefaultExport() const noexcept
{
    return IsDeclgen() && hasDefaultExport_;
}

void SrcDumper::DumpExports()
{
    if (dg_ == nullptr) {
        return;
    }
    auto *varbinder = dg_->GetCtx()->GetChecker()->VarBinder();
    if (!varbinder->IsETSBinder()) {
        return;
    }
    auto const &exportMap = varbinder->AsETSBinder()->GetSelectiveExportAliasMultimap();
    if (auto const it = exportMap.find(dg_->GetCtx()->sourceFile->filePath);
        it != exportMap.cend() && !it->second.empty()) {
        for (auto const &[_, data] : it->second) {
            if (data.second->IsExportNamedDeclaration() &&
                data.second->AsExportNamedDeclaration()->HasDumpData(HasDefaultExport())) {
                ss_ << '\n';
                data.second->Dump(this);
                ss_ << ';';
            }
        }
    }
}

struct PostDumper {
    explicit PostDumper(SrcDumper *dumper) : dumper_ {dumper} {}

    void operator()(const ir::AstNode *node)
    {
        auto g1 = dumper_->GetDeclgen()->BuildAmbientContextGuard();
        auto g2 = dumper_->GetDeclgen()->BuildPostDumpIndirectDepsPhaseLockGuard();
        dumper_->GetDeclgen()->SetPostDumpIndirectDepsPhase();
        auto namespacesCount = ReconstructNamespaces(node);
        node->Dump(dumper_);
        DestroyNamespaces(namespacesCount);
    }

    size_t ReconstructNamespaces(const ir::AstNode *node)
    {
        std::vector<const ir::ClassDefinition *> nsChain;

        auto getImmediateNamespace = [](const ir::AstNode *n) {
            auto classDef = ark::es2panda::util::Helpers::FindAncestorGivenByType(n, ir::AstNodeType::CLASS_DEFINITION);
            if ((classDef != nullptr) && classDef->AsClassDefinition()->IsNamespaceTransformed()) {
                return classDef;
            }
            return static_cast<const ir::AstNode *>(nullptr);
        };
        for (auto classDef = getImmediateNamespace(node); classDef != nullptr;
             classDef = getImmediateNamespace(classDef)) {
            ES2PANDA_ASSERT(classDef->AsClassDefinition()->IsNamespaceTransformed());
            nsChain.push_back(classDef->AsClassDefinition());
        }

        for (auto it = nsChain.rbegin(); it != nsChain.rend(); ++it) {
            auto ns = *it;
            if (ns->IsExported()) {
                dumper_->Add("export ");
            }
            dumper_->GetDeclgen()->TryDeclareAmbientContext(dumper_);
            dumper_->Add("namespace " + ns->Ident()->Name().Mutf8() + " {");
        }
        dumper_->Add('\n');
        return nsChain.size();
    }

    void DestroyNamespaces(size_t namespacesCount)
    {
        std::string nsClose(namespacesCount, '}');
        nsClose += '\n';
        dumper_->Add(nsClose);
    }

    void operator()([[maybe_unused]] std::monostate /*unused*/)
    {
        ES2PANDA_UNREACHABLE();
    }

private:
    // CC-OFFNXT(G.NAM.03-CPP) project codestyle
    SrcDumper *dumper_ {};
};

void Declgen::CollectImport(const ir::ImportDeclaration *import)
{
    imports_.push_back(import);
}

static auto AstFromType(Declgen *dg, const checker::Type *type)
{
    ES2PANDA_ASSERT(type != nullptr);
    auto typeStr = type->ToString();
    auto *parser = dg->GetCtx()->parser->AsETSParser();
    return parser->CreateFormattedTypeAnnotation(typeStr);
}

static std::string DumpImplicitImportsOfSpecifier(Declgen *dg, ImportSpecifier *specifier)
{
    auto type = specifier->Imported()->Variable()->TsType();
    // NOTE: should be generalized, now only return type of a function is introduced.
    // Also a check for predefined types is needed.
    if (!type->IsETSFunctionType()) {
        return "";
    }

    auto rettype = type->AsETSFunctionType()->CallSignatures()[0]->ReturnType();
    if (!rettype->IsETSObjectType()) {
        return "";
    }
    auto ast = AstFromType(dg, type->AsETSObjectType());
    std::string str;
    ast->IterateRecursively([&str](ir::AstNode *node) {
        if (node->IsIdentifier()) {
            str += ", ";
            str += node->AsIdentifier()->Name().Utf8();
        }
    });
    return str;
}

[[maybe_unused]] static std::string DumpImport(Declgen *dg, const ImportDeclaration *import)
{
    std::string res = "import {";
    bool needComma = false;
    for (auto node : import->Specifiers()) {
        if (needComma) {
            res += ", ";
        }
        needComma = true;
        res += node->DumpEtsSrc();
        if (node->IsImportSpecifier()) {
            res += DumpImplicitImportsOfSpecifier(dg, node->AsImportSpecifier());
        }
    }
    res += "} from \"";
    res += import->Source()->Str().Utf8();
    res += "\";\n";

    return res;
}

void Declgen::DumpImports(std::string &res)
{
    if (!imports_.empty()) {
        res += '\n';
        for (auto const *import : imports_) {
            res += import->DumpEtsSrc();  // Instead, 'DumpImport' should be called when it will be fixed.
        }
    }
}

void Declgen::DumpNode(SrcDumper *dumper, const std::string &key)
{
    auto it = unExportNode_.find(key);
    if (it == unExportNode_.end()) {
        return;
    }

    NodeVariant node = it->second;
    unExportNode_.erase(it);

    std::visit(PostDumper {dumper}, node);
}

void Declgen::Dump(ir::SrcDumper *dumper, const checker::Type *type)
{
    AstFromType(this, type)->Dump(dumper);
}

void Declgen::TryDeclareAmbientContext(SrcDumper *srcDumper)
{
    if (!ambientDeclarationLock_.IsAcquired()) {
        ambientDeclarationLock_.Acquire();
        srcDumper->Add("declare ");
    }
}

void Declgen::Run()
{
    while (!taskQueue_.empty()) {
        auto task = std::move(taskQueue_.front());
        taskQueue_.pop();
        task();
    }
}

}  // namespace ark::es2panda::ir
