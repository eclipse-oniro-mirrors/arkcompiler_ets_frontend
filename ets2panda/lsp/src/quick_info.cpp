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

#include "quick_info.h"
#include "internal_api.h"
#include "ir/astNode.h"
#include "public/public.h"
#include "ir/ets/etsTuple.h"
#include "ir/ets/etsUnionType.h"
#include "api.h"
#include "rename.h"
#include "isolated_declaration.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::lsp {

ir::AstNode *GetEnumMemberByName(ir::AstNode *node, const util::StringView &name)
{
    if (node->Type() != ir::AstNodeType::TS_ENUM_DECLARATION) {
        return nullptr;
    }
    auto enumDecl = node->AsTSEnumDeclaration();
    auto enumMember = enumDecl->FindChild([&name](ir::AstNode *child) {
        return child->IsTSEnumMember() && child->AsTSEnumMember()->Key()->AsIdentifier()->Name() == name;
    });
    return enumMember == nullptr ? nullptr : enumMember;
}

bool IsIncludedToken(const ir::AstNode *node)
{
    auto type = node->Type();
    static const std::unordered_set<ir::AstNodeType> INCLUDED_TOKEN_TYPES = {
        ir::AstNodeType::IDENTIFIER,
        ir::AstNodeType::METHOD_DEFINITION,
        ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION,
        ir::AstNodeType::CLASS_DECLARATION,
        ir::AstNodeType::ETS_TUPLE,
        ir::AstNodeType::STRING_LITERAL,
        ir::AstNodeType::NUMBER_LITERAL,
        ir::AstNodeType::TEMPLATE_LITERAL,
        ir::AstNodeType::TEMPLATE_ELEMENT,
        ir::AstNodeType::ASSIGNMENT_EXPRESSION,
    };
    return INCLUDED_TOKEN_TYPES.find(type) != INCLUDED_TOKEN_TYPES.end();
}

ir::AstNode *GetTokenForQuickInfo(es2panda_Context *context, size_t position)
{
    auto node = GetTouchingToken(context, position, false);
    if (node == nullptr) {
        return nullptr;
    }
    if (!IsIncludedToken(node)) {
        return nullptr;
    }
    return node;
}

bool IsObjectLiteralElement(ir::AstNode *node)
{
    auto type = node->Type();
    static const std::unordered_set<ir::AstNodeType> INCLUDED_OBJECT_LITERAL_ELEMENT_TYPES = {
        ir::AstNodeType::PROPERTY,
    };
    return INCLUDED_OBJECT_LITERAL_ELEMENT_TYPES.find(type) != INCLUDED_OBJECT_LITERAL_ELEMENT_TYPES.end();
}

ir::AstNode *GetContainingObjectLiteralNode(ir::AstNode *node)
{
    if (node == nullptr) {
        return nullptr;
    }
    auto type = node->Type();
    if (type == ir::AstNodeType::STRING_LITERAL || type == ir::AstNodeType::NUMBER_LITERAL ||
        type == ir::AstNodeType::TEMPLATE_LITERAL || type == ir::AstNodeType::IDENTIFIER) {
        if (IsObjectLiteralElement(node->Parent())) {
            return node->Parent();
        }
    } else if (type == ir::AstNodeType::TEMPLATE_ELEMENT) {
        if (IsObjectLiteralElement(node->Parent()->Parent())) {
            return node->Parent()->Parent();
        }
    }
    return nullptr;
}

ir::AstNode *GetContextualTypeNode(ir::AstNode *node)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (node->Type() == ir::AstNodeType::OBJECT_EXPRESSION) {
        if (node->Parent()->Type() == ir::AstNodeType::CLASS_PROPERTY) {
            auto propertyObj = node->Parent()->AsClassElement();
            auto type = propertyObj->TsType();
            // note: There should not be a null pointer here.
            // This is a temporary workaround to avoid null pointer issues.
            // A proper fix may require modifying the checker-related code in issue 30716.
            if ((type->Variable() == nullptr) || (type->Variable()->Declaration() == nullptr)) {
                return nullptr;
            }
            auto contextualTypeNode = type->Variable()->Declaration()->Node();
            return contextualTypeNode;
        }
        if (node->Parent()->Type() == ir::AstNodeType::ASSIGNMENT_EXPRESSION) {
            auto propertyObj = node->Parent()->AsAssignmentExpression();
            auto type = propertyObj->TsType();
            // note: There should not be a null pointer here.
            // This is a temporary workaround to avoid null pointer issues.
            // A proper fix may require modifying the checker-related code in issue 30716.
            if ((type->Variable() == nullptr) || (type->Variable()->Declaration() == nullptr)) {
                return nullptr;
            }
            auto contextualTypeNode = type->Variable()->Declaration()->Node();
            return contextualTypeNode;
        }
    }
    return nullptr;
}

ir::AstNode *GetPropertyNodeFromContextualType(ir::AstNode *node, ir::AstNode *contextualTypeNode)
{
    auto type = contextualTypeNode->Type();
    auto property = node->AsProperty()->Key();
    ark::es2panda::util::StringView propertyName;
    if (property->Type() == ir::AstNodeType::STRING_LITERAL) {
        propertyName = property->AsStringLiteral()->Str();
    } else if (property->Type() == ir::AstNodeType::IDENTIFIER) {
        propertyName = property->AsIdentifier()->Name();
    }
    if (type == ir::AstNodeType::CLASS_DEFINITION) {
        auto def = contextualTypeNode->AsClassDefinition();
        auto bodies = def->Body();
        for (auto it : bodies) {
            auto methodDef = it->AsMethodDefinition();
            auto name = methodDef->Key()->AsIdentifier()->Name();
            if (name == propertyName) {
                return it;
            }
        }
    }
    if (type == ir::AstNodeType::TS_INTERFACE_DECLARATION) {
        auto def = contextualTypeNode->AsTSInterfaceDeclaration();
        auto bodies = def->Body()->Body();
        for (auto it : bodies) {
            auto methodDef = it->AsMethodDefinition();
            auto name = methodDef->Key()->AsIdentifier()->Name();
            if (name == propertyName) {
                return it;
            }
        }
    }
    return node;
}

