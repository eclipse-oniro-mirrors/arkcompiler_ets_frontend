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

#include "ETSCompiler.h"

namespace ark::es2panda::compiler {

void ETSCompiler::Compile([[maybe_unused]] const ir::NamedType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::PrefixAssertionExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ClassDefinition *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ClassStaticBlock *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::Decorator *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::MetaProperty *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::MethodDefinition *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::OverloadDeclaration *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::Property *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ScriptFunction *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::SpreadElement *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSIndexSignature *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSMethodSignature *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSPropertySignature *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSSignatureDeclaration *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSModule *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSTuple *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSImportDeclaration *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSPackageDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSGenericInstantiatedNode *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSPrimitiveType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSStructDeclaration *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSNonNullishTypeNode *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSNullType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSUndefinedType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSNeverType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSStringLiteralType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSUnionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSKeyofType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ArrowFunctionExpression *expr) const
{
    // Should have been handled by LambdaLowering
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ChainExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ClassExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSReExportDeclaration *stmt) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::DirectEvalExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSDestructuring *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::FunctionExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::NewExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::OmittedExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::BrokenTypeNode *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TaggedTemplateExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::UpdateExpression *expr) const
{
    // Should have been handled by OpAssignment lowering.
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::YieldExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::RegExpLiteral *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::UndefinedLiteral *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ExportAllDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ExportDefaultDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ExportNamedDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ExportSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ImportDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ImportDefaultSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ImportNamespaceSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ImportSpecifier *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::DebuggerStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ForInStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::FunctionDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::SwitchCaseStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSAnyKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSBigintKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSBooleanKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSClassImplements *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSConditionalType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSConstructorType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSEnumMember *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSExternalModuleReference *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSFunctionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSImportEqualsDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSImportType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSIndexedAccessType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSInferType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSInterfaceBody *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSInterfaceHeritage *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSIntersectionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSLiteralType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSMappedType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSModuleBlock *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSModuleDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSNamedTupleMember *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSNeverKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSNullKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSNumberKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSObjectKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSParameterProperty *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSParenthesizedType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSStringKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSThisType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTupleType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTypeAssertion *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTypeLiteral *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTypeOperator *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTypeParameter *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTypeParameterDeclaration *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTypeParameterInstantiation *expr) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTypePredicate *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTypeQuery *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTypeReference *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSUndefinedKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSUnionType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSUnknownKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSVoidKeyword *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::DummyNode *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSEnumDeclaration *st) const
{
    ES2PANDA_UNREACHABLE();
}
}  // namespace ark::es2panda::compiler
