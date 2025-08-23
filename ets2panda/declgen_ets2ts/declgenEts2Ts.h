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

#ifndef ES2PANDA_DECLGEN_ETS2TS_H
#define ES2PANDA_DECLGEN_ETS2TS_H

#include "parser/program/program.h"
#include "checker/ETSchecker.h"
#include "os/file.h"
#include "utils/arena_containers.h"
#include "util/options.h"
#include "util/diagnosticEngine.h"

namespace ark::es2panda::declgen_ets2ts {

struct DeclgenOptions {
    bool exportAll = false;
    bool isIsolatedDeclgen = false;
    std::string outputDeclEts;
    std::string outputEts;
    std::string recordFile;
};

// Consume program after checker stage and generate out_path typescript file with declarations
bool GenerateTsDeclarations(checker::ETSChecker *checker, const ark::es2panda::parser::Program *program,
                            const DeclgenOptions &declgenOptions);
bool ValidateDeclgenOptions(const DeclgenOptions &options, checker::ETSChecker *checker);
bool WriteOutputFiles(const DeclgenOptions &options, const std::string &combinedEts, const std::string &combinedDEts,
                      checker::ETSChecker *checker);

class TSDeclGen {
public:
    TSDeclGen(checker::ETSChecker *checker, const ark::es2panda::parser::Program *program)
        : checker_(checker),
          program_(program),
          diagnosticEngine_(checker->DiagnosticEngine()),
          allocator_(SpaceType::SPACE_TYPE_COMPILER, nullptr, true),
          indirectDependencyObjects_(allocator_.Adapter()),
          importSet_(allocator_.Adapter()),
          typeAliasMap_(allocator_.Adapter()),
          paramDefaultMap_(allocator_.Adapter())
    {
    }

    void SetDeclgenOptions(const DeclgenOptions &options)
    {
        declgenOptions_ = options;
    }

    const DeclgenOptions &GetDeclgenOptions()
    {
        return declgenOptions_;
    }

    bool Generate();
    void GenImportDeclarations();
    void GenImportRecordDeclarations(const std::string &source);

    std::string GetDtsOutput() const
    {
        return outputDts_.str();
    }

    std::string GetTsOutput() const
    {
        return outputTs_.str();
    }

    void ResetDtsOutput()
    {
        outputDts_.str("");
    }

    void ResetTsOutput()
    {
        outputTs_.str("");
    }

    static constexpr std::string_view INDENT = "    ";

private:
    void LogError(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params,
                  const lexer::SourcePosition &pos);
    void LogWarning(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params,
                    const lexer::SourcePosition &pos);

    const ir::Identifier *GetKeyIdent(const ir::Expression *key);
    const checker::Signature *GetFuncSignature(const checker::ETSFunctionType *etsFunctionType,
                                               const ir::MethodDefinition *methodDef);

    void SplitUnionTypes(std::string &unionTypeString);
    void GenType(const checker::Type *checkerType);
    void GenFunctionType(const checker::ETSFunctionType *functionType, const ir::MethodDefinition *methodDef = nullptr);
    void ProcessFunctionReturnType(const checker::Signature *sig);
    bool ProcessTSQualifiedName(const ir::ETSTypeReference *typeReference);
    void ProcessETSTypeReferenceType(const ir::ETSTypeReference *typeReference,
                                     const checker::Type *checkerType = nullptr);
    bool ProcessTypeAnnotationSpecificTypes(const checker::Type *checkerType);
    void ProcessTypeAnnotationType(const ir::TypeNode *typeAnnotation, const checker::Type *checkerType = nullptr);
    void ProcessETSTypeReference(const ir::TypeNode *typeAnnotation, const checker::Type *checkerType);
    void ProcessETSTuple(const ir::ETSTuple *etsTuple);
    void ProcessETSUnionType(const ir::ETSUnionType *etsUnionType);
    void ProcessTSArrayType(const ir::TSArrayType *tsArrayType);
    void ProcessETSFunctionType(const ir::ETSFunctionType *etsFunction);