bool IsDeclaration(ir::AstNode *node)
{
    return node->Type() == ir::AstNodeType::CLASS_DECLARATION ||
           node->Type() == ir::AstNodeType::FUNCTION_DECLARATION ||
           node->Type() == ir::AstNodeType::IMPORT_DECLARATION ||
           node->Type() == ir::AstNodeType::ANNOTATION_DECLARATION ||
           node->Type() == ir::AstNodeType::EXPORT_ALL_DECLARATION ||
           node->Type() == ir::AstNodeType::EXPORT_DEFAULT_DECLARATION ||
           node->Type() == ir::AstNodeType::EXPORT_NAMED_DECLARATION ||
           node->Type() == ir::AstNodeType::ETS_PACKAGE_DECLARATION ||
           node->Type() == ir::AstNodeType::ETS_IMPORT_DECLARATION ||
           node->Type() == ir::AstNodeType::STRUCT_DECLARATION ||
           node->Type() == ir::AstNodeType::TS_ENUM_DECLARATION ||
           node->Type() == ir::AstNodeType::TS_SIGNATURE_DECLARATION ||
           node->Type() == ir::AstNodeType::TS_TYPE_PARAMETER_DECLARATION ||
           node->Type() == ir::AstNodeType::TS_MODULE_DECLARATION ||
           node->Type() == ir::AstNodeType::TS_IMPORT_EQUALS_DECLARATION ||
           node->Type() == ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION ||
           node->Type() == ir::AstNodeType::TS_INTERFACE_DECLARATION ||
           node->Type() == ir::AstNodeType::VARIABLE_DECLARATION;
}

bool IsDefinition(ir::AstNode *node)
{
    return node->Type() == ir::AstNodeType::CLASS_DEFINITION || node->Type() == ir::AstNodeType::METHOD_DEFINITION;
}

bool IsIdentifierOfDeclaration(ir::AstNode *node)
{
    return node->Parent()->Type() == ir::AstNodeType::IMPORT_SPECIFIER ||
                   (node->Parent()->Type() == ir::AstNodeType::EXPORT_SPECIFIER)
               ? node->Type() == ir::AstNodeType::IDENTIFIER
               : node->Type() == ir::AstNodeType::IDENTIFIER && IsDeclaration(node->Parent());
}

ir::AstNode *GetNodeAtLocation(ir::AstNode *node)
{
    if (node->IsProgram()) {
        return node->Modifiers() == ir::ModifierFlags::EXPORT ? node : nullptr;
    }
    if (node->Type() == ir::AstNodeType::IDENTIFIER) {
        // could get decl instead of getting parant declaration and then child
        return compiler::DeclarationFromIdentifier(node->AsIdentifier());
    }

    return nullptr;
}

ir::AstNode *GetNodeAtLocationForQuickInfo(ir::AstNode *node)
{
    if (node != nullptr) {
        auto object = GetContainingObjectLiteralNode(node);
        if (object != nullptr) {
            auto contextualTypeNode = GetContextualTypeNode(object->Parent());
            if (contextualTypeNode != nullptr) {
                return GetPropertyNodeFromContextualType(object, contextualTypeNode);
            }
        }
        return GetNodeAtLocation(node);
    }
    return nullptr;
}

