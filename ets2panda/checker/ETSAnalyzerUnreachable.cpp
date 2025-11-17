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

#include "ETSAnalyzer.h"

namespace ark::es2panda::checker {

// from as folder
checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::NamedType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::PrefixAssertionExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::Decorator *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::MetaProperty *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ScriptFunction *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSIndexSignature *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSMethodSignature *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSPropertySignature *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSSignatureDeclaration *node) const
{
    ES2PANDA_UNREACHABLE();
}
// from ets folder
checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSModule *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSImportDeclaration *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSTuple *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSUnionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSWildcardType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ChainExpression *expr) const
{
    ES2PANDA_UNREACHABLE();  // eliminated in OptionalLowering
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ClassExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSReExportDeclaration *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::DirectEvalExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::FunctionExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ImportExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::NewExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::OmittedExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TaggedTemplateExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::YieldExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::RegExpLiteral *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::UndefinedLiteral *expr) const
{
    ES2PANDA_UNREACHABLE();
}

// compile methods for MODULE-related nodes in alphabetical order
checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ExportAllDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ExportDefaultDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ExportNamedDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ExportSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ImportDefaultSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ImportSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::DebuggerStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check(ir::ForInStatement *st) const
{
    ES2PANDA_ASSERT(GetETSChecker()->IsAnyError());
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::FunctionDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::SwitchCaseStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

// from ts folder
checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSAnyKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSBigintKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSBooleanKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSClassImplements *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSConditionalType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSConstructorType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSEnumMember *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSExternalModuleReference *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSFunctionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSImportEqualsDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSImportType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSIndexedAccessType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSInferType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSInterfaceBody *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSInterfaceHeritage *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSIntersectionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSLiteralType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSMappedType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSModuleBlock *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSModuleDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSNamedTupleMember *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSNeverKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSNullKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSNumberKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSObjectKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSParameterProperty *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSParenthesizedType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSStringKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSTupleType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSTypeAssertion *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSTypeLiteral *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSTypeOperator *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSTypeParameter *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSTypeParameterDeclaration *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSTypeParameterInstantiation *expr) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSTypePredicate *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSTypeQuery *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSTypeReference *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSUndefinedKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSUnionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSUnknownKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TSVoidKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::DummyNode *expr) const
{
    ES2PANDA_UNREACHABLE();
}
}  // namespace ark::es2panda::checker
