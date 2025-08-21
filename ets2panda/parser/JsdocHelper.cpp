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

#include "JsdocHelper.h"
#include <ir/ets/etsModule.h>
#include "lexer/lexer.h"
#include "ir/ets/etsTuple.h"
#include "ir/statements/annotationDeclaration.h"

namespace ark::es2panda::parser {
static constexpr std::string_view JSDOC_END = "*/";
static constexpr std::string_view EMPTY_JSDOC = "Empty Jsdoc";

static constexpr size_t START_POS = 0;
static constexpr size_t COLLECT_CURRENT_POS = 1;

// NOLINTBEGIN(modernize-avoid-c-arrays)
static constexpr std::string_view POTENTIAL_PREFIX[] = {
    "@",        "get",    "set",    "let",    "const",   "overload", "async",   "readonly",
    "abstract", "native", "static", "public", "private", "declare",  "default", "export"};
// NOLINTEND(modernize-avoid-c-arrays)

// Note: Potential annotation allowed node need to collect jsdoc.
// NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
static const std::unordered_set<ir::AstNodeType> ANNOTATION_ALLOWED_NODE = {
    ir::AstNodeType::METHOD_DEFINITION,         ir::AstNodeType::CLASS_DECLARATION,
    ir::AstNodeType::STRUCT_DECLARATION,        ir::AstNodeType::FUNCTION_DECLARATION,
    ir::AstNodeType::TS_INTERFACE_DECLARATION,  ir::AstNodeType::CLASS_PROPERTY,
    ir::AstNodeType::VARIABLE_DECLARATION,      ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION,
    ir::AstNodeType::ARROW_FUNCTION_EXPRESSION, ir::AstNodeType::ANNOTATION_DECLARATION};
// NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)

static const ArenaVector<ir::AnnotationUsage *> &GetAstAnnotationUsage(const ir::AstNode *node)
{
    switch (node->Type()) {
        case ir::AstNodeType::METHOD_DEFINITION: {
            auto *func = node->AsMethodDefinition()->Function();
            ES2PANDA_ASSERT(func != nullptr);
            return func->Annotations();
        }
        case ir::AstNodeType::CLASS_DECLARATION:
            return node->AsClassDeclaration()->Definition()->Annotations();
        case ir::AstNodeType::FUNCTION_DECLARATION:
            return node->AsFunctionDeclaration()->Annotations();
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
            return node->AsTSInterfaceDeclaration()->Annotations();
        case ir::AstNodeType::CLASS_PROPERTY:
            return node->AsClassProperty()->Annotations();
        case ir::AstNodeType::VARIABLE_DECLARATION:
            return node->AsVariableDeclaration()->Annotations();
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION:
            return node->AsTSTypeAliasDeclaration()->Annotations();
        case ir::AstNodeType::ETS_PARAMETER_EXPRESSION:
            return node->AsETSParameterExpression()->Annotations();
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
            return node->AsArrowFunctionExpression()->Annotations();
        case ir::AstNodeType::ANNOTATION_DECLARATION:
            return node->AsAnnotationDeclaration()->Annotations();
        case ir::AstNodeType::STRUCT_DECLARATION:
            return node->AsETSStructDeclaration()->Definition()->Annotations();
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static void HandlePotentialPrefix(parser::JsdocHelper *jsdocHelper)
{
    jsdocHelper->Iterator().Reset(jsdocHelper->Node()->Start().index);
    jsdocHelper->BackwardAndSkipSpace(1);
    for (auto prefix : POTENTIAL_PREFIX) {
        auto currentSv = jsdocHelper->SourceView(START_POS, jsdocHelper->Iterator().Index() + COLLECT_CURRENT_POS);
        if (currentSv.EndsWith(prefix)) {
            jsdocHelper->BackwardAndSkipSpace(prefix.length());
        }
    }
}

static void HandlePotentialPrefixOrAnnotationUsage(parser::JsdocHelper *jsdocHelper)
{
    if (ANNOTATION_ALLOWED_NODE.count(jsdocHelper->Node()->Type()) == 0) {
        HandlePotentialPrefix(jsdocHelper);
        return;
    }

    auto const &annoUsage = GetAstAnnotationUsage(jsdocHelper->Node());
    if (annoUsage.empty()) {
        HandlePotentialPrefix(jsdocHelper);
        return;
    }

    // Note: eat current iter.
    jsdocHelper->Iterator().Reset(annoUsage[0]->Range().start.index - 1);
    if (jsdocHelper->Iterator().Index() != START_POS) {
        // Note: eat token `@`
        jsdocHelper->BackwardAndSkipSpace(1);
    }
}

bool JsdocHelper::BackWardUntilJsdocStart()
{
    while (true) {
        const char32_t cp = Iterator().Index() == START_POS ? util::StringView::Iterator::INVALID_CP : PeekBackWard();
        switch (cp) {
            case util::StringView::Iterator::INVALID_CP: {
                break;
            }
            case lexer::LEX_CHAR_ASTERISK: {
                Backward(1);
                if (PeekBackWard() == lexer::LEX_CHAR_SLASH) {
                    // Note: found `/*` here, it is only the common start of comments, not jsdoc.
                    return false;
                }
                if (PeekBackWard() != lexer::LEX_CHAR_ASTERISK) {
                    continue;
                }

                if (Iterator().Index() == START_POS) {
                    break;
                }

                Backward(1);
                if (PeekBackWard() == lexer::LEX_CHAR_SLASH) {
                    return true;
                }
                continue;
            }
            default: {
                SkipCpBackward();
                continue;
            }
        }
        return false;
    }
}

util::StringView JsdocHelper::GetJsdocBackward()
{
    HandlePotentialPrefixOrAnnotationUsage(this);
    size_t jsdocEndPos = Iterator().Index() + COLLECT_CURRENT_POS;
    size_t backwardPos = jsdocEndPos;
    auto currentSourceView = SourceView(START_POS, jsdocEndPos);
    while (currentSourceView.EndsWith(JSDOC_END)) {
        BackwardAndSkipSpace(JSDOC_END.length());
        if (!BackWardUntilJsdocStart()) {
            break;
        }
        backwardPos = Iterator().Index();
        BackwardAndSkipSpace(1);
        currentSourceView = SourceView(START_POS, Iterator().Index() + COLLECT_CURRENT_POS);
    }

    if (backwardPos == jsdocEndPos) {
        return EMPTY_JSDOC;
    }
    return SourceView(backwardPos, jsdocEndPos);
}

// Note: Return first matched string that starts with `/*` or `/**` and ends with `*/`
util::StringView JsdocHelper::GetLicenseStringFromStart()
{
    Iterator().Reset(START_POS);
    auto licenseStart = START_POS;
    do {
        const char32_t cp = Iterator().Peek();
        switch (cp) {
            case util::StringView::Iterator::INVALID_CP: {
                break;
            }
            case lexer::LEX_CHAR_ASTERISK: {
                Forward(1);
                if (Iterator().Peek() == lexer::LEX_CHAR_SLASH) {
                    Forward(1);
                    break;
                }
                continue;
            }
            case lexer::LEX_CHAR_SLASH: {
                Forward(1);
                if (Iterator().Peek() == lexer::LEX_CHAR_ASTERISK) {
                    licenseStart = Iterator().Index() - 1;
                }
                continue;
            }
            default: {
                Iterator().SkipCp();
                continue;
            }
        }
        break;
    } while (true);

    return SourceView(licenseStart, Iterator().Index());
}
}  // namespace ark::es2panda::parser