    void GenObjectType(const checker::ETSObjectType *objectType);
    void GenUnionType(const checker::ETSUnionType *unionType);
    void GenTupleType(const checker::ETSTupleType *tupleType);

    template <class UnionType>
    std::vector<UnionType *> FilterUnionTypes(const ArenaVector<UnionType *> &originTypes);

    void GenImportDeclaration(const ir::ETSImportDeclaration *importDeclaration);
    void GenNamespaceImport(const ir::AstNode *specifier, const std::string &source);
    void GenDefaultImport(const ir::AstNode *specifier, const std::string &source, bool isTypeKind = false);
    void GenNamedImports(const ir::ETSImportDeclaration *importDeclaration,
                         const ArenaVector<ir::AstNode *> &specifiers, bool isTypeKind = false);
    void GenDtsImportStatement(std::vector<ir::AstNode *> &specifiers,
                               const ir::ETSImportDeclaration *importDeclaration, bool isTypeKind = false);
    void GenTsImportStatement(std::vector<ir::AstNode *> &specifiers, const ir::ETSImportDeclaration *importDeclaration,
                              bool isInterface = false);
    void GenSingleNamedImport(ir::AstNode *specifier, const ir::ETSImportDeclaration *importDeclaration,
                              bool isGlueCode = false);
    void GenReExportDeclaration(const ir::ETSReExportDeclaration *reExportDeclaration);
    bool GenNamespaceReExportDeclaration(const ir::AstNode *specifier,
                                         const ir::ETSImportDeclaration *importDeclaration);
    void SeparateInterfaceSpecifiers(const ArenaVector<ir::AstNode *> &specifiers,
                                     std::vector<ir::AstNode *> &interfaceSpecifiers,
                                     std::vector<ir::AstNode *> &normalSpecifiers);
    void GenDtsReExportStatement(const ArenaVector<ir::AstNode *> &specifiers,
                                 const ir::ETSImportDeclaration *importDeclaration, bool isTypeKind = false);
    void GenTsReExportStatement(const std::vector<ir::AstNode *> &specifiers,
                                const ir::ETSImportDeclaration *importDeclaration, bool isInterface = false);
    void GenSingleNamedReExport(ir::AstNode *specifier, const ir::ETSImportDeclaration *importDeclaration,
                                bool isGlueCode = false);
    void GenTypeAliasDeclaration(const ir::TSTypeAliasDeclaration *typeAlias);
    void GenEnumDeclaration(const ir::ClassProperty *enumMember);
    void GenInterfaceDeclaration(const ir::TSInterfaceDeclaration *interfaceDecl);
    bool GenInterfaceProp(const ir::MethodDefinition *methodDef);
    void GenClassDeclaration(const ir::ClassDeclaration *classDecl);
    void GenMethodDeclaration(const ir::MethodDefinition *methodDef);
    bool GenMethodDeclarationPrefix(const ir::MethodDefinition *methodDef, const ir::Identifier *methodIdent,
                                    const std::string &methodName);
    void GenMethodSignature(const ir::MethodDefinition *methodDef, const ir::Identifier *methodIdent,
                            const std::string &methodName);
    void GenPropDeclaration(const ir::ClassProperty *classProp);
    void ProcessClassPropDeclaration(const ir::ClassProperty *classProp);
    void GenPropAccessor(const ir::ClassProperty *classProp, const std::string &accessorKind);
    void GenGlobalVarDeclaration(const ir::ClassProperty *globalVar);
    void GenLiteral(const ir::Literal *literal);

    template <class T>
    void GenModifier(const T *node, bool isProp = false);

    template <class T>
    void GenAnnotations(const T *node);
    void GenAnnotationProperties(const ir::AnnotationUsage *anno);
    void GenAnnotationPropertyValue(ir::Expression *value);