std::string ModifiersToString(ir::ModifierFlags flags)
{
    std::ostringstream oss;
    bool first = true;
    auto addModifier = [flags, &oss, &first](ir::ModifierFlags flag, const std::string &name) {
        if ((static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0) {
            if (!first) {
                oss << " ";
            }
            oss << name;
            first = false;
        }
    };
    addModifier(ir::ModifierFlags::STATIC, "static");
    addModifier(ir::ModifierFlags::ASYNC, "async");
    addModifier(ir::ModifierFlags::PUBLIC, "public");
    addModifier(ir::ModifierFlags::PROTECTED, "protected");
    addModifier(ir::ModifierFlags::PRIVATE, "private");
    addModifier(ir::ModifierFlags::DECLARE, "declare");
    addModifier(ir::ModifierFlags::READONLY, "readonly");
    addModifier(ir::ModifierFlags::OPTIONAL, "optional");
    addModifier(ir::ModifierFlags::DEFINITE, "definite");
    addModifier(ir::ModifierFlags::ABSTRACT, "abstract");
    addModifier(ir::ModifierFlags::CONST, "const");
    addModifier(ir::ModifierFlags::FINAL, "final");
    addModifier(ir::ModifierFlags::NATIVE, "native");
    addModifier(ir::ModifierFlags::OVERRIDE, "override");
    addModifier(ir::ModifierFlags::CONSTRUCTOR, "constructor");
    addModifier(ir::ModifierFlags::SYNCHRONIZED, "synchronized");
    addModifier(ir::ModifierFlags::FUNCTIONAL, "functional");
    addModifier(ir::ModifierFlags::IN, "in");
    addModifier(ir::ModifierFlags::OUT, "out");
    addModifier(ir::ModifierFlags::INTERNAL, "internal");
    addModifier(ir::ModifierFlags::EXPORT, "export");
    addModifier(ir::ModifierFlags::GETTER, "getter");
    addModifier(ir::ModifierFlags::SETTER, "setter");
    addModifier(ir::ModifierFlags::DEFAULT_EXPORT, "default_export");
    addModifier(ir::ModifierFlags::SUPER_OWNER, "super_owner");
    addModifier(ir::ModifierFlags::ANNOTATION_DECLARATION, "annotation_declaration");
    addModifier(ir::ModifierFlags::ANNOTATION_USAGE, "annotation_usage");
    addModifier(ir::ModifierFlags::READONLY_PARAMETER, "readonly_parameter");
    return oss.str();
}

std::string GetKindModifiers(ir::AstNode *node)
{
    auto flags = node->Modifiers();
    return ModifiersToString(flags);
}

ir::AstNode *GetContainerNode(ir::AstNode *node)
{
    if (node == nullptr) {
        return nullptr;
    }
    while (!node->IsProgram()) {
        switch (node->Type()) {
            case ir::AstNodeType::TS_METHOD_SIGNATURE:
            case ir::AstNodeType::FUNCTION_DECLARATION:
            case ir::AstNodeType::FUNCTION_EXPRESSION:
            case ir::AstNodeType::CLASS_DECLARATION:
            case ir::AstNodeType::STRUCT_DECLARATION:
            case ir::AstNodeType::TS_INTERFACE_DECLARATION:
            case ir::AstNodeType::TS_ENUM_DECLARATION:
            case ir::AstNodeType::TS_MODULE_DECLARATION:
            case ir::AstNodeType::METHOD_DEFINITION:
                return node;
            default:
                node = node->Parent();
        }
    }
    return node;
}

std::string GetNodeFileName(ir::AstNode *contextualTypeNode)
{
    if (!contextualTypeNode->IsETSImportDeclaration()) {
        return "";
    }
    return std::string(contextualTypeNode->AsETSImportDeclaration()->ResolvedSource());
}

bool IsClass(ir::AstNode *node)
{
    return node->Type() == ir::AstNodeType::STRUCT_DECLARATION || node->Type() == ir::AstNodeType::CLASS_DECLARATION ||
           node->Type() == ir::AstNodeType::CLASS_EXPRESSION || node->Type() == ir::AstNodeType::CLASS_DEFINITION;
}

bool IsCallOrNewExpression(ir::AstNode *node)
{
    return node->Type() == ir::AstNodeType::CALL_EXPRESSION || node->Type() == ir::AstNodeType::NEW_EXPRESSION;
}

bool IsTaggedTemplateExpression(ir::AstNode *node)
{
    return node->Type() == ir::AstNodeType::TAGGED_TEMPLATE_EXPRESSION;
}

bool IsFunctionType(ir::AstNode *node)
{
    return node->Type() == ir::AstNodeType::TS_METHOD_SIGNATURE ||
           node->Type() == ir::AstNodeType::TS_INDEX_SIGNATURE || node->Type() == ir::AstNodeType::ETS_FUNCTION_TYPE ||
           node->Type() == ir::AstNodeType::TS_CONSTRUCTOR_TYPE ||
           node->Type() == ir::AstNodeType::FUNCTION_DECLARATION ||
           node->Type() == ir::AstNodeType::FUNCTION_EXPRESSION ||
           node->Type() == ir::AstNodeType::ARROW_FUNCTION_EXPRESSION;
}

bool IsFunctionLike(ir::AstNode *node)
{
    return node != nullptr && IsFunctionType(node);
}

std::string GetNodeKind(ir::AstNode *node)
{
    if (node->Modifiers() != ir::ModifierFlags::NONE) {
        return ModifiersToString(node->Modifiers());
    }
    if (node->Type() == ir::AstNodeType::PROPERTY) {
        return "property";
    }
    if (IsClass(node)) {
        return "class";
    }
    return "";
}

std::string TransDisplayPartsToStr(const std::vector<SymbolDisplayPart> &displayParts)
{
    std::stringstream ss;
    for (const auto &part : displayParts) {
        ss << "[" << part.GetText() << "](" << part.GetKind() << ")" << std::endl;
    }
    return ss.str();
}

std::string PrimitiveTypeToName(ir::PrimitiveType type)
{
    switch (type) {
        case ir::PrimitiveType::BYTE:
            return "byte";
        case ir::PrimitiveType::INT:
            return "int";
        case ir::PrimitiveType::LONG:
            return "long";
        case ir::PrimitiveType::SHORT:
            return "short";
        case ir::PrimitiveType::FLOAT:
            return "float";
        case ir::PrimitiveType::DOUBLE:
            return "double";
        case ir::PrimitiveType::BOOLEAN:
            return "boolean";
        case ir::PrimitiveType::CHAR:
            return "char";
        case ir::PrimitiveType::VOID:
            return "void";
        default:
            UNREACHABLE();
    }
}

std::string GetNameForUnionType(const ir::TypeNode *unionType)
{
    const auto &types = unionType->AsETSUnionType()->Types();
    std::string newstr;
    for (size_t i = 0; i < types.size(); ++i) {
        newstr += GetNameForTypeNode(types[i]);
        if (i != types.size() - 1) {
            newstr += " | ";
        }
    }
    return newstr;
}

std::string GetNameForTupleType(const ir::TypeNode *tupleType)
{
    const auto &types = tupleType->AsETSTuple()->GetTupleTypeAnnotationsList();
    std::string newstr = "[";
    for (size_t i = 0; i < types.size(); ++i) {
        newstr += GetNameForTypeNode(types[i]);
        if (i != types.size() - 1) {
            newstr += ", ";
        }
    }
    newstr += "]";
    return newstr;
}

std::string GetNameForTypeReference(const ir::TypeNode *typeReference)
{
    auto type = typeReference->AsETSTypeReference()->Part();
    if (type != nullptr && type->IsETSTypeReferencePart()) {
        auto tmp = type->AsETSTypeReferencePart()->Name();
        if (tmp != nullptr && tmp->IsTSQualifiedName()) {
            auto leftStr = tmp->AsTSQualifiedName()->Left()->AsIdentifier()->Name().Mutf8();
            auto rightStr = tmp->AsTSQualifiedName()->Right()->AsIdentifier()->Name().Mutf8();
            return leftStr + "." + rightStr;
        }
    }

    std::string typeParamNames;
    auto typeParam = typeReference->AsETSTypeReference()->Part()->TypeParams();
    if (typeParam != nullptr && typeParam->IsTSTypeParameterInstantiation()) {
        typeParamNames = "<";
        for (auto param : typeParam->Params()) {
            typeParamNames += GetNameForTypeNode(param) + ",";
        }
        typeParamNames.pop_back();
        typeParamNames += ">";
    }
    return typeReference->AsETSTypeReference()->Part()->GetIdent()->Name().Mutf8() + typeParamNames;
}

std::string GetNameForFunctionType(const ir::TypeNode *functionType)
{
    std::string params;
    for (const auto *param : functionType->AsETSFunctionType()->Params()) {
        params += param->AsETSParameterExpression()->Name().Mutf8() + ":" +
                  GetNameForTypeNode(param->AsETSParameterExpression()->TypeAnnotation()) + ",";
    }
    if (!params.empty()) {
        params.pop_back();
    }
    const std::string returnType = GetNameForTypeNode(functionType->AsETSFunctionType()->ReturnType());
    return "((" + params + ") => " + returnType + ")";
}

std::string EscapeJsonString(const std::string &input)
{
    std::ostringstream oss;
    oss << "\"";
    for (char c : input) {
        switch (c) {
            case '\"':
                oss << "\\\"";
                break;
            case '\\':
                oss << "\\\\";
                break;
            case '\n':
                oss << "\\n";
                break;
            case '\r':
                oss << "\\r";
                break;
            case '\t':
                oss << "\\t";
                break;
            default:
                oss << c;
                break;
        }
    }
    oss << "\"";
    return oss.str();
}

std::string GetNameForLiteralTypeNode(const ir::AstNode *node, bool iskindModifier = false)
{
    if (node == nullptr) {
        return "undefined";
    }
    if (node->IsStringLiteral()) {
        return iskindModifier ? EscapeJsonString(std::string(node->AsStringLiteral()->Str())) : "String";
    }
    if (node->IsNumberLiteral()) {
        return iskindModifier ? std::string(node->AsNumberLiteral()->Str()) : "Number";
    }
    if (node->IsBooleanLiteral()) {
        return iskindModifier ? node->AsBooleanLiteral()->Value() ? "true" : "false" : "Boolean";
    }
    if (node->IsBigIntLiteral()) {
        return "Bigint";
    }
    if (node->IsIdentifier()) {
        auto name = node->AsIdentifier()->Name();
        if (name.Is("NaN")) {
            return "Number";
        }
    }
    if (node->IsNullLiteral()) {
        return "null";
    }
    if (node->IsETSStringLiteralType()) {
        return "String";
    }
    return "undefined";
}

std::string GetNameForTypeNode(const ir::TypeNode *typeAnnotation)
{
    if (typeAnnotation->IsETSUnionType()) {
        return GetNameForUnionType(typeAnnotation);
    }
    if (typeAnnotation->IsETSPrimitiveType()) {
        return PrimitiveTypeToName(typeAnnotation->AsETSPrimitiveType()->GetPrimitiveType());
    }
    if (typeAnnotation->IsETSTypeReference()) {
        return GetNameForTypeReference(typeAnnotation);
    }
    if (typeAnnotation->IsETSFunctionType()) {
        return GetNameForFunctionType(typeAnnotation);
    }
    if (typeAnnotation->IsTSArrayType()) {
        return GetNameForTypeNode(typeAnnotation->AsTSArrayType()->ElementType()) + "[]";
    }
    if (typeAnnotation->IsETSNullType()) {
        return "null";
    }
    if (typeAnnotation->IsETSUndefinedType()) {
        return "undefined";
    }
    if (typeAnnotation->IsETSTuple()) {
        return GetNameForTupleType(typeAnnotation);
    }
    return GetNameForLiteralTypeNode(typeAnnotation);
}

std::string GetNameForETSUnionType(const ir::TypeNode *typeAnnotation)
{
    ASSERT(typeAnnotation->IsETSUnionType());
    std::string newstr;
    for (size_t i = 0; i < typeAnnotation->AsETSUnionType()->Types().size(); i++) {
        auto type = typeAnnotation->AsETSUnionType()->Types()[i];
        std::string str = GetNameForTypeNode(type);
        newstr += str;
        if (i != typeAnnotation->AsETSUnionType()->Types().size() - 1) {
            newstr += " | ";
        }
    }
    return newstr;
}

std::vector<SymbolDisplayPart> MergeSymbolDisplayPart(const std::vector<SymbolDisplayPart> &symbolDisplayPart1,
                                                      const std::vector<SymbolDisplayPart> &symbolDisplayPart2)
{
    std::vector<SymbolDisplayPart> result;
    result.reserve(symbolDisplayPart1.size() + symbolDisplayPart2.size());

    for (const SymbolDisplayPart &part : symbolDisplayPart1) {
        result.emplace_back(part);
    }
    for (const SymbolDisplayPart &part : symbolDisplayPart2) {
        result.emplace_back(part);
    }
    return result;
}

std::string GetNameFromClassExpression(ir::AstNode *node)
{
    if (node == nullptr || !node->IsClassExpression()) {
        return "";
    }
    if (node->AsClassExpression()->Variable() == nullptr) {
        return "";
    }
    return std::string(node->AsClassExpression()->Variable()->Name());
}

std::string GetNameFromETSStructDeclaration(ir::AstNode *node)
{
    if (node == nullptr || !node->IsETSStructDeclaration()) {
        return "";
    }
    if (node->AsETSStructDeclaration()->Definition() == nullptr) {
        return "";
    }
    if (node->AsETSStructDeclaration()->Definition()->Ident() == nullptr) {
        return "";
    }
    return std::string(node->AsETSStructDeclaration()->Definition()->Ident()->Name());
}

std::string GetNameFromClassDeclaration(ir::AstNode *node)
{
    if (node == nullptr || !node->IsClassDeclaration()) {
        return "";
    }
    if (node->AsClassDeclaration()->Definition() == nullptr) {
        return "";
    }
    if (node->AsClassDeclaration()->Definition()->Ident() == nullptr) {
        return "";
    }
    return std::string(node->AsClassDeclaration()->Definition()->Ident()->Name());
}

std::string GetNameFromClassDefinition(ir::AstNode *node)
{
    if (node == nullptr || !node->IsClassDefinition()) {
        return "";
    }
    if (node->AsClassDefinition()->Ident() == nullptr) {
        return "";
    }
    return std::string(node->AsClassDefinition()->Ident()->Name());
}

std::vector<SymbolDisplayPart> CreateDisplayForClass(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (!IsClass(node)) {
        return displayParts;
    }
    if (node->Type() == ir::AstNodeType::CLASS_EXPRESSION) {
        displayParts.emplace_back(CreatePunctuation("("));
        displayParts.emplace_back(CreateKeyword("local class"));
        displayParts.emplace_back(CreatePunctuation(")"));
        displayParts.emplace_back(CreateSpace());
        displayParts.emplace_back(CreateClassName(GetNameFromClassExpression(node)));
    } else if (node->Type() == ir::AstNodeType::STRUCT_DECLARATION) {
        displayParts.emplace_back(CreateKeyword("struct"));
        displayParts.emplace_back(CreateSpace());
        displayParts.emplace_back(CreateClassName(GetNameFromETSStructDeclaration(node)));
    } else if (node->Type() == ir::AstNodeType::CLASS_DECLARATION) {
        displayParts.emplace_back(CreateKeyword("class"));
        displayParts.emplace_back(CreateSpace());
        displayParts.emplace_back(CreateClassName(GetNameFromClassDeclaration(node)));
    } else {
        // class definition
        if (node->AsClassDefinition()->OrigEnumDecl() != nullptr) {
            displayParts.emplace_back(CreateKeyword("enum"));
            displayParts.emplace_back(CreateSpace());
            displayParts.emplace_back(CreateEnumName(GetNameFromClassDefinition(node)));
        } else if (node->AsClassDefinition()->IsNamespaceTransformed()) {
            displayParts.emplace_back(CreateKeyword("namespace"));
            displayParts.emplace_back(CreateSpace());
            displayParts.emplace_back(CreateNamespace(GetNameFromClassDefinition(node)));
        } else if (node->AsClassDefinition()->IsFromStruct() || node->Parent()->IsETSStructDeclaration()) {
            displayParts.emplace_back(CreateKeyword("struct"));
            displayParts.emplace_back(CreateSpace());
            displayParts.emplace_back(SignatureCreateStructName(GetNameFromClassDefinition(node)));
        } else {
            displayParts.emplace_back(CreateKeyword("class"));
            displayParts.emplace_back(CreateSpace());
            displayParts.emplace_back(CreateClassName(GetNameFromClassDefinition(node)));
        }
    }
    return displayParts;
}

std::string GetNameFromTSInterfaceDeclaration(ir::AstNode *node)
{
    if (node == nullptr || !node->IsTSInterfaceDeclaration()) {
        return "";
    }
    if (node->AsTSInterfaceDeclaration()->Id() == nullptr) {
        return "";
    }
    return std::string(node->AsTSInterfaceDeclaration()->Id()->AsIdentifier()->Name());
}

std::vector<SymbolDisplayPart> CreateDisplayForInterface(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Type() != ir::AstNodeType::TS_INTERFACE_DECLARATION) {
        return displayParts;
    }
    displayParts.emplace_back(CreateKeyword("interface"));
    displayParts.emplace_back(CreateSpace());
    displayParts.emplace_back(CreateClassName(GetNameFromTSInterfaceDeclaration(node)));
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForTypeAlias(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Type() != ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION) {
        return displayParts;
    }
    displayParts.emplace_back(CreateKeyword("type"));
    displayParts.emplace_back(CreateSpace());
    displayParts.emplace_back(CreateClassName(std::string(node->AsTSTypeAliasDeclaration()->Id()->Name())));
    displayParts.emplace_back(CreateSpace());
    displayParts.emplace_back(CreateOperator("="));
    displayParts.emplace_back(CreateSpace());

    auto typeAnnotation = node->AsTSTypeAliasDeclaration()->TypeAnnotation();
    auto type = GetNameForTypeNode(typeAnnotation);
    displayParts.emplace_back(CreateTypeName(type));
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForEnum(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Type() != ir::AstNodeType::TS_ENUM_DECLARATION) {
        return displayParts;
    }
    displayParts.emplace_back(CreateKeyword("enum"));
    displayParts.emplace_back(CreateSpace());
    displayParts.emplace_back(CreateEnumName(std::string(node->AsTSEnumDeclaration()->Key()->Name())));
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayOfFunctionParams(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Type() != ir::AstNodeType::SCRIPT_FUNCTION) {
        return displayParts;
    }
    displayParts.emplace_back(CreatePunctuation("("));
    auto functionParams = node->AsScriptFunction()->Params();
    bool first = true;
    for (auto param : functionParams) {
        if (!first) {
            displayParts.emplace_back(CreatePunctuation(","));
            displayParts.emplace_back(CreateSpace());
        }
        displayParts.emplace_back(CreateFunctionParameter(std::string(param->AsETSParameterExpression()->Name())));
        displayParts.emplace_back(CreatePunctuation(":"));
        displayParts.emplace_back(CreateSpace());
        displayParts.emplace_back(
            CreateTypeParameter(GetNameForTypeNode(param->AsETSParameterExpression()->TypeAnnotation())));
        first = false;
    }
    displayParts.emplace_back(CreatePunctuation(")"));
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayOfTypeParams(
    const ark::ArenaVector<ark::es2panda::ir::TSTypeParameter *> &typeParams)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (typeParams.empty()) {
        return displayParts;
    }
    displayParts.emplace_back(CreatePunctuation("<"));
    bool first = true;
    for (auto param : typeParams) {
        if (!first) {
            displayParts.emplace_back(CreatePunctuation(","));
            displayParts.emplace_back(CreateSpace());
        }
        displayParts.emplace_back(CreateTypeParameter(std::string(param->AsTSTypeParameter()->Name()->Name())));
        first = false;
    }
    displayParts.emplace_back(CreatePunctuation(">"));
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayOfReturnType(ark::es2panda::ir::TypeNode *returnType)
{
    std::vector<SymbolDisplayPart> displayParts;
    displayParts.emplace_back(CreatePunctuation(":"));
    displayParts.emplace_back(CreateSpace());
    if (returnType == nullptr) {
        displayParts.emplace_back(CreateReturnType("void"));
        return displayParts;
    }
    if (returnType->Type() == ir::AstNodeType::ETS_TYPE_REFERENCE) {
        auto part = returnType->AsETSTypeReference()->Part()->AsETSTypeReferencePart();
        auto nameNode = part->Name();
        if (nameNode->IsIdentifier()) {
            auto typeName = part->Name()->AsIdentifier()->Name();
            displayParts.emplace_back(CreateReturnType(std::string(typeName)));
        } else if (nameNode->IsTSQualifiedName()) {
            displayParts.emplace_back(CreateReturnType(std::string(nameNode->AsTSQualifiedName()->Name())));
        }
    }
    if (returnType->Type() == ir::AstNodeType::ETS_UNION_TYPE) {
        auto unionType = returnType->AsETSUnionType();
        auto types = unionType->Types();
        for (size_t i = 0; i < types.size(); ++i) {
            auto typeName = GetNameForTypeNode(types[i]);
            displayParts.emplace_back(CreateReturnType(typeName));
            if (i != types.size() - 1) {
                displayParts.emplace_back(CreatePunctuation("|"));
                displayParts.emplace_back(CreateSpace());
            }
        }
    }
    if (returnType->Type() == ir::AstNodeType::TS_THIS_TYPE) {
        displayParts.emplace_back(CreateReturnType("this"));
    }
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayOfTypeName(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    std::string typeName(node->AsTSTypeParameter()->Name()->Name());
    displayParts.emplace_back(CreateTypeParameter(typeName));
    displayParts.emplace_back(CreateSpace());
    displayParts.emplace_back(CreateKeyword("in"));
    displayParts.emplace_back(CreateSpace());
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForTypeParameterOfSciprtFunction(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    displayParts = CreateDisplayOfTypeName(node);

    auto grandParent = node->Parent()->Parent();
    if (grandParent->Type() == ir::AstNodeType::SCRIPT_FUNCTION) {
        auto parent = node->Parent();
        displayParts.emplace_back(CreateFunctionName(std::string(grandParent->AsScriptFunction()->Id()->Name())));

        auto typeParam = parent->AsTSTypeParameterDeclaration()->Params();
        auto displayOfTypeParams = CreateDisplayOfTypeParams(typeParam);
        displayParts = MergeSymbolDisplayPart(displayParts, displayOfTypeParams);

        auto displayOfFunctionParam = CreateDisplayOfFunctionParams(grandParent);
        displayParts = MergeSymbolDisplayPart(displayParts, displayOfFunctionParam);

        auto returnType = grandParent->AsScriptFunction()->ReturnTypeAnnotation();
        auto displayOfReturnType = CreateDisplayOfReturnType(returnType);
        displayParts = MergeSymbolDisplayPart(displayParts, displayOfReturnType);
    }
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForTypeParameterOfClassDefinition(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    displayParts = CreateDisplayOfTypeName(node);

    auto grandParent = node->Parent()->Parent();
    if (grandParent->Type() == ir::AstNodeType::CLASS_DEFINITION) {
        auto definition = grandParent->AsClassDefinition();
        displayParts.emplace_back(CreateClassName(std::string(definition->Ident()->Name())));

        auto typeParams = definition->TypeParams()->Params();
        auto displayOfTypeParams = CreateDisplayOfTypeParams(typeParams);
        displayParts = MergeSymbolDisplayPart(displayParts, displayOfTypeParams);
    }
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForTypeParameterOfTypeAliasDeclaration(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    displayParts = CreateDisplayOfTypeName(node);
    displayParts.emplace_back(CreateKeyword("type"));
    displayParts.emplace_back(CreateSpace());

    auto grandParent = node->Parent()->Parent();
    if (grandParent->Type() == ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION) {
        displayParts.emplace_back(CreateTypeName(std::string(grandParent->AsTSTypeAliasDeclaration()->Id()->Name())));
        auto typeParams = node->Parent()->AsTSTypeParameterDeclaration()->Params();
        auto displayOfTypeParams = CreateDisplayOfTypeParams(typeParams);
        displayParts = MergeSymbolDisplayPart(displayParts, displayOfTypeParams);
    }
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForTypeParameter(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Type() != ir::AstNodeType::TS_TYPE_PARAMETER) {
        return displayParts;
    }
    auto grandParent = node->Parent()->Parent();
    if (grandParent->Type() == ir::AstNodeType::SCRIPT_FUNCTION) {
        return CreateDisplayForTypeParameterOfSciprtFunction(node);
    }
    if (grandParent->Type() == ir::AstNodeType::CLASS_DEFINITION) {
        return CreateDisplayForTypeParameterOfClassDefinition(node);
    }
    if (grandParent->Type() == ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION) {
        return CreateDisplayForTypeParameterOfTypeAliasDeclaration(node);
    }

    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForEnumMember(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Type() != ir::AstNodeType::TS_ENUM_MEMBER) {
        return displayParts;
    }
    auto enumDecl = node->Parent()->AsTSEnumDeclaration()->Key()->Name();
    auto enumMember = node->AsTSEnumMember()->Key()->AsIdentifier()->Name();
    displayParts.emplace_back(CreateEnumName(std::string(enumDecl)));
    displayParts.emplace_back(CreatePunctuation("."));
    displayParts.emplace_back(CreateEnumMember(std::string(enumMember)));
    displayParts.emplace_back(CreateSpace());
    displayParts.emplace_back(CreateOperator("="));
    displayParts.emplace_back(CreateSpace());
    auto init = node->AsTSEnumMember()->Init();
    if (init->Type() == ir::AstNodeType::NUMBER_LITERAL) {
        displayParts.emplace_back(CreateText(std::to_string(init->AsNumberLiteral()->Number().GetInt())));
    }
    if (init->Type() == ir::AstNodeType::STRING_LITERAL) {
        displayParts.emplace_back(CreatePunctuation("\""));
        std::string str(init->AsStringLiteral()->Str());
        displayParts.emplace_back(CreateText(str));
        displayParts.emplace_back(CreatePunctuation("\""));
    }
    // Only number and string literal is supported now
    if (init->Type() == ir::AstNodeType::BIGINT_LITERAL) {
        displayParts.emplace_back(CreateText(std::string(init->AsBigIntLiteral()->Str())));
    }
    if (init->Type() == ir::AstNodeType::BOOLEAN_LITERAL) {
        displayParts.emplace_back(CreateText(init->AsBooleanLiteral()->Value() ? "true" : "false"));
    }
    if (init->Type() == ir::AstNodeType::CHAR_LITERAL) {
        char charLiteral = init->AsCharLiteral()->Char();
        std::string charToStr(1, charLiteral);
        displayParts.emplace_back(charToStr);
    }
    if (init->Type() == ir::AstNodeType::NULL_LITERAL) {
        displayParts.emplace_back(CreateText("null"));
    }
    if (init->Type() == ir::AstNodeType::UNDEFINED_LITERAL) {
        displayParts.emplace_back(CreateText("undefined"));
    }
    if (init->Type() == ir::AstNodeType::REGEXP_LITERAL) {
        displayParts.emplace_back(CreateText(std::string(init->AsRegExpLiteral()->Pattern())));
    }
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForMethodDefinitionOfConstructor(ir::AstNode *node,
                                                                             const std::string &kindModifier)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Parent()->Type() != ir::AstNodeType::CLASS_DEFINITION || kindModifier != "constructor") {
        return displayParts;
    }
    displayParts.emplace_back(CreateKeyword(kindModifier));
    displayParts.emplace_back(CreateSpace());
    auto classDefinition = node->Parent();
    auto className = classDefinition->AsClassDefinition()->Ident()->Name();
    displayParts.emplace_back(CreateClassName(std::string(className)));
    auto functionExpression = node->AsMethodDefinition()->Value()->AsFunctionExpression();
    if (functionExpression == nullptr) {
        return displayParts;
    }
    auto scriptFunction = functionExpression->Function();
    if (scriptFunction->Type() == ir::AstNodeType::SCRIPT_FUNCTION) {
        auto script = scriptFunction->AsScriptFunction();
        auto typeParameter = script->TypeParams();
        if (typeParameter != nullptr) {
            auto params = typeParameter->AsTSTypeParameterDeclaration()->Params();
            auto displayOfTypeParams = CreateDisplayOfTypeParams(params);
            displayParts = MergeSymbolDisplayPart(displayParts, displayOfTypeParams);
        }
        auto returnType = script->ReturnTypeAnnotation();
        auto displayOfReturnType = CreateDisplayOfReturnType(returnType);
        displayParts = MergeSymbolDisplayPart(displayParts, displayOfReturnType);
    }
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForMethodDefinitionOfGetterOrSetter(ir::AstNode *node,
                                                                                const std::string &kindModifier)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Parent()->Type() != ir::AstNodeType::CLASS_DEFINITION ||
        (kindModifier != "setter" && kindModifier != "getter")) {
        return displayParts;
    }
    displayParts.emplace_back(CreatePunctuation("("));
    displayParts.emplace_back(CreateKeyword(kindModifier));
    displayParts.emplace_back(CreatePunctuation(")"));
    displayParts.emplace_back(CreateSpace());
    auto classDefinition = node->Parent();
    auto className = classDefinition->AsClassDefinition()->Ident()->Name();
    displayParts.emplace_back(CreateClassName(std::string(className)));
    displayParts.emplace_back(CreatePunctuation("."));
    auto ident = node->AsMethodDefinition()->Key()->AsIdentifier();
    if (ident == nullptr) {
        return displayParts;
    }
    auto propertyName = ident->Name();
    displayParts.emplace_back(CreateProperty(std::string(propertyName)));
    displayParts.emplace_back(CreateSpace());

    auto functionExpression = node->AsMethodDefinition()->Value()->AsFunctionExpression();
    if (functionExpression == nullptr) {
        return displayParts;
    }
    auto scriptFunction = functionExpression->Function();
    if (scriptFunction->Type() == ir::AstNodeType::SCRIPT_FUNCTION) {
        auto script = scriptFunction->AsScriptFunction();
        auto returnType = script->ReturnTypeAnnotation();
        auto displayOfReturnType = CreateDisplayOfReturnType(returnType);
        displayParts = MergeSymbolDisplayPart(displayParts, displayOfReturnType);
    }

    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForMethodDefinitionOfInterfaceBody(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Parent()->Type() != ir::AstNodeType::TS_INTERFACE_BODY) {
        return displayParts;
    }
    auto interfaceDecl = node->Parent()->Parent();
    if (interfaceDecl != nullptr && interfaceDecl->Type() == ir::AstNodeType::TS_INTERFACE_DECLARATION) {
        auto interfaceName = interfaceDecl->AsTSInterfaceDeclaration()->Id()->Name();
        displayParts.emplace_back(CreateInterface(std::string(interfaceName)));
        displayParts.emplace_back(CreatePunctuation("."));
        auto propertyName = node->AsMethodDefinition()->Key()->AsIdentifier()->Name();
        displayParts.emplace_back(CreateProperty(std::string(propertyName)));
    }
    auto functionExpression = node->AsMethodDefinition()->Value()->AsFunctionExpression();
    if (functionExpression == nullptr) {
        return displayParts;
    }
    auto scriptFunction = functionExpression->Function();
    if (scriptFunction->Type() == ir::AstNodeType::SCRIPT_FUNCTION) {
        auto script = scriptFunction->AsScriptFunction();
        auto typeParameter = script->TypeParams();
        if (typeParameter != nullptr) {
            auto params = typeParameter->AsTSTypeParameterDeclaration()->Params();
            auto displayOfTypeParams = CreateDisplayOfTypeParams(params);
            displayParts = MergeSymbolDisplayPart(displayParts, displayOfTypeParams);
        }
        auto returnType = script->ReturnTypeAnnotation();
        auto displayOfReturnType = CreateDisplayOfReturnType(returnType);
        displayParts = MergeSymbolDisplayPart(displayParts, displayOfReturnType);
    }
    return displayParts;
}

void AppendClassOrGlobalPrefix(std::vector<SymbolDisplayPart> &parts, ir::AstNode *parent)
{
    if (!parent->IsClassDefinition()) {
        return;
    }

    auto className = parent->AsClassDefinition()->Ident()->Name();
    if (className != "ETSGLOBAL") {
        parts.emplace_back(CreateClassName(std::string(className)));
        parts.emplace_back(CreatePunctuation("."));
    } else {
        parts.emplace_back(CreateKeyword("function"));
        parts.emplace_back(CreateSpace());
    }
}

void AppendFunctionName(std::vector<SymbolDisplayPart> &parts, ir::MethodDefinition *method)
{
    auto name = method->Key()->AsIdentifier()->Name();
    parts.emplace_back(CreateFunctionName(std::string(name)));
}

static void AppendSignature(std::vector<SymbolDisplayPart> &parts, ir::MethodDefinition *method,
                            checker::ETSChecker *checker)
{
    auto *funcExpr = method->Value();
    if (funcExpr == nullptr) {
        return;
    }
    auto *script = funcExpr->AsFunctionExpression()->Function();
    if (script == nullptr || script->Type() != ir::AstNodeType::SCRIPT_FUNCTION) {
        return;
    }
    // <T, U>
    if (auto *typeParams = script->TypeParams()) {
        auto display = CreateDisplayOfTypeParams(typeParams->AsTSTypeParameterDeclaration()->Params());
        parts = MergeSymbolDisplayPart(parts, display);
    }
    // (a: number, b: string)
    auto paramDisplay = CreateDisplayOfFunctionParams(script);
    parts = MergeSymbolDisplayPart(parts, paramDisplay);
    // return type
    auto returnType = script->ReturnTypeAnnotation();
    if (returnType == nullptr) {
        auto signature = GetFuncSignature(method->TsType()->AsETSFunctionType(), method);
        auto typeStr = GetReturnTypeStr(signature->ReturnType(), checker);
        parts.emplace_back(CreatePunctuation(":"));
        parts.emplace_back(CreateSpace());
        parts.emplace_back(CreateReturnType(typeStr));
    } else {
        auto retDisplay = CreateDisplayOfReturnType(script->ReturnTypeAnnotation());
        parts = MergeSymbolDisplayPart(parts, retDisplay);
    }
}

static std::vector<SymbolDisplayPart> CreateDisplayForRegularOrClassMethod(ir::AstNode *node,
                                                                           checker::ETSChecker *checker)
{
    std::vector<SymbolDisplayPart> parts;
    AppendClassOrGlobalPrefix(parts, node->Parent());
    AppendFunctionName(parts, node->AsMethodDefinition());
    AppendSignature(parts, node->AsMethodDefinition(), checker);
    return parts;
}

std::vector<SymbolDisplayPart> CreateDisplayForMethodDefinition(ir::AstNode *node, const std::string &kindModifier,
                                                                checker::ETSChecker *checker)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Type() != ir::AstNodeType::METHOD_DEFINITION) {
        return displayParts;
    }
    auto parent = node->Parent();
    if (parent == nullptr) {
        return CreateDisplayForRegularOrClassMethod(node, checker);
    }
    if (kindModifier == "constructor" && parent->Type() == ir::AstNodeType::CLASS_DEFINITION) {
        return CreateDisplayForMethodDefinitionOfConstructor(node, kindModifier);
    }
    if ((kindModifier == "getter" || kindModifier == "setter") && parent->Type() == ir::AstNodeType::CLASS_DEFINITION) {
        return CreateDisplayForMethodDefinitionOfGetterOrSetter(node, kindModifier);
    }
    if (parent->Type() == ir::AstNodeType::TS_INTERFACE_BODY) {
        return CreateDisplayForMethodDefinitionOfInterfaceBody(node);
    }

    return CreateDisplayForRegularOrClassMethod(node, checker);
}

std::vector<SymbolDisplayPart> CreateDisplayForClassProperty(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Type() != ir::AstNodeType::CLASS_PROPERTY) {
        return displayParts;
    }
    auto classDef = node->Parent();
    if (classDef->Type() == ir::AstNodeType::CLASS_DEFINITION) {
        auto className = classDef->AsClassDefinition()->Ident()->Name();
        auto isConst = (node->Modifiers() & ir::ModifierFlags::CONST) != 0;
        if (className != "ETSGLOBAL") {
            displayParts.emplace_back(CreateClassName(std::string(className)));
            displayParts.emplace_back(CreatePunctuation("."));
        } else if (isConst) {
            displayParts.emplace_back(CreateKeyword("const"));
            displayParts.emplace_back(CreateSpace());
        } else {
            displayParts.emplace_back(CreateKeyword("let"));
            displayParts.emplace_back(CreateSpace());
        }
        auto propertyName = node->AsClassProperty()->Key()->AsIdentifier()->Name();
        displayParts.emplace_back(CreateProperty(std::string(propertyName)));
        displayParts.emplace_back(CreatePunctuation(":"));
        displayParts.emplace_back(CreateSpace());

        auto typeAnnotation = node->AsClassProperty()->TypeAnnotation();
        std::string type;
        if (typeAnnotation == nullptr) {
            if (node->AsClassProperty()->Value() == nullptr ||
                !node->AsClassProperty()->Value()->IsETSNewClassInstanceExpression()) {
                displayParts.emplace_back(
                    CreateTypeName(GetNameForLiteralTypeNode(node->AsClassProperty()->Value(), isConst)));
                return displayParts;
            }
            auto newClassExpr = node->AsClassProperty()->Value()->AsETSNewClassInstanceExpression();
            if (newClassExpr != nullptr) {
                type = std::string(newClassExpr->GetTypeRef()->AsETSTypeReference()->Part()->GetIdent()->Name());
            }
        } else {
            type = GetNameForTypeNode(typeAnnotation);
        }
        displayParts.emplace_back(CreateTypeName(type));
    }
    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForETSParameterExpression(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Type() != ir::AstNodeType::ETS_PARAMETER_EXPRESSION) {
        return displayParts;
    }
    auto paramName = node->AsETSParameterExpression()->Name();
    displayParts.emplace_back(CreateFunctionParameter(std::string(paramName)));
    displayParts.emplace_back(CreatePunctuation(":"));
    displayParts.emplace_back(CreateSpace());

    auto typeAnnotation = node->AsETSParameterExpression()->TypeAnnotation();
    if (typeAnnotation == nullptr) {
        return displayParts;
    }
    auto type = GetNameForTypeNode(typeAnnotation);
    displayParts.emplace_back(CreateTypeName(type));

    return displayParts;
}

