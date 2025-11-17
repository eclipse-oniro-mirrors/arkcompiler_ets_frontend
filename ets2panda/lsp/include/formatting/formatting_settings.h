/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FORMATTING_SETTINGS_H
#define FORMATTING_SETTINGS_H

#include <string>
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {

static const std::string DEFAULT_NEWLINE_CHARACTER = "\n";

enum class IndentStyle { NONE, BLOCK, SMART };

enum class SemicolonPreference { IGNORE, INSERT, REMOVE };

struct EditorSettings {
public:
    explicit EditorSettings() = default;

    size_t GetBaseIndentSize() const
    {
        return baseIndentSize_;
    }

    size_t GetIndentSize() const
    {
        return indentSize_;
    }

    size_t GetTabSize() const
    {
        return tabSize_;
    }

    std::string GetNewLineCharacter() const
    {
        return newLineCharacter_;
    }

    bool GetConvertTabsToSpaces() const
    {
        return convertTabsToSpaces_;
    }

    IndentStyle GetIndentStyle() const
    {
        return indentStyle_;
    }

    bool GetTrimTrailingWhitespace() const
    {
        return trimTrailingWhitespace_;
    }

    void SetBaseIndentSize(size_t value)
    {
        baseIndentSize_ = value;
    }

    void SetIndentSize(size_t value)
    {
        indentSize_ = value;
    }

    void SetTabSize(size_t value)
    {
        tabSize_ = value;
    }

    void SetNewLineCharacter(const std::string &value)
    {
        newLineCharacter_ = value;
    }

    void SetConvertTabsToSpaces(bool value)
    {
        convertTabsToSpaces_ = value;
    }

    void SetIndentStyle(IndentStyle value)
    {
        indentStyle_ = value;
    }

    void SetTrimTrailingWhitespace(bool value)
    {
        trimTrailingWhitespace_ = value;
    }

private:
    size_t baseIndentSize_ = 4;
    size_t indentSize_ = 4;
    size_t tabSize_ = 4;
    std::string newLineCharacter_ = DEFAULT_NEWLINE_CHARACTER;
    bool convertTabsToSpaces_ = true;
    IndentStyle indentStyle_ = IndentStyle::SMART;
    bool trimTrailingWhitespace_ = true;
};

struct FormatCodeSettings : public EditorSettings {
public:
    explicit FormatCodeSettings() = default;

    bool GetInsertSpaceAfterCommaDelimiter() const
    {
        return insertSpaceAfterCommaDelimiter_;
    }

    bool GetInsertSpaceAfterSemicolonInForStatements() const
    {
        return insertSpaceAfterSemicolonInForStatements_;
    }

    bool GetInsertSpaceBeforeAndAfterBinaryOperators() const
    {
        return insertSpaceBeforeAndAfterBinaryOperators_;
    }

    bool GetInsertSpaceAfterConstructor() const
    {
        return insertSpaceAfterConstructor_;
    }

    bool GetInsertSpaceAfterKeywordsInControlFlowStatements() const
    {
        return insertSpaceAfterKeywordsInControlFlowStatements_;
    }

    bool GetInsertSpaceAfterFunctionKeywordForAnonymousFunctions() const
    {
        return insertSpaceAfterFunctionKeywordForAnonymousFunctions_;
    }

    bool GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis() const
    {
        return insertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis_;
    }

    bool GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets() const
    {
        return insertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets_;
    }

    bool GetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces() const
    {
        return insertSpaceAfterOpeningAndBeforeClosingNonemptyBraces_;
    }

    bool GetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces() const
    {
        return insertSpaceAfterOpeningAndBeforeClosingEmptyBraces_;
    }

    bool GetInsertSpaceAfterOpeningAndBeforeClosingTemplateStringBraces() const
    {
        return insertSpaceAfterOpeningAndBeforeClosingTemplateStringBraces_;
    }

    bool GetInsertSpaceAfterTypeAssertion() const
    {
        return insertSpaceAfterTypeAssertion_;
    }

    bool GetInsertSpaceBeforeFunctionParenthesis() const
    {
        return insertSpaceBeforeFunctionParenthesis_;
    }

    bool GetPlaceOpenBraceOnNewLineForFunctions() const
    {
        return placeOpenBraceOnNewLineForFunctions_;
    }

    bool GetPlaceOpenBraceOnNewLineForControlBlocks() const
    {
        return placeOpenBraceOnNewLineForControlBlocks_;
    }

    bool GetInsertSpaceBeforeTypeAnnotation() const
    {
        return insertSpaceBeforeTypeAnnotation_;
    }

    bool GetIndentMultiLineObjectLiteralBeginningOnBlankLine() const
    {
        return indentMultiLineObjectLiteralBeginningOnBlankLine_;
    }

    SemicolonPreference GetSemicolons() const
    {
        return semicolons_;
    }

    void SetInsertSpaceAfterCommaDelimiter(bool value)
    {
        insertSpaceAfterCommaDelimiter_ = value;
    }

    void SetInsertSpaceAfterSemicolonInForStatements(bool value)
    {
        insertSpaceAfterSemicolonInForStatements_ = value;
    }

    void SetInsertSpaceBeforeAndAfterBinaryOperators(bool value)
    {
        insertSpaceBeforeAndAfterBinaryOperators_ = value;
    }

    void SetInsertSpaceAfterConstructor(bool value)
    {
        insertSpaceAfterConstructor_ = value;
    }

    void SetInsertSpaceAfterKeywordsInControlFlowStatements(bool value)
    {
        insertSpaceAfterKeywordsInControlFlowStatements_ = value;
    }

    void SetInsertSpaceAfterFunctionKeywordForAnonymousFunctions(bool value)
    {
        insertSpaceAfterFunctionKeywordForAnonymousFunctions_ = value;
    }

    void SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis(bool value)
    {
        insertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis_ = value;
    }

    void SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets(bool value)
    {
        insertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets_ = value;
    }

    void SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces(bool value)
    {
        insertSpaceAfterOpeningAndBeforeClosingNonemptyBraces_ = value;
    }

    void SetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces(bool value)
    {
        insertSpaceAfterOpeningAndBeforeClosingEmptyBraces_ = value;
    }

    void SetInsertSpaceAfterOpeningAndBeforeClosingTemplateStringBraces(bool value)
    {
        insertSpaceAfterOpeningAndBeforeClosingTemplateStringBraces_ = value;
    }

    void SetInsertSpaceAfterTypeAssertion(bool value)
    {
        insertSpaceAfterTypeAssertion_ = value;
    }

    void SetInsertSpaceBeforeFunctionParenthesis(bool value)
    {
        insertSpaceBeforeFunctionParenthesis_ = value;
    }

    void SetPlaceOpenBraceOnNewLineForFunctions(bool value)
    {
        placeOpenBraceOnNewLineForFunctions_ = value;
    }

    void SetPlaceOpenBraceOnNewLineForControlBlocks(bool value)
    {
        placeOpenBraceOnNewLineForControlBlocks_ = value;
    }

    void SetInsertSpaceBeforeTypeAnnotation(bool value)
    {
        insertSpaceBeforeTypeAnnotation_ = value;
    }

    void SetIndentMultiLineObjectLiteralBeginningOnBlankLine(bool value)
    {
        indentMultiLineObjectLiteralBeginningOnBlankLine_ = value;
    }

    void SetSemicolons(SemicolonPreference value)
    {
        semicolons_ = value;
    }

private:
    bool insertSpaceAfterCommaDelimiter_ = true;
    bool insertSpaceAfterSemicolonInForStatements_ = true;
    bool insertSpaceBeforeAndAfterBinaryOperators_ = true;
    bool insertSpaceAfterConstructor_ = false;
    bool insertSpaceAfterKeywordsInControlFlowStatements_ = true;
    bool insertSpaceAfterFunctionKeywordForAnonymousFunctions_ = false;
    bool insertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis_ = false;
    bool insertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets_ = false;
    bool insertSpaceAfterOpeningAndBeforeClosingNonemptyBraces_ = true;
    bool insertSpaceAfterOpeningAndBeforeClosingEmptyBraces_ = false;
    bool insertSpaceAfterOpeningAndBeforeClosingTemplateStringBraces_ = false;
    bool insertSpaceAfterTypeAssertion_ = false;
    bool insertSpaceBeforeFunctionParenthesis_ = false;
    bool placeOpenBraceOnNewLineForFunctions_ = false;
    bool placeOpenBraceOnNewLineForControlBlocks_ = false;
    bool insertSpaceBeforeTypeAnnotation_ = false;
    bool indentMultiLineObjectLiteralBeginningOnBlankLine_ = false;
    SemicolonPreference semicolons_ = SemicolonPreference::IGNORE;
};

CAPI_EXPORT FormatCodeSettings
GetDefaultFormatCodeSettings(const std::string &newLineCharacter = DEFAULT_NEWLINE_CHARACTER);

}  // namespace ark::es2panda::lsp

#endif  // FORMATTING_CODE_SETTINGS_H