    void GenTypeParameters(const ir::TSTypeParameterDeclaration *typeParams);
    void GenExport(const ir::Identifier *symbol);
    void GenExport(const ir::Identifier *symbol, const std::string &alias);
    void GenDefaultExport(const ir::Identifier *symbol);
    bool ShouldEmitDeclarationSymbol(const ir::Identifier *symbol);

    template <class T, class CB>
    void GenSeparated(const T &container, const CB &cb, const char *separator = ", ", bool isReExport = false,
                      bool isDtsExport = true);

    void PrepareClassDeclaration(const ir::ClassDefinition *classDef);
    bool ShouldSkipMethodDeclaration(const ir::MethodDefinition *methodDef);
    bool ShouldSkipClassDeclaration(const std::string_view &className) const;
    void HandleClassDeclarationTypeInfo(const ir::ClassDefinition *classDef, const std::string_view &className);
    void HandleClassInherit(const ir::Expression *expr);
    void ProcessClassBody(const ir::ClassDefinition *classDef);
    void ProcessParamDefaultToMap(const ir::Statement *stmt);
    void ProcessParameterName(varbinder::LocalVariable *param);
    void ProcessFuncParameter(varbinder::LocalVariable *param);
    void ProcessFuncParameters(const checker::Signature *sig);
    void ProcessClassPropertyType(const ir::ClassProperty *classProp);
    std::vector<ir::AstNode *> FilterValidImportSpecifiers(const ArenaVector<ir::AstNode *> &specifiers);
    std::string ReplaceETSGLOBAL(const std::string &typeName);
    std::string GetIndent() const;
    std::string RemoveModuleExtensionName(const std::string &filepath);
    void GenPartName(std::string &partName);
    void ProcessIndent();

    bool GenGlobalDescriptor();
    void CollectIndirectExportDependencies();
    void ProcessTypeAliasDependencies(const ir::TSTypeAliasDeclaration *typeAliasDecl);
    void ProcessTypeAnnotationDependencies(const ir::TypeNode *typeAnnotation);
    void ProcessClassDependencies(const ir::ClassDeclaration *classDecl);
    void ProcessClassPropDependencies(const ir::ClassDefinition *classDef);
    void ProcessClassMethodDependencies(const ir::MethodDefinition *methodDef);
    void ProcessInterfaceDependencies(const ir::TSInterfaceDeclaration *interfaceDecl);
    void ProcessInterfacePropDependencies(const ir::TSInterfaceDeclaration *interfaceDecl);
    void ProcessInterfaceMethodDependencies(const ir::MethodDefinition *methodDef);
    void ProcessETSTypeReferenceDependencies(const ir::ETSTypeReference *typeReference);
    void AddSuperType(const ir::Expression *super);
    void AddSuperType(const checker::Type *tsType);
    void ProcessInterfacesDependencies(const ArenaVector<checker::ETSObjectType *> &interfaces);
    void AddObjectDependencies(const util::StringView &typeName, const std::string &alias = "");
    void GenDeclarations();
    void CloseClassBlock(const bool isDts);

    void EmitDeclarationPrefix(const ir::ClassDefinition *classDef, const std::string &typeName,
                               const std::string_view &className);
    void EmitClassDeclaration(const ir::ClassDefinition *classDef, const std::string_view &className);
    void EmitClassGlueCode(const ir::ClassDefinition *classDef, const std::string &className);
    void EmitMethodGlueCode(const std::string &methodName, const ir::Identifier *methodIdentifier);
    void EmitPropGlueCode(const ir::ClassProperty *classProp, const std::string &propName);

    bool HandleBasicTypes(const checker::Type *checkerType);
    void HandleFunctionType(const checker::Type *checkerType);
    bool HandleETSSpecificTypes(const checker::Type *checkerType);
    bool HandleObjectType(const checker::Type *checkerType);
    bool HandleSpecificObjectTypes(const checker::ETSObjectType *objectType);
    void HandleArrayType(const checker::Type *checkerType);
    void HandleTypeArgument(checker::Type *arg, const std::string &typeStr);

