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

#include "ETSparser.h"
#include "generated/diagnostic.h"
#include "lexer/lexer.h"
#include "ir/ets/etsTuple.h"
#include "ir/ets/etsUnionType.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/brokenTypeNode.h"

namespace ark::es2panda::parser {

ir::Statement *ETSParser::ParseTopLevelAnnotation(ir::ModifierFlags memberModifiers)
{
    ir::Statement *result = nullptr;

    Lexer()->NextToken();  // eat '@'
    if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_INTERFACE) {
        result = ParseAnnotationDeclaration(memberModifiers);
    } else {
        auto annotations = ParseAnnotations(true);
        auto savePos = Lexer()->GetToken().Start();
        result = ParseTopLevelStatement();
        if (result != nullptr) {
            ApplyAnnotationsToNode(result, std::move(annotations), savePos);
        } else {
            LogError(diagnostic::ANNOTATION_NOT_ALLOWED);
        }
    }

    return result;
}

template <bool IS_USAGE>
ir::Expression *ETSParser::ParseAnnotationName()
{
    ir::Expression *expr = nullptr;

    auto setAnnotation = [](ir::Identifier *ident) {
        if constexpr (IS_USAGE) {
            ident->SetAnnotationUsage();
        } else {
            ident->SetAnnotationDecl();
        }
    };
    auto save = Lexer()->Save();
    ir::Identifier *ident = nullptr;
    Lexer()->NextToken();
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_PERIOD_PERIOD_PERIOD) {
        Lexer()->Rewind(save);
        expr = ExpectIdentifier();
        ES2PANDA_ASSERT(expr != nullptr);
        ident = expr->AsIdentifier();
    } else {
        Lexer()->Rewind(save);
        if (IS_USAGE && Lexer()->Lookahead() == '.') {
            auto opt = TypeAnnotationParsingOptions::NO_OPTS;
            expr = ParseTypeReference(&opt);
            ES2PANDA_ASSERT(expr != nullptr);
            ident = expr->AsETSTypeReference()->Part()->GetIdent();
        } else {
            expr = ExpectIdentifier();
            ES2PANDA_ASSERT(expr != nullptr);
            ident = expr->AsIdentifier();
        }
    }

    if (ident->IsBrokenExpression()) {
        LogError(diagnostic::INVALID_ANNOTATION_NAME, {}, expr->Start());
    }

    setAnnotation(ident);
    return expr;
}

ir::AnnotationDeclaration *ETSParser::ParseAnnotationDeclaration(ir::ModifierFlags flags)
{
    const lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    // The default modifier of the annotation is public abstract
    flags |= ir::ModifierFlags::ABSTRACT | ir::ModifierFlags::PUBLIC | ir::ModifierFlags::ANNOTATION_DECLARATION;
    flags &= ~ir::ModifierFlags::STATIC;
    if (InAmbientContext()) {
        flags |= ir::ModifierFlags::DECLARE;
    }
    Lexer()->NextToken();
    ir::Expression *expr = ParseAnnotationName<false>();

    ExpectToken(lexer::TokenType::PUNCTUATOR_LEFT_BRACE, false);
    auto properties = ParseAnnotationProperties(flags);

    lexer::SourcePosition endLoc = Lexer()->GetToken().End();

    auto *annotationDecl = AllocNode<ir::AnnotationDeclaration>(expr, std::move(properties), Allocator());
    ES2PANDA_ASSERT(annotationDecl != nullptr);
    annotationDecl->SetRange({startLoc, endLoc});
    annotationDecl->AddModifier(flags);
    return annotationDecl;
}

static bool IsMemberAccessModifiers(lexer::TokenType type)
{
    return type == lexer::TokenType::KEYW_STATIC || type == lexer::TokenType::KEYW_ASYNC ||
           type == lexer::TokenType::KEYW_PUBLIC || type == lexer::TokenType::KEYW_PROTECTED ||
           type == lexer::TokenType::KEYW_PRIVATE || type == lexer::TokenType::KEYW_DECLARE ||
           type == lexer::TokenType::KEYW_READONLY || type == lexer::TokenType::KEYW_ABSTRACT ||
           type == lexer::TokenType::KEYW_CONST || type == lexer::TokenType::KEYW_FINAL ||
           type == lexer::TokenType::KEYW_NATIVE;
}

