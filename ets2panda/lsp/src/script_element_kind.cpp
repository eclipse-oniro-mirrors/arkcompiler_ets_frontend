/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "internal_api.h"
#include "rename.h"
#include "public/public.h"
#include "script_element_kind.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::lsp {
std::tuple<bool, CompletionEntryKind> GetTargetTokenKindIfETSType(ir::AstNodeType type)
{
    switch (type) {
        case ir::AstNodeType::ETS_NULL_TYPE:
        case ir::AstNodeType::ETS_UNDEFINED_TYPE:
        case ir::AstNodeType::ETS_NEVER_TYPE:
        case ir::AstNodeType::ETS_STRING_LITERAL_TYPE:
            return std::make_tuple(true, CompletionEntryKind::VALUE);
        case ir::AstNodeType::ETS_PRIMITIVE_TYPE:
        case ir::AstNodeType::ETS_CLASS_LITERAL:
        case ir::AstNodeType::ETS_KEYOF_TYPE:
            return std::make_tuple(true, CompletionEntryKind::KEYWORD);
        case ir::AstNodeType::ETS_NEW_ARRAY_INSTANCE_EXPRESSION:
        case ir::AstNodeType::ETS_NEW_MULTI_DIM_ARRAY_INSTANCE_EXPRESSION:
        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION:
        case ir::AstNodeType::ETS_PARAMETER_EXPRESSION:
        case ir::AstNodeType::ETS_TUPLE:
            return std::make_tuple(true, CompletionEntryKind::OPERATOR);
        case ir::AstNodeType::ETS_FUNCTION_TYPE:
            return std::make_tuple(true, CompletionEntryKind::FUNCTION);
        case ir::AstNodeType::ETS_PACKAGE_DECLARATION:
        case ir::AstNodeType::ETS_IMPORT_DECLARATION:
        case ir::AstNodeType::ETS_MODULE:
            return std::make_tuple(true, CompletionEntryKind::MODULE);
        case ir::AstNodeType::ETS_TYPE_REFERENCE:
        case ir::AstNodeType::ETS_TYPE_REFERENCE_PART:
            return std::make_tuple(true, CompletionEntryKind::REFERENCE);
        case ir::AstNodeType::ETS_UNION_TYPE:
            return std::make_tuple(true, CompletionEntryKind::TYPE_PARAMETER);
        default:
            break;
    }
    return std::make_tuple(false, CompletionEntryKind::ALIAS_TYPE);
}

bool IsTSParameterKind(ir::AstNodeType type)
{
    switch (type) {
        case ir::AstNodeType::TS_ARRAY_TYPE:
        case ir::AstNodeType::TS_UNION_TYPE:
        case ir::AstNodeType::TS_INTERSECTION_TYPE:
        case ir::AstNodeType::TS_LITERAL_TYPE:
        case ir::AstNodeType::TS_MAPPED_TYPE:
        case ir::AstNodeType::TS_THIS_TYPE:
        case ir::AstNodeType::TS_TYPE_PARAMETER:
        case ir::AstNodeType::TS_TYPE_PARAMETER_DECLARATION:
        case ir::AstNodeType::TS_TYPE_PARAMETER_INSTANTIATION:
        case ir::AstNodeType::TS_TYPE_PREDICATE:
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION:
        case ir::AstNodeType::TS_TYPE_REFERENCE:
        case ir::AstNodeType::TS_INDEXED_ACCESS_TYPE:
        case ir::AstNodeType::TS_TUPLE_TYPE:
            return true;
        default:
            break;
    }
    return false;
}

bool IsTSMoudleKind(ir::AstNodeType type)
{
    switch (type) {
        case ir::AstNodeType::TS_IMPORT_TYPE:
        case ir::AstNodeType::TS_MODULE_BLOCK:
        case ir::AstNodeType::TS_EXTERNAL_MODULE_REFERENCE:
        case ir::AstNodeType::TS_MODULE_DECLARATION:
        case ir::AstNodeType::TS_IMPORT_EQUALS_DECLARATION:
            return true;
        default:
            break;
    }
    return false;
}