    void ProcessInterfaceBody(const ir::TSInterfaceBody *body);
    void ProcessInterfaceMethodDefinition(const ir::MethodDefinition *methodDef);
    void ProcessMethodDefinition(const ir::MethodDefinition *methodDef,
                                 std::unordered_set<std::string> &processedMethods);

    void ProcessMethodsFromInterfaces(std::unordered_set<std::string> &processedMethods,
                                      const ArenaVector<checker::ETSObjectType *> &interfaces);

    void OutDts() {}

    template <class F, class... T>
    void OutDts(F &&first, T &&...rest)
    {
        outputDts_ << first;
        OutDts(std::forward<T>(rest)...);
    }

    void OutTs() {}

    template <class F, class... T>
    void OutTs(F &&first, T &&...rest)
    {
        outputTs_ << first;
        OutTs(std::forward<T>(rest)...);
    }

    void OutEndlDts(const std::size_t count = 1)
    {
        ark::os::file::File::GetEndLine(outputDts_, count);
    }

    void OutEndlTs(const std::size_t count = 1)
    {
        ark::os::file::File::GetEndLine(outputTs_, count);
    }

    void ResetState()
    {
        state_ = GenState();
    }

    void ResetClassNode()
    {
        classNode_.isStruct = false;
        classNode_.isIndirect = false;
    }

    struct GenState {
        const ir::Expression *super {nullptr};
        bool inInterface {false};
        bool inGlobalClass {false};
        bool inClass {false};
        bool inNamespace {false};
        bool inEnum {false};
        bool isClassInNamespace {false};
        bool isInterfaceInNamespace {false};
        bool isDeclareNamespace {false};
        std::string currentClassDescriptor {};
        std::stack<bool> inUnionBodyStack {};
        std::string currentTypeAliasName;
        const ir::TSTypeParameterDeclaration *currentTypeParams {nullptr};
    } state_ {};

    struct ClassNode {
        bool hasNestedClass {false};
        bool isIndirect {false};
        bool isStruct {false};
        size_t indentLevel {1};
    } classNode_ {};

    const std::unordered_set<std::string_view> numberTypes_ = {"Long",  "Float", "Double", "Byte",
                                                               "Short", "Int",   "Number"};
    const std::unordered_set<std::string_view> stringTypes_ = {"Char", "String"};
    const std::unordered_set<std::string_view> annotationList_ = {
        "Component", "Builder",    "LocalBuilder", "BuilderParam", "Styles",     "Extend",   "AnimatableExtend",
        "Require",   "Reusable",   "State",        "Prop",         "Link",       "Provide",  "Consume",
        "Observed",  "ObjectLink", "Watch",        "Track",        "ObservedV2", "Trace",    "ComponentV2",
        "Local",     "Param",      "Once",         "Event",        "Provider",   "Consumer", "Monitor",
        "Computed",  "Type"};
    const std::unordered_set<std::string_view> stdlibNamespaceList_ = {
        "StdProcess", "taskpool", "functions",    "containers", "Intl",     "GC",
        "jsonx",      "proxy",    "unsafeMemory", "reflect",    "StdDebug", "arktest"};
    const std::set<std::string> extensions_ = {".sts", ".ets", ".ts", ".js"};

    std::stringstream outputDts_;
    std::stringstream outputTs_;
    checker::ETSChecker *checker_ {};
    const ark::es2panda::parser::Program *program_ {};
    util::DiagnosticEngine &diagnosticEngine_;
    ArenaAllocator allocator_;
    ArenaSet<std::string> indirectDependencyObjects_;
    ArenaSet<std::string> importSet_;
    DeclgenOptions declgenOptions_ {};
    std::string globalDesc_;
    ArenaMap<std::string, std::string> typeAliasMap_;
    ArenaMap<util::StringView, util::StringView> paramDefaultMap_;
};
}  // namespace ark::es2panda::declgen_ets2ts

#endif  // ES2PANDA_DECLGEN_ETS2TS_H
