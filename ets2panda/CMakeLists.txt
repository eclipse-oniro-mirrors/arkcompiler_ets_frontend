# Copyright (c) 2021-2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required (VERSION 3.5.0)

include(cmake/coverage.cmake)
include(cmake/PandaCmakeFunctions.cmake)

project (es2panda)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----- Aliases ----------------------------------------------------------------

add_custom_target(frontend_bins COMMENT "Build all common frontend binaries")
add_dependencies(frontend_bins
  es2panda
  es2panda-lib
  es2panda-public
  dependency_analyzer
  dependency_analyzer_lib
  declgen_ets2ts
)

set(ES2PANDA_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ES2PANDA_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR})

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

if(PANDA_TARGET_WINDOWS)
    set(DELIM "\\\\")
    string(REPLACE "/" "\\\\" STATIC_CORE ${PANDA_ROOT})
else()
    set(STATIC_CORE ${PANDA_ROOT})
    set(DELIM "/")
endif()

set(GENERATED_DIR ${OUTPUT_DIR}/generated)
set(GENERATED_STAMP ${OUTPUT_DIR}/gen_dir.stamp)
if(CMAKE_CROSSCOMPILING)
    ExternalProject_Get_Property(panda_host_tools binary_dir)
    set(DEFAULT_ARKTSCONFIG    "${binary_dir}/tools/es2panda/aot")
else()
    set(DEFAULT_ARKTSCONFIG "${CMAKE_BINARY_DIR}/bin")
endif()
file(MAKE_DIRECTORY "${GENERATED_DIR}")
if(PANDA_WITH_ETS)
  file(WRITE "${GENERATED_DIR}/arktsconfig.json"
      "{\n"
      "  \"compilerOptions\": {\n"
      "    \"baseUrl\": \"${PANDA_ROOT}\",\n"
      "    \"paths\": {\n"
      "      \"std\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}stdlib${DELIM}std\"],\n"
      "      \"escompat\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}stdlib${DELIM}escompat\"],\n"
      "      \"@ohos.buffer\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.buffer.ets\"],\n"
      "      \"@ohos.util.ArrayList\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.ArrayList.ets\"],\n"
      "      \"@ohos.util.Deque\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.Deque.ets\"],\n"
      "      \"@ohos.util.HashMap\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.HashMap.ets\"],\n"
      "      \"@ohos.util.HashSet\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.HashSet.ets\"],\n"
      "      \"@ohos.util.json\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.json.ets\"],\n"
      "      \"@ohos.util.LightWeightMap\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.LightWeightMap.ets\"],\n"
      "      \"@ohos.util.LightWeightSet\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.LightWeightSet.ets\"],\n"
      "      \"@ohos.util.LinkedList\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.LinkedList.ets\"],\n"
      "      \"@ohos.util.List\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.List.ets\"],\n"
      "      \"@ohos.util.PlainArray\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.PlainArray.ets\"],\n"
      "      \"@ohos.util.Queue\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.Queue.ets\"],\n"
      "      \"@ohos.util.Stack\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.Stack.ets\"],\n"
      "      \"@ohos.util.stream\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.stream.ets\"],\n"
      "      \"@ohos.util\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.ets\"],\n"
      "      \"@ohos.util.TreeMap\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.TreeMap.ets\"],\n"
      "      \"@ohos.util.TreeSet\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.util.TreeSet.ets\"],\n"
      "      \"@ohos.uri\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.uri.ets\"],\n"
      "      \"@ohos.url\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.url.ets\"],\n"
      "      \"@ohos.xml\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.xml.ets\"],\n"
      "      \"@ohos.base\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}api${DELIM}@ohos.base.ets\"],\n"
      "      \"@arkts.math.Decimal\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}arkts${DELIM}@arkts.math.Decimal.ets\"],\n"
      "      \"@arkts.collections\": [\"${STATIC_CORE}${DELIM}plugins${DELIM}ets${DELIM}sdk${DELIM}arkts${DELIM}@arkts.collections.ets\"],\n"
      "      \"import_tests\": [\"${CMAKE_CURRENT_SOURCE_DIR}/test/parser/ets/import_tests\"]\n"
      "    },\n"
      "    \"dynamicPaths\": {\n"
      "      \"dynamic_import_tests\": {\"language\": \"js\", \"ohmUrl\": \"dynamic_import_tests\"},\n"
      "      \"dynamic_import_tests/modules/instanceof\": {\"language\": \"js\", \"declPath\": \"${CMAKE_CURRENT_SOURCE_DIR}/test/parser/ets/dynamic_import_tests/modules/instanceof.ets\", \"ohmUrl\": \"${CMAKE_CURRENT_SOURCE_DIR}/test/parser/ets/dynamic_import_tests/modules/instanceof.ets\"},\n"
      "      \"dynamic_import_tests/modules/module\": {\"language\": \"js\", \"declPath\": \"${CMAKE_CURRENT_SOURCE_DIR}/test/parser/ets/dynamic_import_tests/modules/module.ets\", \"ohmUrl\": \"${CMAKE_CURRENT_SOURCE_DIR}/test/parser/ets/dynamic_import_tests/modules/module.ets\"}\n"
      "    }\n"
      "  }\n"
      "}\n"
  )
  MESSAGE("Generated default arktsconfig to ${GENERATED_DIR}/arktsconfig.json")
  file(COPY "${GENERATED_DIR}/arktsconfig.json" DESTINATION "${DEFAULT_ARKTSCONFIG}/")
  file(COPY "${GENERATED_DIR}/arktsconfig.json" DESTINATION "${CMAKE_BINARY_DIR}/bin")
  file(COPY "${GENERATED_DIR}/arktsconfig.json" DESTINATION "${CMAKE_BINARY_DIR}/bin-gtests")
