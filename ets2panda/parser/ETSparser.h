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

#ifndef ES2PANDA_PARSER_CORE_ETS_PARSER_H
#define ES2PANDA_PARSER_CORE_ETS_PARSER_H

#include "util/arktsconfig.h"
#include "util/importPathManager.h"
#include "innerSourceParser.h"
#include "TypedParser.h"
#include "ir/base/classDefinition.h"
#include "ir/module/importDeclaration.h"
#include <utility>

namespace ark::es2panda::ir {
class ETSPackageDeclaration;
enum class ClassDefinitionModifiers : uint32_t;
enum class PrimitiveType;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::parser {

struct SpecifiersInfo {
    ArenaVector<ir::ImportSpecifier *> result;
    ArenaVector<ir::ImportDefaultSpecifier *> resultDefault;
    ArenaVector<ir::ExportSpecifier *> resultExportDefault;
};

class ETSParser final : public TypedParser {
public:
    ETSParser(Program *program, const util::Options &options, util::DiagnosticEngine &diagnosticEngine,
              ParserStatus status);
    ETSParser(Program *program, std::nullptr_t options, util::DiagnosticEngine &diagnosticEngine);

    ETSParser() = delete;
    NO_COPY_SEMANTIC(ETSParser);
    NO_MOVE_SEMANTIC(ETSParser);

    ~ETSParser() final = default;

    util::ImportPathManager *GetImportPathManager()
    {
        return importPathManager_.get();
    }

    util::StringView GetGlobalProgramAbsName()
    {
        return globalProgram_->AbsoluteName();
    }

    Program *GetGlobalProgram()
    {
        return globalProgram_;
    }

    [[nodiscard]] bool IsETSParser() const noexcept override;

    [[nodiscard]] bool IsValidIdentifierName(const lexer::Token &token) const noexcept override;
    void AddDirectImportsToDirectExternalSources(const std::vector<util::StringView> &directImportsFromMainSource,
                                                 parser::Program *newProg) const;
    bool CheckDupAndReplace(Program *&oldProg, Program *newProg) const;
    ArenaVector<ir::ETSImportDeclaration *> ParseDefaultSources(std::string_view srcFile, std::string_view importSrc);

public:
    //============================================================================================//
    // Methods to create AST node(s) from the specified string (part of valid ETS-code!)
    // NOTE: ScopeInitPhase, SetParent should be called on created subtree after calling any of these methods,
    //============================================================================================//

    template <typename>
    static constexpr bool STATIC_FALSE = false;

    std::string_view FormattingFileName()
    {
        return GetContext().FormattingFileName();
    }

    ir::TypeNode *CreateFormattedTypeAnnotation(std::string_view const sourceCode);
    ir::TypeNode *CreateFormattedTypeAnnotation(std::string_view sourceCode, std::vector<ir::AstNode *> &args);

#include "parser/ETSparserTemplates.h"