ArenaVector<ir::AstNode *> ETSParser::ParseAnnotationProperties(ir::ModifierFlags memberModifiers)
{
    Lexer()->NextToken(lexer::NextTokenFlags::KEYWORD_TO_IDENT);
    ArenaVector<ir::AstNode *> properties(Allocator()->Adapter());

    while (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE &&
           Lexer()->GetToken().Type() != lexer::TokenType::EOS) {
        if ((memberModifiers & ir::ModifierFlags::ANNOTATION_DECLARATION) != 0U &&
            Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SEMI_COLON) {
            Lexer()->NextToken();  // eat ';'
            continue;
        }
        // check no modifiers
        while (IsMemberAccessModifiers(Lexer()->GetToken().Type())) {
            LogError(diagnostic::ANNOTATION_PROPERTY_ACCESS_MODIFIERS, {}, Lexer()->GetToken().Start());
            Lexer()->NextToken();
        }

        auto *fieldName = ExpectIdentifier();
        ES2PANDA_ASSERT(fieldName != nullptr);
        if (fieldName->IsErrorPlaceHolder()) {
            //  Try to recover from error: simplest strategy: only one step ahead.
            //  Probably we can seek for identifier till the enclosing right brace (staring after the next comma?)
            //  if the simplest case failed.
            auto const pos = Lexer()->Save();
            auto *fieldName1 = ExpectIdentifier();
            ES2PANDA_ASSERT(fieldName1 != nullptr);
            if (fieldName1->IsErrorPlaceHolder()) {
                Lexer()->Rewind(pos);
            } else {
                fieldName = fieldName1;
            }
        }

        bool needTypeAnnotation = (memberModifiers & ir::ModifierFlags::ANNOTATION_USAGE) == 0U;
        properties.emplace_back(ParseAnnotationProperty(fieldName, memberModifiers, needTypeAnnotation));

        if ((memberModifiers & ir::ModifierFlags::ANNOTATION_USAGE) != 0U &&
            Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_BRACE) {
            ExpectToken(lexer::TokenType::PUNCTUATOR_COMMA);  // eat ','
        }
    }

    Lexer()->NextToken();  // eat "}"
    return properties;
}

bool ETSParser::ValidAnnotationValue(ir::Expression *initializer)
{
    if (initializer->IsArrayExpression()) {
        for (auto *element : initializer->AsArrayExpression()->Elements()) {
            if (!ValidAnnotationValue(element)) {
                return false;
            }
        }
        return true;
    }
    return initializer->IsStringLiteral() || initializer->IsNumberLiteral() || initializer->IsMemberExpression() ||
           initializer->IsBooleanLiteral() || initializer->IsBinaryExpression() || initializer->IsUnaryExpression() ||
           initializer->IsConditionalExpression() || initializer->IsIdentifier() || initializer->IsTSAsExpression();
}

ir::AstNode *ETSParser::ParseAnnotationProperty(ir::Identifier *fieldName, ir::ModifierFlags memberModifiers,
                                                bool needTypeAnnotation)
{
    lexer::SourcePosition endLoc = fieldName->End();
    // check no methods
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS ||
        Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LESS_THAN) {
        LogError(diagnostic::ANNOTATION_METHOD_AS_PROP, {}, Lexer()->GetToken().Start());
    }

    ir::TypeNode *typeAnnotation = nullptr;
    TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::REPORT_ERROR;
    if (needTypeAnnotation && Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON) {
        Lexer()->NextToken();  // eat ':'
        typeAnnotation = ParseTypeAnnotation(&options);
    }

    if (typeAnnotation == nullptr && (memberModifiers & ir::ModifierFlags::ANNOTATION_DECLARATION) != 0 &&
        !fieldName->IsErrorPlaceHolder()) {
        LogError(diagnostic::MISSING_TYPE_ANNOTATION, {fieldName->Name().Mutf8()}, Lexer()->GetToken().Start());
        typeAnnotation = AllocNode<ir::BrokenTypeNode>(Allocator());
        typeAnnotation->SetRange({endLoc, endLoc});
    }

    if (typeAnnotation != nullptr) {
        endLoc = typeAnnotation->End();
    }

    ir::Expression *initializer = nullptr;
    lexer::SourcePosition savePos;
    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION ||
        (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_COLON)) {
        Lexer()->NextToken();  // eat '=' or ':'
        savePos = Lexer()->GetToken().Start();
        initializer = ParseExpression();
    }

    if (initializer == nullptr && (memberModifiers & ir::ModifierFlags::ANNOTATION_USAGE) != 0 &&
        !fieldName->IsErrorPlaceHolder()) {
        LogError(diagnostic::INVALID_ARGUMENT_PASSED, {fieldName->Name().Mutf8()}, Lexer()->GetToken().Start());
    }

    if (initializer != nullptr && !ValidAnnotationValue(initializer)) {
        LogError(diagnostic::INVALID_VAL_ANNOTATION_FIELD, {}, savePos);
        initializer = AllocBrokenExpression(savePos);
    }

    memberModifiers |= ir::ModifierFlags::PUBLIC;
    memberModifiers |= ir::ModifierFlags::ABSTRACT;
    auto *field =
        AllocNode<ir::ClassProperty>(fieldName, initializer, typeAnnotation, memberModifiers, Allocator(), false);
    ES2PANDA_ASSERT(field != nullptr);
    field->SetRange({fieldName->Start(), initializer != nullptr ? initializer->End() : endLoc});
    return field;
}

