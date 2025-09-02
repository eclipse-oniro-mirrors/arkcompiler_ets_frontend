/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.Apache.Org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "helpers.h"
#include <iomanip>

#include "checker/ETSchecker.h"

#include "parser/program/program.h"
#include "varbinder/privateBinding.h"
#include "varbinder/ETSBinder.h"
#include "lexer/token/letters.h"

#include "ir/base/classDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/classProperty.h"
#include "ir/base/property.h"
#include "ir/base/spreadElement.h"
#include "ir/base/methodDefinition.h"

#include "ir/expressions/identifier.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/literals/booleanLiteral.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/objectExpression.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/assignmentExpression.h"

#include "ir/statements/variableDeclarator.h"

#include "ir/module/importSpecifier.h"
#include "ir/module/importDefaultSpecifier.h"

#include "ir/ets/etsImportDeclaration.h"
#include "ir/ets/etsParameterExpression.h"

#include "ir/ts/tsParameterProperty.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsEnumDeclaration.h"

#include "utils/utf.h"

namespace ark::es2panda::util {
// Helpers

bool Helpers::IsGlobalIdentifier(const util::StringView &str)
{
    return (str.Is("NaN") || str.Is("undefined") || str.Is("Infinity"));
}

bool Helpers::ContainSpreadElement(const ArenaVector<ir::Expression *> &args)
{
    return std::any_of(args.begin(), args.end(), [](const auto *it) { return it->IsSpreadElement(); });
}

util::StringView Helpers::LiteralToPropName(const ir::Expression *lit)
{
    switch (lit->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            return lit->AsIdentifier()->Name();
        }
        case ir::AstNodeType::STRING_LITERAL: {
            return lit->AsStringLiteral()->Str();
        }
        case ir::AstNodeType::NUMBER_LITERAL: {
            return lit->AsNumberLiteral()->Str();
        }
        case ir::AstNodeType::NULL_LITERAL: {
            return "null";
        }
        case ir::AstNodeType::UNDEFINED_LITERAL: {
            return "undefined";
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

bool Helpers::IsIndex(double number)
{
    if (number >= 0 && number < static_cast<double>(INVALID_INDEX)) {
        auto intNum = static_cast<uint32_t>(number);

        if (static_cast<double>(intNum) == number) {
            return true;
        }
    }

    return false;
}

static bool IsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

int64_t Helpers::GetIndex(const util::StringView &str)
{
    const auto &s = str.Utf8();

    if (s.empty() || (*s.begin() == '0' && s.length() > 1)) {
        return INVALID_INDEX;
    }

    int64_t value = 0;
    for (const auto c : s) {
        if (!IsDigit(c)) {
            return INVALID_INDEX;
        }

        constexpr auto MULTIPLIER = 10;
        value *= MULTIPLIER;
        value += (c - '0');

        if (value >= INVALID_INDEX) {
            return INVALID_INDEX;
        }
    }

    return value;
}

std::string Helpers::ToString(double number)
{
    std::string str;

    if (Helpers::IsInteger<int32_t>(number)) {
        str = std::to_string(static_cast<int32_t>(number));
    } else {
        str = std::to_string(number);
    }

    return str;
}

util::StringView Helpers::ToStringView(ArenaAllocator *allocator, double number)
{
    util::UString str(ToString(number), allocator);
    return str.View();
}

util::StringView Helpers::ToStringView(ArenaAllocator *allocator, uint32_t number)
{
    ES2PANDA_ASSERT(number <= static_cast<uint32_t>(std::numeric_limits<int32_t>::max()));
    return ToStringView(allocator, static_cast<int32_t>(number));
}

util::StringView Helpers::ToStringView(ArenaAllocator *allocator, int32_t number)
{
    util::UString str(ToString(number), allocator);
    return str.View();
}

bool Helpers::EndsWith(const std::string &str, const std::string &suffix)
{
    if (str.length() < suffix.length()) {
        return false;
    }
    size_t expectPos = str.length() - suffix.length();
    return str.find(suffix, expectPos) == expectPos;
}

const ir::ScriptFunction *Helpers::GetContainingConstructor(const ir::AstNode *node)
{
    const ir::ScriptFunction *iter = GetContainingFunction(node);

    while (iter != nullptr) {
        if (iter->IsConstructor()) {
            return iter;
        }

        if (!iter->IsArrow()) {
            return nullptr;
        }

        iter = GetContainingFunction(iter);
    }

    return iter;
}

const ir::TSEnumDeclaration *Helpers::GetContainingEnumDeclaration(const ir::AstNode *node)
{
    auto *iter = node;

    while (iter != nullptr) {
        if (iter->IsTSEnumDeclaration()) {
            return iter->AsTSEnumDeclaration();
        }

        iter = iter->Parent();
    }

    return nullptr;
}

const checker::ETSObjectType *Helpers::GetContainingObjectType(const ir::AstNode *node)
{
    const auto *iter = node;

    while (iter != nullptr) {
        if (iter->IsClassDefinition()) {
            auto *ret = iter->AsClassDefinition()->TsType();
            return ret != nullptr ? ret->AsETSObjectType() : nullptr;
        }

        if (iter->IsTSInterfaceDeclaration()) {
            auto *ret = iter->AsTSInterfaceDeclaration()->TsType();
            return ret != nullptr ? ret->AsETSObjectType() : nullptr;
        }

        if (iter->IsTSEnumDeclaration()) {
            auto *ret = iter->AsTSEnumDeclaration()->TsType();
            return ret != nullptr ? ret->AsETSObjectType() : nullptr;
        }

        iter = iter->Parent();
    }

    return nullptr;
}

const ir::ClassDefinition *Helpers::GetContainingClassDefinition(const ir::AstNode *node)
{
    const auto *iter = node;

    while (iter != nullptr) {
        if (iter->IsClassDefinition()) {
            return iter->AsClassDefinition();
        }

        iter = iter->Parent();
    }

    return nullptr;
}

const ir::TSInterfaceDeclaration *Helpers::GetContainingInterfaceDeclaration(const ir::AstNode *node)
{
    const auto *iter = node;

    while (iter != nullptr) {
        if (iter->IsTSInterfaceDeclaration()) {
            return iter->AsTSInterfaceDeclaration();
        }

        iter = iter->Parent();
    }

    return nullptr;
}

const ir::MethodDefinition *Helpers::GetContainingClassMethodDefinition(const ir::AstNode *node)
{
    const auto *iter = node;

    while (iter != nullptr) {
        if (iter->IsMethodDefinition()) {
            return iter->AsMethodDefinition();
        }

        if (iter->IsClassDefinition()) {
            break;
        }

        iter = iter->Parent();
    }

    return nullptr;
}

const ir::ClassStaticBlock *Helpers::GetContainingClassStaticBlock(const ir::AstNode *node)
{
    const auto *iter = node;

    while (iter != nullptr) {
        if (iter->IsClassStaticBlock()) {
            return iter->AsClassStaticBlock();
        }

        if (iter->IsClassDefinition()) {
            break;
        }

        iter = iter->Parent();
    }

    return nullptr;
}

const ir::ScriptFunction *Helpers::GetContainingConstructor(const ir::ClassProperty *node)
{
    for (const auto *parent = node->Parent(); parent != nullptr; parent = parent->Parent()) {
        if (parent->IsClassDefinition()) {
            ES2PANDA_ASSERT(parent->AsClassDefinition()->Ctor() != nullptr);
            return parent->AsClassDefinition()->Ctor()->Function();
        }
    }

    return nullptr;
}

const ir::ScriptFunction *Helpers::GetContainingFunction(const ir::AstNode *node)
{
    for (const auto *parent = node->Parent(); parent != nullptr; parent = parent->Parent()) {
        if (parent->IsScriptFunction()) {
            return parent->AsScriptFunction();
        }
    }

    return nullptr;
}

const ir::ClassDefinition *Helpers::GetClassDefiniton(const ir::ScriptFunction *node)
{
    ES2PANDA_ASSERT(node->IsConstructor());
    ES2PANDA_ASSERT(node->Parent()->IsFunctionExpression());
    ES2PANDA_ASSERT(node->Parent()->Parent()->IsMethodDefinition());
    ES2PANDA_ASSERT(node->Parent()->Parent()->Parent()->IsClassDefinition());

    return node->Parent()->Parent()->Parent()->AsClassDefinition();
}

bool Helpers::IsSpecialPropertyKey(const ir::Expression *expr)
{
    if (!expr->IsStringLiteral()) {
        return false;
    }

    auto *lit = expr->AsStringLiteral();
    return lit->Str().Is("prototype") || lit->Str().Is("constructor");
}

bool Helpers::IsConstantPropertyKey(const ir::Expression *expr, bool isComputed)
{
    switch (expr->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            return !isComputed;
        }
        case ir::AstNodeType::NUMBER_LITERAL:
        case ir::AstNodeType::STRING_LITERAL:
        case ir::AstNodeType::BOOLEAN_LITERAL:
        case ir::AstNodeType::NULL_LITERAL: {
            return true;
        }
        default:
            break;
    }

    return false;
}

compiler::Literal Helpers::ToConstantLiteral(const ir::Expression *expr)
{
    switch (expr->Type()) {
        case ir::AstNodeType::NUMBER_LITERAL: {
            auto *lit = expr->AsNumberLiteral();
            if (util::Helpers::IsInteger<uint32_t>(lit->Number().GetDouble())) {
                return compiler::Literal(static_cast<uint32_t>(lit->Number().GetDouble()));
            }
            return compiler::Literal(lit->Number().GetDouble());
        }
        case ir::AstNodeType::STRING_LITERAL: {
            auto *lit = expr->AsStringLiteral();
            return compiler::Literal(lit->Str());
        }
        case ir::AstNodeType::BOOLEAN_LITERAL: {
            auto *lit = expr->AsBooleanLiteral();
            return compiler::Literal(lit->Value());
        }
        case ir::AstNodeType::NULL_LITERAL: {
            return compiler::Literal::NullLiteral();
        }
        case ir::AstNodeType::UNDEFINED_LITERAL: {
            return compiler::Literal::UndefinedLiteral();
        }
        default:
            break;
    }

    return compiler::Literal();
}

bool Helpers::IsBindingPattern(const ir::AstNode *node)
{
    return node->IsArrayPattern() || node->IsObjectPattern();
}

bool Helpers::IsPattern(const ir::AstNode *node)
{
    return node->IsArrayPattern() || node->IsObjectPattern() || node->IsAssignmentPattern();
}

static void CollectBindingName(varbinder::VarBinder *vb, ir::AstNode *node, std::vector<ir::Identifier *> *bindings)
{
    switch (node->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            if (!vb->IsGlobalIdentifier(node->AsIdentifier()->Name())) {
                bindings->push_back(node->AsIdentifier());
            }

            break;
        }
        case ir::AstNodeType::OBJECT_PATTERN: {
            for (auto *prop : node->AsObjectPattern()->Properties()) {
                CollectBindingName(vb, prop, bindings);
            }
            break;
        }
        case ir::AstNodeType::ARRAY_PATTERN: {
            for (auto *element : node->AsArrayPattern()->Elements()) {
                CollectBindingName(vb, element, bindings);
            }
            break;
        }
        case ir::AstNodeType::ASSIGNMENT_PATTERN: {
            CollectBindingName(vb, node->AsAssignmentPattern()->Left(), bindings);
            break;
        }
        case ir::AstNodeType::PROPERTY: {
            CollectBindingName(vb, node->AsProperty()->Value(), bindings);
            break;
        }
        case ir::AstNodeType::REST_ELEMENT: {
            CollectBindingName(vb, node->AsRestElement()->Argument(), bindings);
            break;
        }
        default:
            break;
    }
}

std::vector<ir::Identifier *> Helpers::CollectBindingNames(varbinder::VarBinder *vb, ir::Expression *node)
{
    std::vector<ir::Identifier *> bindings;
    CollectBindingName(vb, node, &bindings);
    return bindings;
}

void Helpers::CheckImportedName(const ArenaVector<ir::ImportSpecifier *> &specifiers,
                                const ir::ImportSpecifier *specifier, const std::string &fileName)
{
    auto newIdentName = specifier->Imported()->Name();
    auto newAliasName = specifier->Local()->Name();
    std::stringstream message {};

    for (auto *it : specifiers) {
        auto savedIdentName = it->Imported()->Name();
        auto savedAliasName = it->Local()->Name();
        if (savedIdentName == savedAliasName && savedAliasName == newIdentName) {
            message << "Warning: '" << newIdentName << "' has already imported ";
            break;
        }
        if (savedIdentName == newIdentName && newAliasName != savedAliasName) {
            message << "Warning: '" << newIdentName << "' is explicitly used with alias several times ";
            break;
        }
    }

    if (message.rdbuf()->in_avail() > 0) {
        std::cerr << message.str() << "[" << fileName.c_str() << ":" << specifier->Start().line << ":"
                  << specifier->Start().index << "]" << std::endl;
    }
}

void Helpers::CheckDefaultImportedName(const ArenaVector<ir::ImportDefaultSpecifier *> &specifiers,
                                       const ir::ImportDefaultSpecifier *specifier, const std::string &fileName)
{
    for (auto *it : specifiers) {
        if (specifier->Local()->Name() != it->Local()->Name()) {
            std::cerr << "Warning: default element is explicitly used with alias several times [" << fileName.c_str()
                      << ":" << specifier->Start().line << ":" << specifier->Start().index << "]" << std::endl;
            return;
        }
    }
}

void Helpers::CheckDefaultImport(const ArenaVector<ir::ETSImportDeclaration *> &statements)
{
    for (auto statement : statements) {
        for (auto specifier : statement->Specifiers()) {
            if (specifier->Type() == ir::AstNodeType::IMPORT_DEFAULT_SPECIFIER) {
                auto fileName = statement->ResolvedSource();
                std::cerr << "Warning: default element has already imported [" << fileName << ":"
                          << specifier->Start().line << ":" << specifier->Start().index << "]" << std::endl;
                return;
            }
        }
    }
}

static util::StringView FunctionNameFromParent(const ir::AstNode *parent, ArenaAllocator *allocator)
{
    switch (parent->Type()) {
        case ir::AstNodeType::VARIABLE_DECLARATOR: {
            const ir::VariableDeclarator *varDecl = parent->AsVariableDeclarator();

            if (varDecl->Id()->IsIdentifier()) {
                return varDecl->Id()->AsIdentifier()->Name();
            }

            break;
        }
        case ir::AstNodeType::METHOD_DEFINITION: {
            const ir::MethodDefinition *methodDef = parent->AsMethodDefinition();

            if (methodDef->Key()->IsIdentifier()) {
                auto *ident = methodDef->Id();
                ES2PANDA_ASSERT(ident != nullptr);

                if (!ident->IsPrivateIdent()) {
                    return ident->Name();
                }

                return util::UString(varbinder::PrivateBinding::ToPrivateBinding(ident->Name()), allocator).View();
            }

            break;
        }
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION: {
            const ir::AssignmentExpression *assignment = parent->AsAssignmentExpression();

            if (assignment->Left()->IsIdentifier()) {
                return assignment->Left()->AsIdentifier()->Name();
            }

            break;
        }
        case ir::AstNodeType::ASSIGNMENT_PATTERN: {
            const ir::AssignmentExpression *assignment = parent->AsAssignmentPattern();

            if (assignment->Left()->IsIdentifier()) {
                return assignment->Left()->AsIdentifier()->Name();
            }

            break;
        }
        case ir::AstNodeType::PROPERTY: {
            const ir::Property *prop = parent->AsProperty();

            if (prop->Kind() != ir::PropertyKind::PROTO &&
                Helpers::IsConstantPropertyKey(prop->Key(), prop->IsComputed())) {
                return Helpers::LiteralToPropName(prop->Key());
            }

            break;
        }
        default:
            break;
    }

    return util::StringView();
}

util::StringView Helpers::FunctionName(ArenaAllocator *allocator, const ir::ScriptFunction *func)
{
    if (func->Id() != nullptr) {
        return func->Id()->Name();
    }

    if (func->Parent()->IsFunctionDeclaration()) {
        return "*default*";
    }

    const ir::AstNode *parent = func->Parent()->Parent();

    if (func->IsConstructor()) {
        parent = parent->Parent();
        if (parent->AsClassDefinition()->Ident() != nullptr) {
            return parent->AsClassDefinition()->Ident()->Name();
        }

        parent = parent->Parent()->Parent();
    }

    return FunctionNameFromParent(parent, allocator);
}

std::tuple<util::StringView, bool> Helpers::ParamName(ArenaAllocator *allocator, const ir::Expression *param,
                                                      std::uint32_t index)
{
    switch (param->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            return {param->AsIdentifier()->Name(), false};
        }
        case ir::AstNodeType::ASSIGNMENT_PATTERN: {
            const auto *lhs = param->AsAssignmentPattern()->Left();
            if (lhs->IsIdentifier()) {
                return {param->AsAssignmentPattern()->Left()->AsIdentifier()->Name(), false};
            }
            break;
        }
        case ir::AstNodeType::REST_ELEMENT: {
            if (param->AsRestElement()->Argument()->IsIdentifier()) {
                return {param->AsRestElement()->Argument()->AsIdentifier()->Name(), false};
            }
            break;
        }
        case ir::AstNodeType::TS_PARAMETER_PROPERTY: {
            return ParamName(allocator, param->AsTSParameterProperty()->Parameter(), index);
        }
        case ir::AstNodeType::ETS_PARAMETER_EXPRESSION: {
            return {param->AsETSParameterExpression()->Name(), false};
        }
        default:
            break;
    }

