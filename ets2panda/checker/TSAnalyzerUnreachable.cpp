/*
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

#include "TSAnalyzer.h"

namespace ark::es2panda::checker {

// from as folder
checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::NamedType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::PrefixAssertionExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ClassProperty *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ClassStaticBlock *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::Decorator *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::MethodDefinition *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::OverloadDeclaration *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::Property *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ScriptFunction *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::SpreadElement *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TemplateElement *expr) const
{
    ES2PANDA_UNREACHABLE();
}

// from ets folder
checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSModule *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSClassLiteral *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSFunctionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSIntrinsicNode *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSImportDeclaration *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSNewArrayInstanceExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSNewClassInstanceExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSNewMultiDimArrayInstanceExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSPackageDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSGenericInstantiatedNode *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSParameterExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSDestructuring *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSPrimitiveType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSStructDeclaration *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSTuple *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSTypeReference *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSTypeReferencePart *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSNonNullishTypeNode *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSNullType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSUndefinedType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSNeverType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSStringLiteralType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSUnionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSKeyofType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSWildcardType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::BlockExpression *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ClassExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::DirectEvalExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ImportExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::CharLiteral *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::UndefinedLiteral *expr) const
{
    ES2PANDA_UNREACHABLE();
}

// compile methods for MODULE-related nodes in alphabetical order
checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ExportAllDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ExportDefaultDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ExportNamedDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ExportSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ImportDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ImportDefaultSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ImportNamespaceSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ImportSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}
// compile methods for STATEMENTS in alphabetical order
checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::AssertStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ClassDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ContinueStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::DebuggerStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ForInStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ForOfStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::LabelledStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::SwitchCaseStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ThrowStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSClassImplements *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSConditionalType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ETSReExportDeclaration *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSEnumMember *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSExternalModuleReference *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSImportEqualsDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSImportType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSInferType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSInterfaceHeritage *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSIntersectionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSMappedType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSModuleBlock *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSModuleDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSNonNullExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSObjectKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSParameterProperty *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSThisType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSTypeAssertion *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSTypeOperator *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSTypeParameter *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSTypeParameterDeclaration *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSTypeParameterInstantiation *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSTypePredicate *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::DummyNode *node) const
{
    ES2PANDA_UNREACHABLE();
}
}  // namespace ark::es2panda::checker