std::tuple<bool, CompletionEntryKind> GetTargetTokenKindIfTSType(ir::AstNodeType type)
{
    auto reuslt = std::make_tuple(false, CompletionEntryKind::ALIAS_TYPE);
    if (IsValidAncestorType(type)) {
        reuslt = std::make_tuple(true, CompletionEntryKind::KEYWORD);
    } else if (IsTSParameterKind(type)) {
        reuslt = std::make_tuple(true, CompletionEntryKind::TYPE_PARAMETER);
    } else if (IsTSMoudleKind(type)) {
        reuslt = std::make_tuple(true, CompletionEntryKind::MODULE);
    }
    switch (type) {
        case ir::AstNodeType::TS_ENUM_DECLARATION:
            return std::make_tuple(true, CompletionEntryKind::ENUM);
        case ir::AstNodeType::TS_ENUM_MEMBER:
            return std::make_tuple(true, CompletionEntryKind::ENUM_MEMBER);
        case ir::AstNodeType::TS_NON_NULL_EXPRESSION:
        case ir::AstNodeType::TS_TYPE_OPERATOR:
        case ir::AstNodeType::TS_AS_EXPRESSION:
            return std::make_tuple(true, CompletionEntryKind::OPERATOR);
        case ir::AstNodeType::TS_TYPE_LITERAL:
            return std::make_tuple(true, CompletionEntryKind::VALUE);
        case ir::AstNodeType::TS_PROPERTY_SIGNATURE:
            return std::make_tuple(true, CompletionEntryKind::PROPERTY);
        case ir::AstNodeType::TS_METHOD_SIGNATURE:
            return std::make_tuple(true, CompletionEntryKind::METHOD);
        case ir::AstNodeType::TS_FUNCTION_TYPE:
            return std::make_tuple(true, CompletionEntryKind::FUNCTION);
        case ir::AstNodeType::TS_CONSTRUCTOR_TYPE:
            return std::make_tuple(true, CompletionEntryKind::CONSTRUCTOR);
        case ir::AstNodeType::TS_NAMED_TUPLE_MEMBER:
            return std::make_tuple(true, CompletionEntryKind::VALUE);
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
        case ir::AstNodeType::TS_INTERFACE_BODY:
        case ir::AstNodeType::TS_INTERFACE_HERITAGE:
            return std::make_tuple(true, CompletionEntryKind::INTERFACE);
        case ir::AstNodeType::TS_SIGNATURE_DECLARATION:
        case ir::AstNodeType::TS_PARENT_TYPE:
        case ir::AstNodeType::TS_INFER_TYPE:
        case ir::AstNodeType::TS_CONDITIONAL_TYPE:
        case ir::AstNodeType::TS_PARAMETER_PROPERTY:
        case ir::AstNodeType::TS_QUALIFIED_NAME:
        case ir::AstNodeType::TS_INDEX_SIGNATURE:
        case ir::AstNodeType::TS_TYPE_QUERY:
        case ir::AstNodeType::TS_CLASS_IMPLEMENTS:
        case ir::AstNodeType::TS_TYPE_ASSERTION:
            return std::make_tuple(true, CompletionEntryKind::TEXT);
        default:
            break;
    }
    return reuslt;
}