    return {Helpers::ToStringView(allocator, index), true};
}

static std::string GetEscapedCharacter(const unsigned char c)
{
    std::stringstream escapedStr;
    escapedStr << '\\';
    switch (c) {
        case lexer::LEX_CHAR_DOUBLE_QUOTE: {
            escapedStr << '"';
            break;
        }
        case lexer::LEX_CHAR_BS: {
            escapedStr << 'b';
            break;
        }
        case lexer::LEX_CHAR_TAB: {
            escapedStr << 't';
            break;
        }
        case lexer::LEX_CHAR_LF: {
            escapedStr << 'n';
            break;
        }
        case lexer::LEX_CHAR_VT: {
            escapedStr << 'v';
            break;
        }
        case lexer::LEX_CHAR_FF: {
            escapedStr << 'f';
            break;
        }
        case lexer::LEX_CHAR_CR: {
            escapedStr << 'r';
            break;
        }
        case lexer::LEX_CHAR_NULL: {
            escapedStr << '0';
            break;
        }
        default: {
            escapedStr << 'u' << std::hex << std::setw(4U) << std::setfill('0') << static_cast<unsigned int>(c);
            break;
        }
    }
    return escapedStr.str();
}

std::string Helpers::CreateEscapedString(const std::string &str)
{
    std::string escapedStr;
    for (const unsigned char c : str) {
        // check if a given character is printable
        // the cast is necessary to avoid undefined behaviour
        if (LIKELY((std::isprint(c) != 0U || c >= lexer::LEX_ASCII_MAX_BITS) && c != lexer::LEX_CHAR_DOUBLE_QUOTE)) {
            escapedStr += c;
        } else {
            escapedStr += GetEscapedCharacter(c);
        }
    }
    return escapedStr;
}

