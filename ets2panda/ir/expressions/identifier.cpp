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

#include "identifier.h"

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"

namespace ark::es2panda::ir {
Identifier::Identifier([[maybe_unused]] Tag const tag, Identifier const &other, ArenaAllocator *const allocator)
    : AnnotatedExpression(static_cast<AnnotatedExpression const &>(other), allocator)
{
    name_ = other.name_;
    flags_ = other.flags_;
    InitHistory();
}

Identifier::Identifier(ArenaAllocator *const allocator) : Identifier(ERROR_LITERAL, allocator)
{
    flags_ |= IdentifierFlags::ERROR_PLACEHOLDER;
    InitHistory();
}

Identifier::Identifier(util::StringView const name, [[maybe_unused]] ArenaAllocator *const allocator)
    : AnnotatedExpression(AstNodeType::IDENTIFIER), name_(name)
{
    if (name == ERROR_LITERAL) {
        flags_ |= IdentifierFlags::ERROR_PLACEHOLDER;
    }
    InitHistory();
}

Identifier::Identifier(util::StringView const name, TypeNode *const typeAnnotation,
                       [[maybe_unused]] ArenaAllocator *const allocator)
    : AnnotatedExpression(AstNodeType::IDENTIFIER, typeAnnotation), name_(name)
{
    if (name == ERROR_LITERAL) {
        flags_ |= IdentifierFlags::ERROR_PLACEHOLDER;
    }
    InitHistory();
}

void Identifier::SetName(const util::StringView &newName) noexcept
{
    ES2PANDA_ASSERT(newName != ERROR_LITERAL);
    GetOrCreateHistoryNodeAs<Identifier>()->name_ = newName;
}

Identifier *Identifier::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<Identifier>(Tag {}, *this, allocator);
    ES2PANDA_ASSERT(clone != nullptr);

    clone->SetTsType(TsType());
    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());
    return clone;
}

Identifier *Identifier::CloneReference(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = Clone(allocator, parent);
    ES2PANDA_ASSERT(clone != nullptr);
    if (clone->IsReference(ScriptExtension::ETS)) {
        clone->SetTsTypeAnnotation(nullptr);
    }
    return clone;
}

void Identifier::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    if (auto *typeAnnotation = TypeAnnotation(); typeAnnotation != nullptr) {
        if (auto *transformedNode = cb(typeAnnotation); typeAnnotation != transformedNode) {
            typeAnnotation->SetTransformedNode(transformationName, transformedNode);
            SetTsTypeAnnotation(static_cast<TypeNode *>(transformedNode));
        }
    }
}

void Identifier::Iterate(const NodeTraverser &cb) const
{
    if (TypeAnnotation() != nullptr) {
        cb(TypeAnnotation());
    }
}

void Identifier::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", IsPrivateIdent() ? "PrivateIdentifier" : "Identifier"},
                 {"name", Name()},
                 {"typeAnnotation", AstDumper::Optional(TypeAnnotation())},
                 {"optional", AstDumper::Optional(IsOptional())}});
}

void Identifier::Dump(ir::SrcDumper *dumper) const
{
    if (IsReceiver()) {
        dumper->Add("this");
        return;
    }

    if (IsPrivateIdent()) {
        dumper->Add("private ");
    }

    auto name = std::string(Name());
    std::string propertyStr = compiler::Signatures::PROPERTY.data();
    if (UNLIKELY(name.find(propertyStr) != std::string::npos)) {
        name.replace(name.find(propertyStr), propertyStr.length(), "_$property$_");
    }
    dumper->Add(name);
    if (IsOptional()) {
        dumper->Add("?");
    }

    dumper->PushTask([dumper, name = std::string(name_)] { dumper->DumpNode(name); });
}

void Identifier::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void Identifier::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *Identifier::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType Identifier::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

bool Identifier::IsDeclaration(ScriptExtension ext) const
{
    // We can determine reference status from parent node
    if (Parent() == nullptr) {
        return false;
    }

    auto const *const parent = Parent();

    // We need two Parts because check is too long to fit in one function
    if (CheckDeclarationsPart1(parent, ext) || CheckDeclarationsPart2(parent, ext)) {
        return true;
    }

    // This will check other cases that are not related to declarations
    return CheckNotDeclarations(parent, ext);
}