bool IsExpress(ir::AstNodeType type)
{
    switch (type) {
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
        case ir::AstNodeType::AWAIT_EXPRESSION:
        case ir::AstNodeType::BINARY_EXPRESSION:
        case ir::AstNodeType::CALL_EXPRESSION:
        case ir::AstNodeType::CHAIN_EXPRESSION:
        case ir::AstNodeType::CLASS_EXPRESSION:
        case ir::AstNodeType::CONDITIONAL_EXPRESSION:
        case ir::AstNodeType::DIRECT_EVAL:
        case ir::AstNodeType::FUNCTION_EXPRESSION:
        case ir::AstNodeType::MEMBER_EXPRESSION:
        case ir::AstNodeType::META_PROPERTY_EXPRESSION:
        case ir::AstNodeType::NEW_EXPRESSION:
        case ir::AstNodeType::OMITTED_EXPRESSION:
        case ir::AstNodeType::PREFIX_ASSERTION_EXPRESSION:
        case ir::AstNodeType::SEQUENCE_EXPRESSION:
        case ir::AstNodeType::SUPER_EXPRESSION:
        case ir::AstNodeType::TAGGED_TEMPLATE_EXPRESSION:
        case ir::AstNodeType::THIS_EXPRESSION:
        case ir::AstNodeType::TYPEOF_EXPRESSION:
        case ir::AstNodeType::UNARY_EXPRESSION:
        case ir::AstNodeType::UPDATE_EXPRESSION:
        case ir::AstNodeType::YIELD_EXPRESSION:
        case ir::AstNodeType::BLOCK_EXPRESSION:
            return true;
        default:
            break;
    }
    return false;
}

bool IsStatement(ir::AstNodeType type)
{
    switch (type) {
        case ir::AstNodeType::ASSERT_STATEMENT:
        case ir::AstNodeType::CONTINUE_STATEMENT:
        case ir::AstNodeType::IF_STATEMENT:
        case ir::AstNodeType::DEBUGGER_STATEMENT:
        case ir::AstNodeType::DO_WHILE_STATEMENT:
        case ir::AstNodeType::EMPTY_STATEMENT:
        case ir::AstNodeType::EXPRESSION_STATEMENT:
        case ir::AstNodeType::BLOCK_STATEMENT:
        case ir::AstNodeType::BREAK_STATEMENT:
        case ir::AstNodeType::CATCH_CLAUSE:
        case ir::AstNodeType::FOR_IN_STATEMENT:
        case ir::AstNodeType::FOR_OF_STATEMENT:
        case ir::AstNodeType::FOR_UPDATE_STATEMENT:
        case ir::AstNodeType::REEXPORT_STATEMENT:
        case ir::AstNodeType::RETURN_STATEMENT:
        case ir::AstNodeType::LABELLED_STATEMENT:
        case ir::AstNodeType::SWITCH_CASE_STATEMENT:
        case ir::AstNodeType::SWITCH_STATEMENT:
        case ir::AstNodeType::THROW_STATEMENT:
        case ir::AstNodeType::TRY_STATEMENT:
        case ir::AstNodeType::WHILE_STATEMENT:
            return true;
        default:
            break;
    }
    return false;
}

bool IsLiteral(ir::AstNodeType type)
{
    switch (type) {
        case ir::AstNodeType::BIGINT_LITERAL:
        case ir::AstNodeType::BOOLEAN_LITERAL:
        case ir::AstNodeType::CHAR_LITERAL:
        case ir::AstNodeType::NULL_LITERAL:
        case ir::AstNodeType::UNDEFINED_LITERAL:
        case ir::AstNodeType::NUMBER_LITERAL:
        case ir::AstNodeType::REGEXP_LITERAL:
        case ir::AstNodeType::STRING_LITERAL:
        case ir::AstNodeType::TEMPLATE_LITERAL:
            return true;
        default:
            break;
    }
    return false;
}

bool IsModule(ir::AstNodeType type)
{
    switch (type) {
        case ir::AstNodeType::EXPORT_ALL_DECLARATION:
        case ir::AstNodeType::EXPORT_DEFAULT_DECLARATION:
        case ir::AstNodeType::EXPORT_NAMED_DECLARATION:
        case ir::AstNodeType::EXPORT_SPECIFIER:
        case ir::AstNodeType::IMPORT_DECLARATION:
        case ir::AstNodeType::IMPORT_EXPRESSION:
        case ir::AstNodeType::IMPORT_DEFAULT_SPECIFIER:
        case ir::AstNodeType::IMPORT_NAMESPACE_SPECIFIER:
        case ir::AstNodeType::IMPORT_SPECIFIER:
            return true;
        default:
            break;
    }
    return false;
}