ArenaVector<ir::AnnotationUsage *> ETSParser::ParseAnnotations(bool isTopLevelSt)
{
    ArenaVector<ir::AnnotationUsage *> annotations(Allocator()->Adapter());
    bool hasMoreAnnotations = true;
    auto save = Lexer()->Save();
    while (hasMoreAnnotations) {
        if (Lexer()->GetToken().Type() == lexer::TokenType::KEYW_INTERFACE) {
            if (!isTopLevelSt) {
                LogError(diagnostic::ANNOTATION_ONLY_TOP_LEVEL);
            }
            Lexer()->Rewind(save);
            return annotations;
        }

        annotations.emplace_back(ParseAnnotationUsage());
        if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_AT) {
            hasMoreAnnotations = false;
        } else {
            save = Lexer()->Save();
            Lexer()->NextToken();
        }
    }
    return annotations;
}

static bool ApplyAnnotationsToNamespace(ir::ETSModule *ns, ArenaVector<ir::AnnotationUsage *> &annotations)
{
    if (ns->IsNamespaceChainLastNode()) {
        ns->SetAnnotations(std::move(annotations));
        return true;
    }

    for (auto *node : ns->Statements()) {
        if (node->IsETSModule()) {
            if (ApplyAnnotationsToNamespace(node->AsETSModule(), annotations)) {
                return true;
            }
        }
    }
    return false;
}

void ETSParser::ApplyAnnotationsToNode(ir::AstNode *node, ArenaVector<ir::AnnotationUsage *> &&annotations,
                                       lexer::SourcePosition pos, TypeAnnotationParsingOptions options)
{
    if (node == nullptr || annotations.empty()) {
        return;
    }

    if ((options & TypeAnnotationParsingOptions::ANNOTATION_NOT_ALLOW) != 0) {
        LogError(diagnostic::ANNOTATION_WRONG_DEC, {}, pos);
        return;
    }

    if (node->IsExpressionStatement()) {
        ApplyAnnotationsToNode(node->AsExpressionStatement()->GetExpression(), std::move(annotations), pos);
        return;
    }

    ApplyAnnotationsToSpecificNodeType(node, std::move(annotations), pos);
}

// CC-OFFNXT(huge_method,huge_cyclomatic_complexity,G.FUN.01-CPP) big switch-case, solid logic
void ETSParser::ApplyAnnotationsToSpecificNodeType(ir::AstNode *node, ArenaVector<ir::AnnotationUsage *> &&annotations,
                                                   lexer::SourcePosition pos)
{
    switch (node->Type()) {
        case ir::AstNodeType::METHOD_DEFINITION: {
            auto *func = node->AsMethodDefinition()->Function();
            ES2PANDA_ASSERT(func != nullptr);
            func->SetAnnotations(std::move(annotations));
            break;
        }
        case ir::AstNodeType::CLASS_DECLARATION:
            node->AsClassDeclaration()->Definition()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::STRUCT_DECLARATION:
            node->AsETSStructDeclaration()->Definition()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::FUNCTION_DECLARATION:
            node->AsFunctionDeclaration()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
            node->AsTSInterfaceDeclaration()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::CLASS_PROPERTY:
            node->AsClassProperty()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::VARIABLE_DECLARATION:
            node->AsVariableDeclaration()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION:
            node->AsTSTypeAliasDeclaration()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::ETS_PARAMETER_EXPRESSION:
            node->AsETSParameterExpression()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
            node->AsArrowFunctionExpression()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::ETS_MODULE:
            ApplyAnnotationsToNamespace(node->AsETSModule(), annotations);
            break;
        case ir::AstNodeType::ETS_TYPE_REFERENCE:
            node->AsETSTypeReference()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::TS_ARRAY_TYPE:
            node->AsTSArrayType()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::ETS_TUPLE:
            node->AsETSTuple()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::ETS_UNION_TYPE:
            node->AsETSUnionType()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::ETS_FUNCTION_TYPE:
            node->AsETSFunctionType()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::ETS_STRING_LITERAL_TYPE:
            node->AsETSStringLiteralType()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::ETS_NULL_TYPE:
            node->AsETSNullType()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::ETS_UNDEFINED_TYPE:
            node->AsETSUndefinedType()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::TS_TYPE_PARAMETER:
            node->AsTSTypeParameter()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::ANNOTATION_DECLARATION:
            node->AsAnnotationDeclaration()->SetAnnotations(std::move(annotations));
            break;
        case ir::AstNodeType::TS_ENUM_DECLARATION:
            node->AsTSEnumDeclaration()->SetAnnotations(std::move(annotations));
            break;
        default:
            LogError(diagnostic::ANNOTATION_WRONG_DEC, {}, pos);
    }
}