bool Identifier::CheckNotDeclarations(const ir::AstNode *parent, [[maybe_unused]] ScriptExtension ext) const
{
    if (parent->IsTSMethodSignature() && ext == ScriptExtension::TS) {
        // NOTE(kkonkuznetsov): fix in TS
        // Example: c(a: (a: number, b: void) => string, b?: number[]): undefined;
        // a, b should not be references
        return false;
    }

    // Parameters in methods are not references
    // Example:
    // function foo(a: int)
    if (parent->IsETSParameterExpression()) {
        return true;
    }

    if (parent->IsTSPropertySignature() && ext == ScriptExtension::TS) {
        // Example:
        //
        // interface foo {
        //     a: number,
        // }
        auto *prop = parent->AsTSPropertySignature();
        if (prop->Key() == this) {
            return true;
        }
    }

    if (parent->IsClassProperty()) {
        // RHS is a reference
        // Example: const foo1: (self: Object, x: int) => int = foo;
        // foo here is a reference
        auto *prop = parent->AsClassProperty();
        return !(prop->Value() == this);
    }

    // Identifier in catch is not a reference
    // Example:
    //
    // _try_{
    //   _throw_new_Exception();}_catch_(e)_{}
    if (parent->IsCatchClause()) {
        return true;
    }

    // Type Parameter is not a reference
    // Example:
    // interface X <K> {}
    if (parent->IsTSTypeParameter()) {
        return true;
    }

    // Rest Parameter is not a reference
    // Example:
    // class A {
    //    constructor(... items :Object[]){}
    // }
    if (parent->IsRestElement()) {
        if (ext == ScriptExtension::TS) {
            // NOTE(kkonkuznetsov): fix in TS
            // Example: var func8: { (...c): boolean, (...c: number[]): string };
            // c is expected to be a reference
            return false;
        }

        if (ext == ScriptExtension::JS) {
            // NOTE(kkonkuznetsov): some JS tests have the following code:
            //
            // let x;
            // async function* fn() {
            //   for await ([...x] of [g()]) {
            //   }
            // }
            // Here in `[...x]` x is parsed as Rest Element,
            // however x is expected to be a reference.
            // Otherwise the tests fail.
            return !(parent->Parent() != nullptr && parent->Parent()->IsArrayPattern());
        }

        return true;
    }

    // Script function identifiers are not references
    // Example:
    // public static foo()
    if (parent->IsScriptFunction()) {
        if (ext == ScriptExtension::TS) {
            // NOTE(kkonkuznetsov): fix in TS
            // Example: let c = (a?: number) => { }
            return false;
        }

        if (ext == ScriptExtension::JS) {
            // In some JS tests the following code:
            //
            // let_f_=_()_=>_o;
            //
            // creates AST that has Identifier as Body
            return !(parent->AsScriptFunction()->Body() == this);
        }

        return true;
    }

    // Helper function to reduce function size
    return CheckDefinitions(parent, ext);
}

bool Identifier::CheckDefinitions(const ir::AstNode *parent, [[maybe_unused]] ScriptExtension ext) const
{
    // New methods are not references
    if (parent->IsMethodDefinition()) {
        return true;
    }

    // New classes are not references
    if (parent->IsClassDefinition()) {
        if (ext == ScriptExtension::JS) {
            // Example from JS tests:
            // inner Outer is a reference
            //
            // class Outer {
            //   innerclass() {
            //     return class extends Outer {};
            //   }
            // }
            auto *def = parent->AsClassDefinition();
            return !(def->Super() == this);
        }

        return true;
    }

    return false;
}

bool Identifier::CheckDeclarationsPart1(const ir::AstNode *parent, [[maybe_unused]] ScriptExtension ext) const
{
    // All declarations are not references
    if (parent->IsVariableDeclarator()) {
        if (ext == ScriptExtension::TS) {
            // NOTE(kkonkuznetsov): fix in TS
            // All variable declarations in TS are expected to be references for some reason
            return false;
        }

        auto *decl = parent->AsVariableDeclarator();

        // Example: let goo = foo;
        // RHS is a reference
        return !(decl->Init() == this);
    }

    // All declarations are not references
    if (parent->IsVariableDeclaration()) {
        return true;
    }

    if (parent->IsClassDeclaration()) {
        return true;
    }

    if (parent->IsETSPackageDeclaration()) {
        return true;
    }

    if (parent->IsFunctionDeclaration()) {
        return true;
    }

    if (parent->IsImportDeclaration()) {
        return true;
    }

    if (parent->IsETSImportDeclaration()) {
        return true;
    }

    if (parent->IsETSStructDeclaration()) {
        return true;
    }

    if (parent->IsExportAllDeclaration()) {
        return true;
    }

    if (parent->IsExportDefaultDeclaration()) {
        return true;
    }

    if (Parent()->IsClassDefinition()) {
        return Parent()->AsClassDefinition()->IsGlobal();
    }

    return false;
}

bool Identifier::CheckDeclarationsPart2(const ir::AstNode *parent, ScriptExtension ext) const
{
    // All declarations are not references
    if (parent->IsExportNamedDeclaration()) {
        return true;
    }

    if (parent->IsTSEnumDeclaration()) {
        // NOTE(kkonkuznetsov): fix in TS
        // Should not be a reference
        // However currently some TS tests depend on declaration ident being a reference
        return !(ext == ScriptExtension::TS);
    }

    if (parent->IsTSInterfaceDeclaration()) {
        // NOTE(kkonkuznetsov): This should not be a reference
        // Example:
        //
        // interface foo {
        //     a: number,
        // }
        //
        // However currently some TS tests depend on interface ident being a reference
        return !(ext == ScriptExtension::TS);
    }

    if (parent->IsTSModuleDeclaration()) {
        return true;
    }

    if (parent->IsTSSignatureDeclaration()) {
        // NOTE(kkonkuznetsov): fix in TS
        // Example: new(a: null, b?: string): { a: number, b: string, c?([a, b]): string }
        // a, b should not be references
        return !(ext == ScriptExtension::TS);
    }

    if (parent->IsETSReExportDeclaration()) {
        return true;
    }

    if (parent->IsTSImportEqualsDeclaration()) {
        return true;
    }

    if (parent->IsTSTypeAliasDeclaration()) {
        return true;
    }

    if (parent->IsTSTypeParameterDeclaration()) {
        return true;
    }

    if (parent->IsDeclare()) {
        return true;
    }

    if (parent->Parent() != nullptr) {
        if (parent->Parent()->IsTSEnumDeclaration() &&
            !(parent->IsTSEnumMember() && parent->AsTSEnumMember()->Init() == this)) {
            return true;
        }
    }

    return false;
}

std::string Identifier::ToString() const
{
    if (!Name().Empty() && !Name().Is(ERROR_LITERAL)) {
        return std::string {Name().Utf8()};
    }

    return AnnotatedExpression::ToString();
}
}  // namespace ark::es2panda::ir