std::vector<SymbolDisplayPart> CreateDisplayForImportDeclaration(ir::AstNode *node)
{
    std::vector<SymbolDisplayPart> displayParts;
    if (node->Type() != ir::AstNodeType::IMPORT_DECLARATION) {
        return displayParts;
    }
    displayParts.emplace_back(CreateKeyword("namespace"));
    displayParts.emplace_back(CreateSpace());
    auto specifiers = node->AsImportDeclaration()->Specifiers();
    bool first = true;
    for (auto spec : specifiers) {
        if (!first) {
            displayParts.emplace_back(CreatePunctuation(","));
            displayParts.emplace_back(CreateSpace());
        }
        if (spec->Type() == ir::AstNodeType::IMPORT_DEFAULT_SPECIFIER ||
            spec->Type() == ir::AstNodeType::IMPORT_NAMESPACE_SPECIFIER ||
            spec->Type() == ir::AstNodeType::IMPORT_SPECIFIER) {
            displayParts.emplace_back(CreateNamespace(std::string(spec->AsImportDefaultSpecifier()->Local()->Name())));
        }
        first = false;
    }

    return displayParts;
}

QuickInfo GetQuickInfo(ir::AstNode *node, ir::AstNode *containerNode, ir::AstNode *nodeForQuickInfo,
                       const std::string &fileName, checker::ETSChecker *checker)
{
    if (containerNode == nullptr || nodeForQuickInfo == nullptr || node == nullptr) {
        return QuickInfo();
    }
    auto kindModifiers = GetKindModifiers(node);
    TextSpan span(nodeForQuickInfo->Start().index, nodeForQuickInfo->End().index - nodeForQuickInfo->Start().index);
    std::vector<SymbolDisplayPart> displayParts;

    std::string kind;
    std::vector<SymbolDisplayPart> document;
    std::vector<DocTagInfo> tags;

    if (IsClass(node)) {
        displayParts = CreateDisplayForClass(node);
    } else if (node->Type() == ir::AstNodeType::ETS_PARAMETER_EXPRESSION) {
        displayParts = CreateDisplayForETSParameterExpression(node);
    } else if (node->Type() == ir::AstNodeType::CLASS_PROPERTY) {
        // After enum refactoring, enum declaration is transformed to a class declaration
        if (compiler::ClassDefinitionIsEnumTransformed(node->Parent())) {
            auto enumDecl = node->Parent()->AsClassDefinition()->OrigEnumDecl()->AsTSEnumDeclaration();
            auto enumMember = GetEnumMemberByName(enumDecl, node->AsClassProperty()->Key()->AsIdentifier()->Name());
            if (enumMember != nullptr) {
                displayParts = CreateDisplayForEnumMember(enumMember);
            }
        } else {
            displayParts = CreateDisplayForClassProperty(node);
        }
    } else if (node->Type() == ir::AstNodeType::TS_ENUM_MEMBER) {
        displayParts = CreateDisplayForEnumMember(node);
    } else if (node->Type() == ir::AstNodeType::TS_INTERFACE_DECLARATION) {
        displayParts = CreateDisplayForInterface(node);
    } else if (node->Type() == ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION) {
        displayParts = CreateDisplayForTypeAlias(node);
    } else if (node->Type() == ir::AstNodeType::TS_ENUM_DECLARATION) {
        displayParts = CreateDisplayForEnum(node);
    } else if (node->Type() == ir::AstNodeType::IMPORT_DECLARATION) {
        displayParts = CreateDisplayForImportDeclaration(node);
    } else if (node->Type() == ir::AstNodeType::TS_TYPE_PARAMETER) {
        displayParts = CreateDisplayForTypeParameter(node);
    } else if (node->Type() == ir::AstNodeType::METHOD_DEFINITION) {
        displayParts = CreateDisplayForMethodDefinition(node, kindModifiers, checker);
    }
    // Unify this kind
    kind = GetNodeKindForRenameInfo(node);
    return QuickInfo(kind, kindModifiers, span, displayParts, document, tags, fileName);
}