std::string Helpers::UTF16toUTF8(const char16_t c)
{
    const utf::Utf8Char utf8Ch = utf::ConvertUtf16ToUtf8(c, 0, false);
    return std::string(reinterpret_cast<const char *>(utf8Ch.ch.data()), utf8Ch.n);
}

std::pair<std::string_view, std::string_view> Helpers::SplitSignature(std::string_view signature)
{
    auto idx = signature.find_last_of(':');
    auto stripped = signature.substr(0, idx);
    idx = stripped.find_last_of('.');
    auto fullClassName = stripped.substr(0, idx);
    auto methodName = stripped.substr(idx + 1);
    idx = fullClassName.find_last_of('.');
    auto className = fullClassName.substr(idx + 1);
    return {className, methodName};
}

std::vector<std::string> const &Helpers::StdLib()
{
    static std::vector<std::string> stdlib {"std/core", "std/math",        "std/containers",        "std/interop/js",
                                            "std/time", "std/debug",       "std/debug/concurrency", "std/testing",
                                            "escompat", "std/concurrency", "std/annotations",       "std/interop"};
    return stdlib;
}

bool Helpers::IsStdLib(const parser::Program *program)
{
    // NOTE(rsipka): early check: if program is not in a package then it is not part of the stdlib either
    if (!program->IsPackage()) {
        return false;
    }

    auto fileFolder = program->ModuleName().Mutf8();
    std::replace(fileFolder.begin(), fileFolder.end(), *compiler::Signatures::METHOD_SEPARATOR.begin(),
                 *compiler::Signatures::NAMESPACE_SEPARATOR.begin());

    if (fileFolder == "std/math/consts") {
        return true;
    }

    auto const &stdlib = StdLib();
    return std::count(stdlib.begin(), stdlib.end(), fileFolder) != 0;
}