static lexer::SourcePosition GetAnnotationExpressionEndLoc(ir::Expression *expr)
{
    ES2PANDA_ASSERT(expr != nullptr);
    if (expr->IsIdentifier()) {
        return expr->AsIdentifier()->End();
    }

    return expr->AsETSTypeReference()->Part()->GetIdent()->End();
}

ir::AnnotationUsage *ETSParser::ParseAnnotationUsage()
{
    const lexer::SourcePosition startLoc = Lexer()->GetToken().Start();
    ir::Expression *expr = ParseAnnotationName<true>();

    auto flags = ir::ModifierFlags::ANNOTATION_USAGE;
    ArenaVector<ir::AstNode *> properties(Allocator()->Adapter());

    if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS &&
        !IsArrowFunctionExpressionStart() && IsAnnotationUsageStart(GetAnnotationExpressionEndLoc(expr))) {
        Lexer()->NextToken();  // eat '('
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_BRACE) {
            properties = ParseAnnotationProperties(flags);
        } else if (Lexer()->GetToken().Type() != lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
            // handle single field annotation
            auto *singleParamName = AllocNode<ir::Identifier>(compiler::Signatures::ANNOTATION_KEY_VALUE, Allocator());
            ES2PANDA_ASSERT(singleParamName != nullptr);
            singleParamName->SetRange({Lexer()->GetToken().Start(), Lexer()->GetToken().End()});

            const auto savePos = Lexer()->GetToken().Start();
            auto *initializer = ParseExpression();
            if (initializer != nullptr && !ValidAnnotationValue(initializer)) {
                LogError(diagnostic::INVALID_VAL_ANNOTATION_FIELD, {}, savePos);
            }

            auto *singleParam = AllocNode<ir::ClassProperty>(singleParamName, initializer, nullptr,
                                                             ir::ModifierFlags::ANNOTATION_USAGE, Allocator(), false);
            ES2PANDA_ASSERT(singleParam != nullptr);
            singleParam->SetRange(
                {singleParamName->Start(), initializer != nullptr ? initializer->End() : singleParamName->End()});
            properties.push_back(singleParam);
        }

        auto *annotationUsage = AllocNode<ir::AnnotationUsage>(expr, std::move(properties));
        annotationUsage->AddModifier(flags);
        annotationUsage->SetRange({startLoc, Lexer()->GetToken().End()});
        annotationUsage->SetHasParameterParen(true);
        ExpectToken(lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS, true);  // eat ')'

        return annotationUsage;
    }

    auto *annotationUsage = AllocNode<ir::AnnotationUsage>(expr, std::move(properties));
    ES2PANDA_ASSERT(annotationUsage != nullptr);
    annotationUsage->AddModifier(flags);
    annotationUsage->SetRange({startLoc, GetAnnotationExpressionEndLoc(expr)});

    return annotationUsage;
}

bool ETSParser::TryParseAnnotations()
{
    std::vector<ir::AnnotationUsage *> annotations {};

    while (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_AT) {
        Lexer()->NextToken();

        ir::Expression *ident = ParseAnnotationName<true>();
        ArenaVector<ir::AstNode *> properties(Allocator()->Adapter());

        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
            TryParseAnnotationsParams();
        }

        auto *annotation = AllocNode<ir::AnnotationUsage>(ident, std::move(properties));
        annotations.push_back(annotation);
    }

    return !annotations.empty();
}

void ETSParser::TryParseAnnotationsParams()
{
    Lexer()->NextToken();
    int parenthesesCount = 1;

    while (parenthesesCount > 0) {
        if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_LEFT_PARENTHESIS) {
            parenthesesCount++;
        } else if (Lexer()->GetToken().Type() == lexer::TokenType::PUNCTUATOR_RIGHT_PARENTHESIS) {
            parenthesesCount--;
        }
        Lexer()->NextToken();
    }
}

bool ETSParser::IsAnnotationUsageStart(lexer::SourcePosition lastTokenEndIndex)
{
    auto result = true;
    if (Lexer()->GetToken().End().index - lastTokenEndIndex.index != 1) {
        result = false;
    }
    return result;
}

}  // namespace ark::es2panda::parser