    ir::Expression *CreateExpression(std::string_view sourceCode,
                                     ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    ir::Expression *CreateFormattedExpression(std::string_view sourceCode, std::vector<ir::AstNode *> &insertingNodes);

    ir::Expression *CreateFormattedExpression(std::string_view const sourceCode, ArenaVector<ir::Expression *> &args);
    ir::Expression *CreateFormattedExpression(std::string_view const sourceCode, std::vector<ir::Expression *> &args);
    ir::Statement *CreateFormattedStatement(std::string_view sourceCode, std::vector<ir::AstNode *> &insertingNodes);
    ArenaVector<ir::Statement *> CreateStatements(std::string_view sourceCode);
    ArenaVector<ir::Statement *> CreateFormattedStatements(std::string_view sourceCode,
                                                           std::vector<ir::AstNode *> &insertingNodes);
    ir::Statement *ParseTopLevelAnnotation(ir::ModifierFlags memberModifiers);
    ArenaVector<ir::AnnotationUsage *> ParseAnnotations(bool isTopLevelSt);
    ir::Statement *CreateFormattedClassDeclaration(std::string_view sourceCode,
                                                   std::vector<ir::AstNode *> &insertingNodes,
                                                   bool allowStatic = false);
    ir::AstNode *CreateFormattedClassElement(std::string_view sourceCode, std::vector<ir::AstNode *> &insertingNodes,
                                             const ArenaVector<ir::AstNode *> &properties,
                                             ir::ClassDefinitionModifiers modifiers);
    ir::AstNode *CreateFormattedClassFieldDefinition(std::string_view sourceCode,
                                                     std::vector<ir::AstNode *> &insertingNodes);
    ir::AstNode *CreateFormattedClassMethodDefinition(std::string_view sourceCode,
                                                      std::vector<ir::AstNode *> &insertingNodes);
    ir::Statement *CreateFormattedTopLevelStatement(std::string_view sourceCode,
                                                    std::vector<ir::AstNode *> &insertingNodes);
    void ApplyAnnotationsToNode(ir::AstNode *node, ArenaVector<ir::AnnotationUsage *> &&annotations,
                                lexer::SourcePosition pos,
                                TypeAnnotationParsingOptions options = TypeAnnotationParsingOptions::NO_OPTS);
    [[nodiscard]] bool IsInitializerBlockStart() const;

    uint32_t GetNamespaceNestedRank()
    {
        return namespaceNestedRank_;
    }

    void IncrementNamespaceNestedRank()
    {
        namespaceNestedRank_++;
    }

    void DecrementNamespaceNestedRank()
    {
        namespaceNestedRank_--;
    }

    ir::ETSImportDeclaration *BuildImportDeclaration(ir::ImportKinds importKind,
                                                     ArenaVector<ir::AstNode *> &&specifiers,
                                                     ir::StringLiteral *pathToResolve, parser::Program *program,
                                                     util::ImportFlags importFlag);

    void AddExternalSource(const std::vector<Program *> &programs);
    std::vector<Program *> ParseSources(bool firstSource = false);
    static void AddGenExtenralSourceToParseList(public_lib::Context *ctx);

private:
    NodeFormatType GetFormatPlaceholderType();
    ir::Statement *ParseStatementFormatPlaceholder() override;
    ir::Expression *ParseExpressionFormatPlaceholder();
    ir::Identifier *ParseIdentifierFormatPlaceholder(std::optional<NodeFormatType> nodeFormat) override;
    ir::TypeNode *ParseTypeFormatPlaceholder(std::optional<NodeFormatType> nodeFormat = std::nullopt);
    ir::AstNode *ParseTypeParametersFormatPlaceholder() override;
    void ApplyAnnotationsToArrayType(ir::AstNode *node, ArenaVector<ir::AnnotationUsage *> &&annotations,
                                     lexer::SourcePosition pos);
    void ApplyAnnotationsToSpecificNodeType(ir::AstNode *node, ArenaVector<ir::AnnotationUsage *> &&annotations,
                                            lexer::SourcePosition pos);
    ArenaVector<ir::AstNode *> &ParseAstNodesArrayFormatPlaceholder() override;
    ArenaVector<ir::Statement *> &ParseStatementsArrayFormatPlaceholder() override;
    ArenaVector<ir::Expression *> &ParseExpressionsArrayFormatPlaceholder() override;
    [[nodiscard]] std::unique_ptr<lexer::Lexer> InitFormattedLexer(std::string_view formattedName,
                                                                   std::string_view sourceCode);
    ir::Statement *CreateStatement(std::string_view sourceCode);
    ir::MethodDefinition *CreateConstructorDefinition(ir::ModifierFlags modifiers, std::string_view sourceCode);
    ir::Statement *CreateClassDeclaration(std::string_view sourceCode, bool allowStatic = false);
    ir::AstNode *CreateClassElement(std::string_view sourceCode, const ArenaVector<ir::AstNode *> &properties,
                                    ir::ClassDefinitionModifiers modifiers);
    ir::TypeNode *CreateTypeAnnotation(TypeAnnotationParsingOptions *options, std::string_view sourceCode);
    // NOLINTNEXTLINE(google-default-arguments)
    void ValidateGetterSetter(ir::MethodDefinitionKind methodDefinition, const ir::ScriptFunction *func,
                              bool hasReceiver = false) override;
    ir::Statement *CreateTopLevelStatement(std::string_view const sourceCode);

    //============================================================================================//
    //  END: Methods to create AST node(s)...
    //============================================================================================//

private:
    void ParseProgram(ScriptKind kind) override;
    [[nodiscard]] std::unique_ptr<lexer::Lexer> InitLexer(const SourceFile &sourceFile) override;
    ir::ETSPackageDeclaration *ParsePackageDeclaration();
    void AddPackageSourcesToParseList();
    ArenaVector<ir::Statement *> ParseTopLevelStatements();
    ir::AstNode *HandleAmbientDeclaration(ir::ModifierFlags &memberModifiers,
                                          const std::function<ir::AstNode *(ir::Identifier *)> &parseClassMethod);
    static bool IsClassMethodModifier(lexer::TokenType type) noexcept;

#ifdef USE_FTW
    static int NFTWCallBack(const char *fpath, const struct stat * /*unused*/, int tflag, struct FTW * /*unused*/);
#endif
    ir::ETSImportDeclaration *ParseImportPathBuildImport(ArenaVector<ir::AstNode *> &&specifiers, bool requireFrom,
                                                         lexer::SourcePosition startLoc, ir::ImportKinds importKind);
    ir::Statement *CreateReExportDeclarationNode(ir::ETSImportDeclaration *reExportDeclaration,
                                                 const lexer::SourcePosition &startLoc,
                                                 const ir::ModifierFlags &modifiers);
    void ParseNamedExportSpecifiers(ArenaVector<ir::AstNode *> *specifiers, bool defaultExport);
    void ParseUserSources(std::vector<std::string> userParths);
    ArenaVector<ir::Statement *> ParseTopLevelDeclaration();
    void ParseParseListElement(const util::ImportPathManager::ParseInfo &parseListElem, std::string_view extSrc,
                               const std::vector<util::StringView> &directImportsFromMainSource,
                               std::vector<Program *> *programs);
    bool IsDefaultImport();
    bool IsDefaultExport();
    void ParseNamedSpecifiesDefaultImport(ArenaVector<ir::ImportDefaultSpecifier *> *resultDefault,
                                          const std::string &fileName);
    void ParseNamedSpecifiersDefaultExport(ArenaVector<ir::ImportSpecifier *> *result,
                                           ArenaVector<ir::ImportDefaultSpecifier *> *resultDefault,
                                           const std::string &fileName);
    bool ParseNamedSpecifiesImport(ArenaVector<ir::ImportSpecifier *> *result,
                                   ArenaVector<ir::ExportSpecifier *> *resultExportDefault);
    SpecifiersInfo ParseNamedSpecifiers(const ir::ImportKinds importKind);
    SpecifiersInfo ParseExportNamedSpecifiers(const ir::ImportKinds exportKind);
    ir::ExportNamedDeclaration *ParseSingleExport(ir::ModifierFlags modifiers);
    ir::ExportNamedDeclaration *ParseSingleExportForAnonymousConst(ir::ModifierFlags modifiers);
    ArenaVector<ir::ETSImportDeclaration *> ParseImportDeclarations();
    ir::Statement *ParseImportDeclarationHelper(lexer::SourcePosition startLoc, ArenaVector<ir::AstNode *> &specifiers,
                                                ir::ImportKinds importKind);
    bool TryMergeFromCache(util::ImportPathManager::ImportMetadata const &importData);

    std::optional<std::string_view> GetDeclarationSource(std::string &&fileToParse) const;
    std::vector<Program *> SearchForNotParsed(ArenaVector<util::ImportPathManager::ParseInfo> &parseList,
                                              std::vector<util::StringView> &directImportsFromMainSource);
    parser::Program *ParseSource(const SourceFile &sourceFile);
    ir::ETSModule *ParseETSGlobalScript(lexer::SourcePosition startLoc, ArenaVector<ir::Statement *> &statements);
    void ParseFileHeaderFlag(lexer::SourcePosition startLoc, ArenaVector<ir::Statement *> *statements);
    ir::ETSModule *ParseImportsAndReExportOnly(lexer::SourcePosition startLoc,
                                               ArenaVector<ir::Statement *> &statements);
    ir::AstNode *ParseImportDefaultSpecifier(ArenaVector<ir::AstNode *> *specifiers) override;
    void *ApplyAnnotationsToClassElement(ir::AstNode *property, ArenaVector<ir::AnnotationUsage *> &&annotations,
                                         lexer::SourcePosition pos);
    ir::MethodDefinition *ParseClassGetterSetterMethod(const ArenaVector<ir::AstNode *> &properties,
                                                       ir::ClassDefinitionModifiers modifiers,
                                                       ir::ModifierFlags memberModifiers, bool isDefault);
    ir::MethodDefinition *ParseInterfaceGetterSetterMethod(const ir::ModifierFlags modifiers);
    ir::Statement *ParseIdentKeyword();
    ir::Statement *ParseTypeDeclaration(bool allowStatic = false);
    ir::Statement *ParseTypeDeclarationAbstractFinal(bool allowStatic, ir::ClassDefinitionModifiers modifiers);
    ir::ModifierFlags ParseClassModifiers();
    ir::ModifierFlags ParseInterfaceMethodModifiers();
    ir::AstNode *ParseInterfaceField();
    void ParseIndexedSignature();
    ir::TypeNode *ParseInterfaceTypeAnnotation(ir::Identifier *name);
    void ParseInterfaceModifiers(ir::ModifierFlags &fieldModifiers, bool &optionalField);
    ir::OverloadDeclaration *ParseInterfaceOverload(ir::ModifierFlags modifiers);
    void ThrowOptionalMethodErrorIfNeeded();
    ir::MethodDefinition *ParseInterfaceMethod(ir::ModifierFlags flags, ir::MethodDefinitionKind methodKind);
    void ReportAccessModifierError(const lexer::Token &token);
    std::tuple<ir::ModifierFlags, bool, bool> ParseClassMemberAccessModifiers();
    void ParseClassFieldModifiers(ir::ModifierFlags &memberModifiers);
    ir::ModifierFlags ParseClassMethodModifierFlag();
    void ParseClassMethodModifiers(ir::ModifierFlags &memberModifiers);
    void ValidateOverloadDeclarationModifiers(ir::ModifierFlags modifiers);
    bool ParseOverloadListElement(ArenaVector<ir::Expression *> &overloads, ir::OverloadDeclaration *overloadDecl);
    void ValidateOverloadList(ArenaVector<ir::Expression *> const &overloadList);
    ir::OverloadDeclaration *ParseClassOverloadDeclaration(ir::ModifierFlags modifiers);
    ir::MethodDefinition *ParseClassMethodDefinition(ir::Identifier *methodName, ir::ModifierFlags modifiers,
                                                     bool isDefault);
    ir::ScriptFunction *ParseFunction(ParserStatus newStatus);
    ir::MethodDefinition *ParseClassMethod(ClassElementDescriptor *desc, const ArenaVector<ir::AstNode *> &properties,
                                           ir::Expression *propName, lexer::SourcePosition *propEnd) override;
    std::tuple<bool, ir::BlockStatement *, lexer::SourcePosition, bool> ParseFunctionBody(
        const ArenaVector<ir::Expression *> &params, ParserStatus newStatus, ParserStatus contextStatus) override;
    ir::TypeNode *ParseFunctionReturnType(ParserStatus status) override;
    ir::Expression *CreateParameterThis(ir::TypeNode *typeAnnotation) override;
    ir::TypeNode *ConvertToOptionalUnionType(ir::TypeNode *typeAnno);
    void ValidateFieldModifiers(ir::ModifierFlags modifiers, bool optionalField, ir::Expression *initializer,
                                lexer::SourcePosition pos);
    // NOLINTNEXTLINE(google-default-arguments)
    void ParseClassFieldDefinition(ir::Identifier *fieldName, ir::ModifierFlags modifiers,
                                   ArenaVector<ir::AstNode *> *declarations, bool isDefault);
    std::tuple<ir::Expression *, ir::TSTypeParameterInstantiation *> ParseTypeReferencePart(
        TypeAnnotationParsingOptions *options);
    ir::TypeNode *ParseTypeReference(TypeAnnotationParsingOptions *options);
    ir::TypeNode *ParseBaseTypeReference(TypeAnnotationParsingOptions *options);
    ir::TypeNode *ParsePrimitiveType(TypeAnnotationParsingOptions *options, ir::PrimitiveType type);
    ir::TypeNode *ParseUnionType(ir::TypeNode *firstType);
    ir::TypeNode *GetTypeAnnotationOfPrimitiveType(lexer::TokenType tokenType, TypeAnnotationParsingOptions *options);
    ir::TypeNode *ParseWildcardType(TypeAnnotationParsingOptions *options);
    ir::TypeNode *ParseFunctionType(TypeAnnotationParsingOptions *options);
    ir::TypeNode *ParseETSTupleType(TypeAnnotationParsingOptions *options);
    ir::TypeNode *CreateErrorForWrongArrayType(lexer::SourcePosition &startPos);
    ir::TypeNode *ParseTsArrayType(ir::TypeNode *typeNode, TypeAnnotationParsingOptions *options);
    bool ParseTriplePeriod(bool spreadTypePresent);
    std::string PrimitiveTypeToName(ir::PrimitiveType type) const;
    std::string GetNameForTypeNode(const ir::TypeNode *typeAnnotation) const;
    std::string GetNameForETSUnionType(const ir::TypeNode *typeAnnotation) const;
    ir::TSInterfaceDeclaration *ParseInterfaceBody(ir::Identifier *name, bool isStatic);
    bool IsArrowFunctionExpressionStart();
    bool IsValidTokenTypeOfArrowFunctionStart(lexer::TokenType tokenType);
    bool EatArrowFunctionParams(lexer::Lexer *lexer);
    bool IsPrimitiveType(const lexer::TokenType &tokenType);
    ir::ArrowFunctionExpression *ParseArrowFunctionExpression();
    void ReportIfVarDeclaration(VariableParsingFlags flags) override;
    ir::TypeNode *ParsePotentialFunctionalType(TypeAnnotationParsingOptions *options);
    std::pair<ir::TypeNode *, bool> ParseNonNullableType(TypeAnnotationParsingOptions *options);
    void CheckForConditionalTypeError(TypeAnnotationParsingOptions options, lexer::TokenType tokenType);
    std::pair<ir::TypeNode *, bool> GetTypeAnnotationFromToken(TypeAnnotationParsingOptions *options);
    std::pair<ir::TypeNode *, bool> GetTypeAnnotationFromParentheses(TypeAnnotationParsingOptions *options);
    ir::TypeNode *ParseLiteralIdent(TypeAnnotationParsingOptions *options);
    void ParseRightParenthesis(TypeAnnotationParsingOptions *options, ir::TypeNode *&typeAnnotation,
                               lexer::LexerPosition savedPos);
    ir::TypeNode *ParseTypeAnnotationNoPreferParam(TypeAnnotationParsingOptions *options);
    ir::TypeNode *ParseTypeAnnotation(TypeAnnotationParsingOptions *options) override;
    bool ParseReadonlyInTypeAnnotation();
    ir::TypeNode *ParseMultilineString();
    ir::TSTypeAliasDeclaration *ParseTypeAliasDeclaration() override;
    bool ValidateForInStatement() override;
    bool ValidAnnotationValue(ir::Expression *initializer);
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParseCoverParenthesizedExpressionAndArrowParameterList(
        ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    ir::Statement *ParseTryStatement() override;
    ir::Statement *ParseDebuggerStatement() override;
    ir::Statement *ParseDefaultIfSingleExport(ir::ModifierFlags modifiers);
    ir::ExportNamedDeclaration *CreateExportNamedDeclaration(const SpecifiersInfo &specs,
                                                             const ir::ModifierFlags &modifiers,
                                                             const lexer::SourcePosition &startLoc);
    ir::Statement *ParseExport(lexer::SourcePosition startLoc, ir::ModifierFlags modifiers);
    ir::Statement *ParseImportDeclaration(StatementParsingFlags flags) override;
    ir::Statement *ParseExportDeclaration(StatementParsingFlags flags) override;
    ir::AnnotatedExpression *ParseVariableDeclaratorKey(VariableParsingFlags flags) override;
    ir::Statement *ParseAnnotationsInStatement(StatementParsingFlags flags) override;
    ir::Statement *ParseInitModuleStatement() override;
    ir::VariableDeclarator *ParseVariableDeclarator(ir::Expression *init, lexer::SourcePosition startLoc,
                                                    VariableParsingFlags flags) override;
    ir::VariableDeclarator *ParseVariableDeclaratorInitializer(ir::Expression *init, VariableParsingFlags flags,
                                                               const lexer::SourcePosition &startLoc) override;
    bool IsFieldStartToken(lexer::TokenType t);
    void LookForOptionalMethod(char32_t &nextCp);
    ir::AstNode *ParseTypeLiteralOrInterfaceMember() override;
    ir::AstNode *ParseAnnotationsInInterfaceBody();
    void ParseNameSpaceSpecifier(ArenaVector<ir::AstNode *> *specifiers, bool isReExport = false);
    void CheckModuleAsModifier();
    bool IsFixedArrayTypeNode(ir::AstNode *node);
    ir::Expression *ParseFunctionParameterExpression(ir::AnnotatedExpression *paramIdent, bool isOptional);
    std::pair<ir::Expression *, std::string> TypeAnnotationValue(ir::TypeNode *typeAnnotation);
    ir::ETSParameterExpression *ParseFunctionParameterTail(ir::AnnotatedExpression *paramIdent, bool defaultUndefined);
    ir::Expression *ParseFunctionParameterAnnotations();
    ir::Expression *ParseFunctionParameter() override;
    ir::ThisExpression *ParseThisExpression() override;
    ir::Expression *ParseFunctionReceiver();
    ir::AnnotatedExpression *GetAnnotatedExpressionFromParam();
    ir::Expression *ResolveArgumentUnaryExpr(ExpressionParseFlags flags);
    ir::Expression *CreateUnaryExpressionFromArgument(ir::Expression *argument, lexer::TokenType operatorType,
                                                      char32_t beginningChar);

    ir::Expression *HandleDeepNesting();
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParseUnaryOrPrefixUpdateExpression(
        ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParsePropertyDefinition(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    ir::Expression *ParsePropertyKey(ExpressionParseFlags flags) override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParseDefaultPrimaryExpression(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS);
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParsePrimaryExpression(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    ir::Expression *ParsePrimaryExpressionWithLiterals(ExpressionParseFlags flags) override;
    ir::Expression *ParsePostPrimaryExpression(ir::Expression *primaryExpr, lexer::SourcePosition startLoc,
                                               bool ignoreCallExpression, bool *isChainExpression) override;
    ir::Expression *ParsePotentialAsExpression(ir::Expression *primaryExpr) override;
    bool ValidateLabeledStatement(lexer::TokenType type) override;
    ir::Expression *ParseCatchParam() override;
    void ParseCatchParamTypeAnnotation([[maybe_unused]] ir::AnnotatedExpression *param) override;
    ir::Expression *ParseSuperClassReference() override;
    ir::Identifier *ParseClassIdent(ir::ClassDefinitionModifiers modifiers) override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Statement *ParseClassStatement(StatementParsingFlags flags, ir::ClassDefinitionModifiers modifiers,
                                       ir::ModifierFlags modFlags = ir::ModifierFlags::NONE) override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Statement *ParseStructStatement(StatementParsingFlags flags, ir::ClassDefinitionModifiers modifiers,
                                        ir::ModifierFlags modFlags = ir::ModifierFlags::NONE) override;
    ir::Statement *ParseInterfaceStatement(StatementParsingFlags flags) override;
    ir::Statement *ParseEnumStatement(StatementParsingFlags flags) override;
    ir::Statement *ParseTypeAliasStatement() override;
    ir::AstNode *ParseClassElement(const ArenaVector<ir::AstNode *> &properties, ir::ClassDefinitionModifiers modifiers,
                                   ir::ModifierFlags flags) override;
    std::tuple<bool, bool, bool> HandleClassElementModifiers(ir::ModifierFlags &memberModifiers);
    void UpdateMemberModifiers(ir::ModifierFlags &memberModifiers, bool &seenStatic);
    ir::ModifierFlags ParseMemberAccessModifiers();
    template <bool IS_USAGE>
    ir::Expression *ParseAnnotationName();
    ir::AnnotationDeclaration *ParseAnnotationDeclaration(
        [[maybe_unused]] ir::ModifierFlags flags = ir::ModifierFlags::NONE);
    ir::AstNode *ParseAnnotationProperty(ir::Identifier *fieldName,
                                         ir::ModifierFlags memberModifiers = ir::ModifierFlags::NONE,
                                         bool needTypeAnnotation = true);
    ArenaVector<ir::AstNode *> ParseAnnotationProperties(ir::ModifierFlags memberModifiers = ir::ModifierFlags::NONE);
    ir::AnnotationUsage *ParseAnnotationUsage();
    bool TryParseAnnotations();
    void TryParseAnnotationsParams();
    bool IsAnnotationUsageStart(lexer::SourcePosition lastTokenEndIndex);
    ir::AstNode *ParseInnerTypeDeclaration(ir::ModifierFlags memberModifiers, lexer::LexerPosition savedPos,
                                           bool isStepToken, bool seenStatic);
    ir::AstNode *ParseInnerConstructorDeclaration(ir::ModifierFlags memberModifiers,
                                                  const lexer::SourcePosition &startLoc, bool isDefault);
    ir::AstNode *ParseInnerRest(const ArenaVector<ir::AstNode *> &properties, ir::ClassDefinitionModifiers modifiers,
                                ir::ModifierFlags memberModifiers, const lexer::SourcePosition &startLoc,
                                bool isDefault);
    bool CheckAccessorDeclaration(ir::ModifierFlags memberModifiers);
    ir::AstNode *ParseAmbientSignature(const lexer::SourcePosition &startPosAmbient);
    util::StringView ParseIndexName();
    std::pair<ir::TypeNode *, ir::TypeNode *> ParseIndexTypeAndReturnType();
    void ParseArgumentsNewExpression(ArenaVector<ir::Expression *> &arguments, ir::TypeNode *typeReference);
    ir::Identifier *CreateInvokeIdentifier();
    ir::Expression *ParseNewExpression() override;
    ir::Expression *ParseAsyncExpression();
    ir::Expression *ParseAwaitExpression();
    ir::Expression *ParseArrayOrDestructuringExpression(const ExpressionParseFlags flags);
    ir::ArrayExpression *ParseArrayExpression(ExpressionParseFlags flags) override;
    ir::TSTypeParameter *ParseTypeParameter(TypeAnnotationParsingOptions *options) override;
    bool IsStringEnum();
    ir::TSEnumDeclaration *ParseEnumMembers(ir::Identifier *key, const lexer::SourcePosition &enumStart, bool isConst,
                                            bool isStatic) override;
    ir::Expression *ParseEnumExpression();
    bool ParseNumberEnumHelper();
    lexer::SourcePosition ParseEnumMember(ArenaVector<ir::AstNode *> &members);
    ir::Statement *ParseInterfaceDeclaration(bool isStatic) override;
    ir::TypeNode *ParseThisType(TypeAnnotationParsingOptions *options);
    ir::Statement *ParseFunctionStatement(StatementParsingFlags flags) override;
    ir::OverloadDeclaration *ParseOverloadDeclaration(ir::ModifierFlags modifiers);
    ir::FunctionDeclaration *ParseFunctionDeclaration(bool canBeAnonymous, ir::ModifierFlags modifiers);
    ir::FunctionDeclaration *ParseTopLevelAccessor(ir::ModifierFlags modifiers);
    ir::TypeNode *ParseExtensionFunctionsTypeAnnotation();
    std::tuple<ir::Expression *, ir::TSTypeParameterInstantiation *> ParseClassImplementsElement() override;
    ir::TypeNode *ParseInterfaceExtendsElement() override;
    // NOLINTNEXTLINE(google-default-arguments)
    ir::ClassDefinition *ParseClassDefinition(ir::ClassDefinitionModifiers modifiers,
                                              ir::ModifierFlags flags = ir::ModifierFlags::NONE) override;
    bool CheckInNamespaceContextIsExported();
    ir::ETSModule *ParseNamespaceStatement(ir::ModifierFlags memberModifiers);
    ir::Statement *ParseNamespace(ir::ModifierFlags flags) override;
    ir::ETSModule *ParseNamespaceImp(ir::ModifierFlags flags);
    using NamespaceBody = std::tuple<ir::MethodDefinition *, ArenaVector<ir::AstNode *>, lexer::SourceRange>;
    NamespaceBody ParseNamespaceBody(ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags);
    // NOLINTNEXTLINE(google-default-arguments)
    ir::Statement *ParseEnumDeclaration(bool isConst = false, bool isStatic = false) override;
    ir::Statement *ParsePotentialConstEnum(VariableParsingFlags flags) override;
    void ValidateInstanceOfExpression(ir::Expression *expr);
    void ValidateRestParameter(ir::Expression *param) override;
    bool ValidateBreakLabel(util::StringView label) override;
    bool ValidateContinueLabel(util::StringView label) override;
    void CheckPredefinedMethods(ir::ScriptFunction const *function, const lexer::SourcePosition &position);
    bool CheckClassElement(ir::AstNode *property, ir::MethodDefinition *&ctor,
                           ArenaVector<ir::AstNode *> &properties) override;
    void CreateImplicitConstructor(ir::MethodDefinition *&ctor, ArenaVector<ir::AstNode *> &properties,
                                   ir::ClassDefinitionModifiers modifiers, ir::ModifierFlags flags,
                                   const lexer::SourcePosition &startLoc) override;
    bool ParsePotentialGenericFunctionCall(ir::Expression *primaryExpr, ir::Expression **returnExpression,
                                           const lexer::SourcePosition &startLoc, bool ignoreCallExpression) override;
    bool ParsePotentialNonNullExpression(ir::Expression **expression, lexer::SourcePosition startLoc) override;
    std::pair<ir::ModifierFlags, lexer::SourcePosition> ParseMemberModifiers();
    void SkipInvalidType() const;
    bool IsStructKeyword() const;

    bool IsExternal() const override
    {
        return util::Helpers::IsStdLib(GetProgram());
    }

    // NOLINTNEXTLINE(google-default-arguments)
    ir::Expression *ParseExpression(ExpressionParseFlags flags = ExpressionParseFlags::NO_OPTS) override;
    ir::Expression *ParseExpressionOrTypeAnnotation(lexer::TokenType type, ExpressionParseFlags flags) override;
    void ValidateKeywordIn();
    ir::Expression *ParsePotentialExpressionSequence(ir::Expression *expr, ExpressionParseFlags flags) override;
    ir::Expression *ParseGenericLambdaOrTypeAssertion();
    ir::ModifierFlags ParseTypeVarianceModifier(TypeAnnotationParsingOptions *const options);
    bool IsExportedDeclaration(ir::ModifierFlags memberModifiers);
    ir::Statement *ParseTopLevelDeclStatement(StatementParsingFlags flags);
    ir::Statement *ParseTopLevelStatement();
    void ParseTrailingBlock([[maybe_unused]] ir::CallExpression *callExpr) override;
    bool IsInOperatorTypeSupported() const override
    {
        return false;
    }
    void CheckDeclare();

    friend class ExternalSourceParser;
    friend class InnerSourceParser;

private:
    uint32_t namespaceNestedRank_;
    std::optional<ir::Expression *> GetPostPrimaryExpression(ir::Expression *returnExpression,
                                                             lexer::SourcePosition startLoc, bool ignoreCallExpression,
                                                             [[maybe_unused]] bool *isChainExpression);
    parser::Program *globalProgram_;
    std::vector<ir::AstNode *> insertingNodes_ {};
    std::unique_ptr<util::ImportPathManager> importPathManager_ {nullptr};
};

class ExternalSourceParser {
public:
    explicit ExternalSourceParser(ETSParser *parser, Program *newProgram);
    NO_COPY_SEMANTIC(ExternalSourceParser);
    NO_MOVE_SEMANTIC(ExternalSourceParser);

    ~ExternalSourceParser();

    void *operator new(size_t) = delete;
    void *operator new[](size_t) = delete;

private:
    ETSParser *parser_;
    Program *savedProgram_;
    lexer::Lexer *savedLexer_;
    varbinder::GlobalScope *savedTopScope_;
};

class SavedFormattingFileName {
public:
    SavedFormattingFileName(ETSParser *parser, std::string_view fname)
        : parser_(parser), savedFname_(parser->FormattingFileName())
    {
        parser->SetFormattingFileName(fname);
    }

    NO_COPY_SEMANTIC(SavedFormattingFileName);
    NO_MOVE_SEMANTIC(SavedFormattingFileName);

    ~SavedFormattingFileName()
    {
        parser_->SetFormattingFileName(savedFname_);
    }

private:
    ETSParser *parser_;
    std::string_view savedFname_;
};
}  // namespace ark::es2panda::parser
#endif