checker::Type *Helpers::CheckReturnTypeOfCheck([[maybe_unused]] const ir::AstNode *const node,
                                               checker::Type *const type)
{
    ES2PANDA_ASSERT(type != nullptr || !node->IsExpression());
    return type;
}

util::UString Helpers::EscapeHTMLString(ArenaAllocator *allocator, const std::string &str)
{
    util::UString replaced(allocator);
    for (const auto c : str) {
        switch (c) {
            case '<':
                replaced.Append("&lt;");
                break;
            case '>':
                replaced.Append("&gt;");
                break;
            case '&':
                replaced.Append("&amp;");
                break;
            case '"':
                replaced.Append("&quot;");
                break;
            case '\'':
                replaced.Append("&apos;");
                break;
            default:
                replaced.Append(c);
                break;
        }
    }
    return replaced;
}

ir::AstNode *Helpers::DerefETSTypeReference(ir::AstNode *node)
{
    ES2PANDA_ASSERT(node->IsETSTypeReference());
    do {
        auto *name = node->AsETSTypeReference()->Part()->GetIdent();

        ES2PANDA_ASSERT(name->IsIdentifier());
        if (varbinder::ETSBinder::IsSpecialName(name->Name())) {
            return node;
        }
        auto *var = name->AsIdentifier()->Variable();
        ES2PANDA_ASSERT(var != nullptr);
        auto *declNode = var->Declaration()->Node();
        if (!declNode->IsTSTypeAliasDeclaration()) {
            return declNode;
        }
        node = declNode->AsTSTypeAliasDeclaration()->TypeAnnotation();
    } while (node->IsETSTypeReference());
    return node;
}

bool Helpers::IsAsyncMethod(ir::AstNode const *node)
{
    if (!node->IsMethodDefinition()) {
        return false;
    }
    auto *method = node->AsMethodDefinition();
    ES2PANDA_ASSERT(method->Function() != nullptr);
    return method->Function()->IsAsyncFunc() && !method->Function()->IsProxy();
}

bool Helpers::IsGlobalVar(const ark::es2panda::varbinder::Variable *var)
{
    return var->Declaration()->Node()->IsClassDeclaration() &&
           var->Declaration()->Node()->AsClassDeclaration()->Definition()->IsGlobal();
}

}  // namespace ark::es2panda::util