endif()

# add check for PANDA_PRODUCT_BUILD after normal version tracking will be implemented
execute_process(
  COMMAND date "+%Y-%m-%d_%H:%M:%S"
  OUTPUT_VARIABLE CURRENT_DATE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND git rev-parse HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE LAST_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions(-DES2PANDA_DATE="${CURRENT_DATE}")
add_definitions(-DES2PANDA_HASH="${LAST_COMMIT_HASH}")


add_custom_command(
  OUTPUT ${GENERATED_STAMP}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
  COMMAND ${CMAKE_COMMAND} -E touch ${GENERATED_STAMP}
)

set(TEMPLATES
  isa.h.erb
  formats.h.erb
)

panda_isa_gen(
  TEMPLATES ${TEMPLATES}
  SOURCE ${CMAKE_CURRENT_LIST_DIR}/compiler/templates
  DESTINATION ${GENERATED_DIR}
  EXTRA_DEPENDENCIES ${GENERATED_STAMP}
)

panda_gen(
    DATA ${CMAKE_CURRENT_SOURCE_DIR}/util/options.yaml
    TARGET_NAME es2panda_options_gen
    TEMPLATES options.h.erb
    SOURCE ${PANDA_ROOT}/templates/options
    DESTINATION ${GENERATED_DIR}
    API ${PANDA_ROOT}/templates/common.rb
)

set(DIAGNOSTIC_DIR
   ${CMAKE_CURRENT_SOURCE_DIR}/util/diagnostic/
)

panda_gen(
    DATA
        ${DIAGNOSTIC_DIR}/syntax.yaml
        ${DIAGNOSTIC_DIR}/semantic.yaml
        ${DIAGNOSTIC_DIR}/warning.yaml
        ${DIAGNOSTIC_DIR}/fatal.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/declgen_ets2ts/declgen_ets2ts_error.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/declgen_ets2ts/declgen_ets2ts_warning.yaml
        ${DIAGNOSTIC_DIR}/arktsconfig_error.yaml
        ${DIAGNOSTIC_DIR}/isolated_declgen.yaml
    TARGET_NAME es2panda_diagnostic_gen
    TEMPLATES diagnostic.h.erb
    SOURCE ${DIAGNOSTIC_DIR}
    DESTINATION ${GENERATED_DIR}
    API
        ${DIAGNOSTIC_DIR}/diagnostic.rb
        ${DIAGNOSTIC_DIR}/diagnostic.rb
        ${DIAGNOSTIC_DIR}/diagnostic.rb
        ${DIAGNOSTIC_DIR}/diagnostic.rb
        ${DIAGNOSTIC_DIR}/diagnostic.rb
        ${DIAGNOSTIC_DIR}/diagnostic.rb
        ${DIAGNOSTIC_DIR}/diagnostic.rb
        ${DIAGNOSTIC_DIR}/diagnostic.rb
)

panda_gen(
    TARGET_NAME es2panda_keywords
    TEMPLATES
        keywords.h.erb
        token.inl.erb
        tokenType.h.erb
    DATA
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/scripts/keywords.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/scripts/tokens.yaml
    API
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/scripts/keywords.rb
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/scripts/tokens.rb
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lexer/templates
    DESTINATION ${GENERATED_DIR}
)

panda_gen(
    DATA ${CMAKE_CURRENT_SOURCE_DIR}/compiler/scripts/signatures.yaml
    TARGET_NAME es2panda_signatures
    TEMPLATES signatures.h.erb
    API
       ${CMAKE_CURRENT_SOURCE_DIR}/compiler/scripts/signatures.rb
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compiler/templates
    DESTINATION ${GENERATED_DIR}
)

set(ES2PANDA_LIB_SRC
  ast_verifier/ASTVerifier.cpp
  ast_verifier/helpers.cpp
  ast_verifier/invariants/arithmeticOperationValid.cpp
  ast_verifier/invariants/checkAbstractMethod.cpp
  ast_verifier/invariants/checkConstProperties.cpp
  ast_verifier/invariants/checkScopeDeclaration.cpp
  ast_verifier/invariants/checkStructDeclaration.cpp
  ast_verifier/invariants/everyChildHasValidParent.cpp
  ast_verifier/invariants/everyChildInParentRange.cpp
  ast_verifier/invariants/getterSetterValidation.cpp
  ast_verifier/invariants/identifierHasVariable.cpp
  ast_verifier/invariants/importExportAccessValid.cpp
  ast_verifier/invariants/nodeHasParent.cpp
  ast_verifier/invariants/nodeHasSourceRange.cpp
  ast_verifier/invariants/nodeHasType.cpp
  ast_verifier/invariants/referenceTypeAnnotationIsNull.cpp
  ast_verifier/invariants/forLoopCorrectlyInitialized.cpp
  ast_verifier/invariants/modifierAccessValid.cpp
  ast_verifier/invariants/sequenceExpressionHasLastType.cpp
  ast_verifier/invariants/variableHasEnclosingScope.cpp
  ast_verifier/invariants/variableHasScope.cpp
  ast_verifier/invariants/variableNameIdentifierNameSame.cpp
  es2panda.cpp
  varbinder/ASBinder.cpp
  varbinder/TSBinder.cpp
  varbinder/TypedBinder.cpp
  varbinder/ETSBinder.cpp
  varbinder/JSBinder.cpp
  varbinder/varbinder.cpp
  varbinder/declaration.cpp
  varbinder/recordTable.cpp
  varbinder/scope.cpp
  varbinder/variable.cpp
  compiler/base/catchTable.cpp
  compiler/base/condition.cpp
  compiler/base/destructuring.cpp
  compiler/base/hoisting.cpp
  compiler/base/iterators.cpp
  compiler/base/lexenv.cpp
  compiler/base/literals.cpp
  compiler/base/lreference.cpp
  compiler/base/optionalChain.cpp
  compiler/core/CFG.cpp
  compiler/core/codeGen.cpp
  compiler/core/compileJob.cpp
  compiler/core/compileQueue.cpp
  compiler/core/compilerImpl.cpp
  compiler/core/dynamicContext.cpp
  compiler/core/emitter.cpp
  compiler/core/JSCompiler.cpp
  compiler/core/JSCompilerUnreachable.cpp
  compiler/core/JSemitter.cpp
  compiler/core/envScope.cpp
  compiler/core/function.cpp
  compiler/core/labelTarget.cpp
  compiler/core/moduleContext.cpp
  compiler/core/pandagen.cpp
  compiler/core/programElement.cpp
  compiler/core/regAllocator.cpp
  compiler/core/regScope.cpp
  compiler/core/regSpiller.cpp
  compiler/core/ETSCompiler.cpp
  compiler/core/ETSCompilerUnrechable.cpp
  compiler/core/ETSemitter.cpp
  compiler/core/ETSGen.cpp
  compiler/core/ETSfunction.cpp
  compiler/core/switchBuilder.cpp
  compiler/core/targetTypeContext.cpp
  compiler/core/vReg.cpp
  compiler/debugger/debuginfoDumper.cpp
  compiler/function/asyncFunctionBuilder.cpp
  compiler/function/asyncGeneratorFunctionBuilder.cpp
  compiler/function/functionBuilder.cpp
  compiler/function/generatorFunctionBuilder.cpp
  compiler/lowering/checkerPhase.cpp
  compiler/lowering/scopesInit/savedBindingsCtx.cpp
  compiler/lowering/scopesInit/scopesInitPhase.cpp
  compiler/lowering/phase.cpp
  compiler/lowering/plugin_phase.cpp
  compiler/lowering/resolveIdentifiers.cpp
  compiler/lowering/util.cpp
  compiler/lowering/ets/topLevelStmts/importExportDecls.cpp
  compiler/lowering/ets/topLevelStmts/globalClassHandler.cpp
  compiler/lowering/ets/topLevelStmts/globalDeclTransformer.cpp
  compiler/lowering/ets/topLevelStmts/topLevelStmts.cpp
  compiler/lowering/ets/expressionLambdaLowering.cpp
  compiler/lowering/ets/extensionAccessorLowering.cpp
  compiler/lowering/ets/genericBridgesLowering.cpp
  compiler/lowering/ets/arrayLiteralLowering.cpp
  compiler/lowering/ets/boxedTypeLowering.cpp
  compiler/lowering/ets/boxingForLocals.cpp
  compiler/lowering/ets/capturedVariables.cpp
  compiler/lowering/ets/cfgBuilderPhase.cpp
  compiler/lowering/ets/constantExpressionLowering.cpp
  compiler/lowering/ets/convertPrimitiveCastMethodCall.cpp
  compiler/lowering/ets/declareOverloadLowering.cpp
  compiler/lowering/ets/defaultParametersInConstructorLowering.cpp
  compiler/lowering/ets/defaultParametersLowering.cpp
  compiler/lowering/ets/dynamicImportLowering.cpp
  compiler/lowering/ets/exportAnonymousConst.cpp
  compiler/lowering/ets/lateInitialization.cpp
  compiler/lowering/ets/lambdaLowering.cpp
  compiler/lowering/ets/restTupleLowering.cpp
  compiler/lowering/ets/spreadLowering.cpp
  compiler/lowering/ets/objectIndexAccess.cpp
  compiler/lowering/ets/objectIterator.cpp
  compiler/lowering/ets/insertOptionalParametersAnnotation.cpp
  compiler/lowering/ets/interfacePropertyDeclarations.cpp
  compiler/lowering/ets/opAssignment.cpp
  compiler/lowering/ets/ambientLowering.cpp
  compiler/lowering/ets/asyncMethodLowering.cpp
  compiler/lowering/ets/bigintLowering.cpp
  compiler/lowering/ets/recordLowering.cpp
  compiler/lowering/ets/resizableArrayLowering.cpp
  compiler/lowering/ets/restArgsLowering.cpp
  compiler/lowering/ets/unionLowering.cpp
  compiler/lowering/ets/optionalArgumentsLowering.cpp
  compiler/lowering/ets/optionalLowering.cpp
  compiler/lowering/ets/expandBrackets.cpp
  compiler/lowering/ets/packageImplicitImport.cpp
  compiler/lowering/ets/partialExportClassGen.cpp
  compiler/lowering/ets/promiseVoid.cpp
  compiler/lowering/ets/objectLiteralLowering.cpp
  compiler/lowering/ets/interfaceObjectLiteralLowering.cpp
  compiler/lowering/ets/stringComparison.cpp
  compiler/lowering/ets/stringConstantsLowering.cpp
  compiler/lowering/ets/stringConstructorLowering.cpp
  compiler/lowering/ets/typeFromLowering.cpp
  compiler/lowering/ets/enumLowering.cpp
  compiler/lowering/ets/enumPostCheckLowering.cpp
  compiler/lowering/ets/setJumpTarget.cpp
  ir/astDump.cpp
  ir/srcDump.cpp
  ir/astNode.cpp
  ir/astNodeHistory.cpp
  ir/irnode.cpp
  ir/typeNode.cpp
  ir/opaqueTypeNode.cpp
  ir/brokenTypeNode.cpp
  ir/base/catchClause.cpp
  ir/base/classElement.cpp
  ir/base/classDefinition.cpp
  ir/base/classProperty.cpp
  ir/base/classStaticBlock.cpp
  ir/base/decorator.cpp
  ir/base/metaProperty.cpp
  ir/base/methodDefinition.cpp
  ir/base/property.cpp
  ir/base/scriptFunction.cpp
  ir/base/scriptFunctionSignature.cpp
  ir/base/spreadElement.cpp
  ir/base/templateElement.cpp
  ir/base/tsIndexSignature.cpp
  ir/base/tsMethodSignature.cpp
  ir/base/tsPropertySignature.cpp
  ir/base/tsSignatureDeclaration.cpp
  ir/expression.cpp
  ir/expressions/arrayExpression.cpp
  ir/expressions/arrowFunctionExpression.cpp
  ir/expressions/assignmentExpression.cpp
  ir/expressions/awaitExpression.cpp
  ir/expressions/binaryExpression.cpp
  ir/expressions/blockExpression.cpp
  ir/expressions/callExpression.cpp
  ir/expressions/chainExpression.cpp
  ir/expressions/classExpression.cpp
  ir/expressions/conditionalExpression.cpp
  ir/expressions/directEvalExpression.cpp
  ir/expressions/functionExpression.cpp
  ir/expressions/identifier.cpp
  ir/expressions/dummyNode.cpp
  ir/expressions/importExpression.cpp
  ir/expressions/literal.cpp
  ir/expressions/literals/bigIntLiteral.cpp
  ir/expressions/literals/booleanLiteral.cpp
  ir/expressions/literals/charLiteral.cpp
  ir/expressions/literals/nullLiteral.cpp
  ir/expressions/literals/numberLiteral.cpp
  ir/expressions/literals/regExpLiteral.cpp
  ir/expressions/literals/stringLiteral.cpp
  ir/expressions/literals/undefinedLiteral.cpp
  ir/expressions/memberExpression.cpp
  ir/expressions/newExpression.cpp
  ir/expressions/objectExpression.cpp
  ir/expressions/omittedExpression.cpp
  ir/expressions/sequenceExpression.cpp
  ir/expressions/superExpression.cpp
  ir/expressions/taggedTemplateExpression.cpp
  ir/expressions/templateLiteral.cpp
  ir/expressions/thisExpression.cpp
  ir/expressions/typeofExpression.cpp
  ir/expressions/unaryExpression.cpp
  ir/expressions/updateExpression.cpp
  ir/expressions/yieldExpression.cpp
  ir/module/exportAllDeclaration.cpp
  ir/module/exportDefaultDeclaration.cpp
  ir/module/exportNamedDeclaration.cpp
  ir/module/exportSpecifier.cpp
  ir/module/importDeclaration.cpp
  ir/module/importDefaultSpecifier.cpp
  ir/module/importNamespaceSpecifier.cpp
  ir/module/importSpecifier.cpp
  ir/statement.cpp
  ir/statements/annotationDeclaration.cpp
  ir/statements/annotationUsage.cpp
  ir/statements/assertStatement.cpp
  ir/statements/blockStatement.cpp
  ir/statements/breakStatement.cpp
  ir/statements/classDeclaration.cpp
  ir/ets/etsStructDeclaration.cpp
  ir/ets/etsReExportDeclaration.cpp
  ir/statements/continueStatement.cpp
  ir/statements/debuggerStatement.cpp
  ir/statements/doWhileStatement.cpp
  ir/statements/emptyStatement.cpp
  ir/statements/expressionStatement.cpp
  ir/statements/forInStatement.cpp
  ir/statements/forOfStatement.cpp
  ir/statements/forUpdateStatement.cpp
  ir/statements/functionDeclaration.cpp
  ir/statements/ifStatement.cpp
  ir/statements/labelledStatement.cpp
  ir/statements/loopStatement.cpp
  ir/statements/returnStatement.cpp
  ir/statements/switchCaseStatement.cpp
  ir/statements/switchStatement.cpp
  ir/statements/throwStatement.cpp
  ir/statements/tryStatement.cpp
  ir/statements/variableDeclaration.cpp
  ir/statements/variableDeclarator.cpp
  ir/statements/whileStatement.cpp
  ir/as/namedType.cpp
  ir/as/prefixAssertionExpression.cpp
  ir/ets/etsClassLiteral.cpp
  ir/ets/etsFunctionType.cpp
  ir/ets/etsKeyofType.cpp
  ir/ets/etsNewArrayInstanceExpression.cpp
  ir/ets/etsNewClassInstanceExpression.cpp
  ir/ets/etsNewMultiDimArrayInstanceExpression.cpp
  ir/ets/etsPackageDeclaration.cpp
  ir/ets/etsParameterExpression.cpp
  ir/ets/etsPrimitiveType.cpp
  ir/ets/etsNonNullishTypeNode.cpp
  ir/ets/etsNullishTypes.cpp
  ir/ets/etsNeverType.cpp
  ir/ets/etsModule.cpp
  ir/ets/etsStringLiteralType.cpp
  ir/ets/etsTuple.cpp
  ir/ets/etsTypeReference.cpp
  ir/ets/etsTypeReferencePart.cpp
  ir/ets/etsUnionType.cpp
  ir/ets/etsWildcardType.cpp
  ir/ts/tsAnyKeyword.cpp
  ir/ts/tsArrayType.cpp
  ir/ts/tsAsExpression.cpp
  ir/ts/tsBigintKeyword.cpp
  ir/ts/tsBooleanKeyword.cpp
  ir/ts/tsClassImplements.cpp
  ir/ts/tsConditionalType.cpp
  ir/ts/tsConstructorType.cpp
  ir/ts/tsEnumDeclaration.cpp
  ir/ts/tsEnumMember.cpp
  ir/ts/tsExternalModuleReference.cpp
  ir/ts/tsFunctionType.cpp
  ir/ts/tsImportEqualsDeclaration.cpp
  ir/ts/tsImportType.cpp
  ir/ts/tsIndexedAccessType.cpp
  ir/ts/tsInferType.cpp
  ir/ts/tsInterfaceBody.cpp
  ir/ts/tsInterfaceDeclaration.cpp
  ir/ts/tsInterfaceHeritage.cpp
  ir/ts/tsIntersectionType.cpp
  ir/ts/tsLiteralType.cpp
  ir/ts/tsMappedType.cpp
  ir/ts/tsModuleBlock.cpp
  ir/ts/tsModuleDeclaration.cpp
  ir/ts/tsNamedTupleMember.cpp
  ir/ts/tsNeverKeyword.cpp
  ir/ts/tsNonNullExpression.cpp
  ir/ts/tsNullKeyword.cpp
  ir/ts/tsNumberKeyword.cpp
  ir/ts/tsObjectKeyword.cpp
  ir/ts/tsParameterProperty.cpp
  ir/ts/tsParenthesizedType.cpp
  ir/ts/tsQualifiedName.cpp
  ir/ts/tsStringKeyword.cpp
  ir/ts/tsThisType.cpp
  ir/ts/tsTupleType.cpp
  ir/ts/tsTypeAliasDeclaration.cpp
  ir/ts/tsTypeAssertion.cpp
  ir/ts/tsTypeLiteral.cpp
  ir/ts/tsTypeOperator.cpp
  ir/ts/tsTypeParameter.cpp
  ir/ts/tsTypeParameterDeclaration.cpp
  ir/ts/tsTypeParameterInstantiation.cpp
  ir/ts/tsTypePredicate.cpp
  ir/ts/tsTypeQuery.cpp
  ir/ts/tsTypeReference.cpp
  ir/ts/tsUndefinedKeyword.cpp
  ir/ts/tsUnionType.cpp
  ir/ts/tsUnknownKeyword.cpp
  ir/ts/tsVoidKeyword.cpp
  lexer/ASLexer.cpp
  lexer/keywords.cpp
  lexer/keywordsUtil.cpp
  lexer/lexer.cpp
  lexer/ETSLexer.cpp
  lexer/TSLexer.cpp
  lexer/regexp/regexp.cpp
  lexer/token/number.cpp
  lexer/token/sourceLocation.cpp
  lexer/token/token.cpp
  parser/context/classPrivateContext.cpp
  parser/context/parserContext.cpp
  parser/expressionParser.cpp
  parser/expressionTSParser.cpp
  parser/ASparser.cpp
  parser/JSparser.cpp
  parser/parserImpl.cpp
  parser/ETSFormattedParser.cpp
  parser/ETSparser.cpp
  parser/ETSparserAnnotations.cpp
  parser/ETSparserClasses.cpp
  parser/ETSparserEnums.cpp
  parser/ETSparserExpressions.cpp
  parser/ETSparserJsDocInfo.cpp
  parser/ETSparserNamespaces.cpp
  parser/ETSparserStatements.cpp
  parser/ETSparserTypes.cpp
  parser/ETSNolintParser.cpp
  parser/TSparser.cpp
  parser/ThrowingTypedParser.cpp
  parser/TypedParser.cpp
  parser/program/entityNameVisitor.cpp
  parser/program/program.cpp
  parser/statementParser.cpp
  parser/statementTSParser.cpp
  checker/checker.cpp
  checker/checkerContext.cpp
  checker/ETSAnalyzer.cpp
  checker/ETSAnalyzerHelpers.cpp
  checker/ETSAnalyzerUnreachable.cpp
  checker/ETSchecker.cpp
  checker/TSchecker.cpp
  checker/ASchecker.cpp
  checker/TSAnalyzer.cpp
  checker/TSAnalyzerUnreachable.cpp
  checker/JSchecker.cpp
  checker/IsolatedDeclgenChecker.cpp
  checker/typeChecker/TypeChecker.cpp
  checker/ets/aliveAnalyzer.cpp
  checker/ets/etsWarningAnalyzer.cpp
  checker/ets/arithmetic.cpp
  checker/ets/assignAnalyzer.cpp
  checker/ets/baseAnalyzer.cpp
  checker/ets/boxingConverter.cpp
  checker/ets/castingContext.cpp
  checker/ets/conversion.cpp
  checker/ets/dynamic.cpp
  checker/ets/dynamic/dynamicCall.cpp
  checker/ets/function.cpp
  checker/ets/validateHelpers.cpp
  checker/ets/typeCheckingHelpers.cpp
  checker/ets/helpers.cpp
  checker/ets/narrowingConverter.cpp
  checker/ets/narrowingWideningConverter.cpp
  checker/ets/object.cpp
  checker/ets/typeConverter.cpp
  checker/ets/typeCreation.cpp
  checker/ets/typeRelationContext.cpp
  checker/ets/unboxingConverter.cpp
  checker/ets/utilityTypeHandlers.cpp
  checker/ets/wideningConverter.cpp
  checker/ts/binaryLikeExpression.cpp
  checker/ts/destructuringContext.cpp
  checker/ts/function.cpp
  checker/ts/helpers.cpp
  checker/ts/object.cpp
  checker/ts/typeCreation.cpp
  checker/ts/typeElaborationContext.cpp
  checker/ts/util.cpp
  checker/types/signature.cpp
  checker/types/type.cpp
  checker/types/typeRelation.cpp
  checker/types/globalTypesHolder.cpp
  checker/types/ets/byteType.cpp
  checker/types/ets/charType.cpp
  checker/types/ets/doubleType.cpp
  checker/types/ets/floatType.cpp
  checker/types/ets/intType.cpp
  checker/types/ets/longType.cpp
  checker/types/ets/shortType.cpp
  checker/types/ets/etsAnyType.cpp
  checker/types/ets/etsArrayType.cpp
  checker/types/ets/etsBooleanType.cpp
  checker/types/ets/etsDynamicType.cpp
  checker/types/ets/etsEnumType.cpp
  checker/types/ets/etsExtensionFuncHelperType.cpp
  checker/types/ets/etsFunctionType.cpp
  checker/types/ets/etsNonNullishType.cpp
  checker/types/ets/etsNeverType.cpp
  checker/types/ets/etsReadonlyType.cpp
  checker/types/ets/etsResizableArrayType.cpp
  checker/types/ets/etsNullishTypes.cpp
  checker/types/ets/etsObjectType.cpp
  checker/types/ets/etsStringType.cpp
  checker/types/ets/etsBigIntType.cpp
  checker/types/ets/etsTupleType.cpp
  checker/types/ets/etsTypeAliasType.cpp
  checker/types/ets/etsTypeParameter.cpp
  checker/types/ets/etsPartialTypeParameter.cpp
  checker/types/ets/etsUnionType.cpp
  checker/types/ets/etsVoidType.cpp
  checker/types/ets/wildcardType.cpp
  checker/types/ets/etsAsyncFuncReturnType.cpp
  checker/types/ts/anyType.cpp
  checker/types/ts/arrayType.cpp
  checker/types/ts/bigintLiteralType.cpp
  checker/types/ts/bigintType.cpp
  checker/types/ts/booleanLiteralType.cpp
  checker/types/ts/booleanType.cpp
  checker/types/ts/constructorType.cpp
  checker/types/ts/enumLiteralType.cpp
  checker/types/ts/enumType.cpp
  checker/types/ts/functionType.cpp
  checker/types/ts/indexInfo.cpp
  checker/types/ts/interfaceType.cpp
  checker/types/ts/neverType.cpp
  checker/types/ts/nonPrimitiveType.cpp
  checker/types/ts/nullType.cpp
  checker/types/ts/numberLiteralType.cpp
  checker/types/ts/numberType.cpp
  checker/types/ts/objectDescriptor.cpp
  checker/types/ts/objectLiteralType.cpp
  checker/types/ts/objectType.cpp
  checker/types/ts/stringLiteralType.cpp
  checker/types/ts/stringType.cpp
  checker/types/ts/tupleType.cpp
  checker/types/ts/typeParameter.cpp
  checker/types/ts/typeReference.cpp
  checker/types/ts/undefinedType.cpp
  checker/types/ts/unionType.cpp
  checker/types/ts/unknownType.cpp
  checker/types/ts/voidType.cpp
  util/arktsconfig.cpp
  util/bitset.cpp
  util/dtoa_helper.cpp
  util/diagnostic.cpp
  util/diagnosticEngine.cpp
  util/errorRecovery.cpp
  util/es2pandaMacros.cpp
  util/helpers.cpp
  util/importPathManager.cpp
  util/path.cpp
  util/ustring.cpp
  test/utils/panda_executable_path_getter.cpp
  evaluate/debugInfoDeserialization/debugInfoDeserializer.cpp
  evaluate/debugInfoDeserialization/inheritanceResolution.cpp
  evaluate/debugInfoDeserialization/methodBuilder.cpp
  evaluate/debugInfoDeserialization/classBuilder.cpp
  evaluate/debugInfoStorage.cpp
  evaluate/helpers.cpp
  evaluate/pathResolver.cpp
  evaluate/irCheckHelper.cpp
  evaluate/proxyProgramsCache.cpp
  evaluate/scopedDebugInfoPlugin.cpp
  evaluate/entityDeclarator.cpp
  evaluate/evaluateContext.cpp
)

# libes2panda does not include bytecode optimizer, because it is used in
# libarkruntime, and conflict with JIT setup ensues
panda_frontend_add_library(es2panda-lib ${PANDA_DEFAULT_LIB_TYPE} ${ES2PANDA_LIB_SRC})
add_dependencies(es2panda-lib isa_gen_es2panda es2panda_options_gen es2panda_diagnostic_gen es2panda_keywords es2panda_signatures)

set(ICU_INCLUDE_DIRS
    ${PANDA_THIRD_PARTY_SOURCES_DIR}/icu/icu4c/source/common
    ${PANDA_THIRD_PARTY_SOURCES_DIR}/icu/icu4c/source/i18n
    ${PANDA_THIRD_PARTY_SOURCES_DIR}/icu/icu4c/source
)

panda_target_include_directories(es2panda-lib SYSTEM
  PRIVATE ${ICU_INCLUDE_DIRS}
)

panda_target_include_directories(es2panda-lib
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${OUTPUT_DIR}
)

panda_target_compile_options(es2panda-lib
  PRIVATE -fexceptions -Werror=shadow
)

panda_target_link_libraries(es2panda-lib
  PUBLIC arkbase hmicuuc.z
  PRIVATE arkassembler arkdisassembler arkfile
)

if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1) OR
   (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0))
    panda_target_link_libraries(es2panda-lib
      PUBLIC stdc++fs
    )