QuickInfo GetQuickInfoAtPositionImpl(es2panda_Context *context, size_t position, std::string fileName)
{
    if (context == nullptr) {
        return QuickInfo();
    }
    auto touchingToken = GetTouchingToken(context, position, false);
    if (touchingToken == nullptr || touchingToken->IsProgram()) {
        return QuickInfo();
    }
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    // The nodeForQuickInfo will be identifier in all of currently known scenarios
    auto nodeForQuickInfo = GetTokenForQuickInfo(context, position);
    auto node = GetNodeAtLocationForQuickInfo(nodeForQuickInfo);
    auto object = GetContainingObjectLiteralNode(nodeForQuickInfo);
    auto nodeFileName = std::move(fileName);
    if (object != nullptr) {
        auto contextualTypeNode = GetContextualTypeNode(GetContainingObjectLiteralNode(nodeForQuickInfo)->Parent());
        if (contextualTypeNode != nullptr && contextualTypeNode->IsETSImportDeclaration()) {
            nodeFileName = GetNodeFileName(contextualTypeNode);
        }
    }
    if (node == nullptr) {
        return QuickInfo();
    }

    return GetQuickInfo(node, GetContainerNode(nodeForQuickInfo), nodeForQuickInfo, nodeFileName, checker);
}

}  // namespace ark::es2panda::lsp