CompletionEntryKind GetTargetTokenKind(const ir::AstNode *node)
{
    CompletionEntryKind normalResult = CompletionEntryKind::ALIAS_TYPE;
    if (node == nullptr) {
        return normalResult;
    }
    auto type = node->Type();
    if (IsExpress(type)) {
        normalResult = CompletionEntryKind::OPERATOR;
    } else if (IsStatement(type)) {
        normalResult = CompletionEntryKind::SNIPPET;
    } else if (IsLiteral(type)) {
        normalResult = CompletionEntryKind::VALUE;
    } else if (IsModule(type)) {
        normalResult = CompletionEntryKind::MODULE;
    }
    switch (type) {
        case ir::AstNodeType::CLASS_DEFINITION:
        case ir::AstNodeType::CLASS_DECLARATION:
            return CompletionEntryKind::CLASS;
        case ir::AstNodeType::CLASS_PROPERTY:
        case ir::AstNodeType::PROPERTY:
            return CompletionEntryKind::FIELD;
        case ir::AstNodeType::FUNCTION_DECLARATION:
        case ir::AstNodeType::SCRIPT_FUNCTION:
            return CompletionEntryKind::FUNCTION;
        case ir::AstNodeType::METHOD_DEFINITION: {
            auto kind = node->AsMethodDefinition()->Kind();
            return (kind == ir::MethodDefinitionKind::CONSTRUCTOR) ? CompletionEntryKind::CONSTRUCTOR
                                                                   : CompletionEntryKind::METHOD;
        }
        case ir::AstNodeType::NAMED_TYPE:
            return CompletionEntryKind::TYPE_PARAMETER;
        case ir::AstNodeType::STRUCT_DECLARATION:
            return CompletionEntryKind::STRUCT;
        case ir::AstNodeType::VARIABLE_DECLARATION:
        case ir::AstNodeType::VARIABLE_DECLARATOR:
            return CompletionEntryKind::VARIABLE;
        default:
            auto etsResult = GetTargetTokenKindIfETSType(type);
            if (std::get<0>(etsResult)) {
                return std::get<1>(etsResult);
            }
            auto tsResult = GetTargetTokenKindIfTSType(type);
            if (std::get<0>(tsResult)) {
                return std::get<1>(tsResult);
            }
    }
    return normalResult;
}

const ir::TSTypeAliasDeclaration *GetAliasDeclFromCurrentToken(const ir::AstNode *node)
{
    if (node == nullptr) {
        return nullptr;
    }
    const ir::TSTypeAliasDeclaration *aliasDecl = nullptr;
    if (node->IsTSTypeAliasDeclaration()) {
        aliasDecl = node->AsTSTypeAliasDeclaration();
    } else if (node->IsIdentifier()) {
        auto decl = compiler::DeclarationFromIdentifier(node->AsIdentifier());
        if (decl == nullptr) {
            return nullptr;
        }
        aliasDecl = decl->AsTSTypeAliasDeclaration();
    }
    return aliasDecl;
}

CompletionEntryKind GetAliasScriptElementKindImpl(es2panda_Context *context, size_t position)
{
    auto touchingToken = GetTouchingToken(context, position, false);
    auto aliasDecl = GetAliasDeclFromCurrentToken(touchingToken);
    if (aliasDecl == nullptr) {
        return CompletionEntryKind::TEXT;
    }
    auto typeAnnotation = aliasDecl->TypeAnnotation();
    if (typeAnnotation == nullptr) {
        return CompletionEntryKind::ALIAS_TYPE;
    }
    if (typeAnnotation->IsETSTypeReference()) {
        auto part = typeAnnotation->AsETSTypeReference()->Part()->AsETSTypeReferencePart();
        if (part == nullptr) {
            return CompletionEntryKind::ALIAS_TYPE;
        }
        auto targetIdent = part->GetIdent();
        auto decl = compiler::DeclarationFromIdentifier(targetIdent);
        if (compiler::ClassDefinitionIsEnumTransformed(decl)) {
            return CompletionEntryKind::ENUM;
        }
        return GetTargetTokenKind(decl);
    }
    return GetTargetTokenKind(typeAnnotation);
}
}  // namespace ark::es2panda::lsp