endif()

if (PANDA_FUZZILLI)
  panda_target_compile_options(es2panda-lib
      PRIVATE -fPIC
  )
endif()
panda_add_sanitizers(TARGET es2panda-lib SANITIZERS
  ${PANDA_SANITIZERS_LIST})

if(ES2PANDA_ENABLE_PCH AND NOT PANDA_USE_PREBUILT_TARGETS)
  target_precompile_headers(es2panda-lib
      PRIVATE
          util/ustring.h
          ir/astNode.h
          compiler/core/pandagen.h
          compiler/core/ETSGen.h
          checker/TSchecker.h
          public/public.h
  )
endif()

SET(ES2PANDA_LIB_DIR public)
add_subdirectory(${ES2PANDA_LIB_DIR})

add_subdirectory(aot)
add_subdirectory(declgen_ets2ts)
add_subdirectory(driver/dependency_analyzer)

if(PANDA_TARGET_LINUX OR PANDA_TARGET_WINDOWS)
  SET(LSP_LIB "es2panda_lsp")
  add_subdirectory(lsp)
  set(ES2PANDA_NAPI_LIB_DIR "bindings")
  add_subdirectory(${ES2PANDA_NAPI_LIB_DIR})
endif()

if(PANDA_WITH_TESTS)
  add_subdirectory(test)
endif()
