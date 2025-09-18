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

#ifndef ES2PANDA_IR_AST_NODE_MAPPING_H
#define ES2PANDA_IR_AST_NODE_MAPPING_H

// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define AST_NODE_MAPPING(_)                                                               \
    _(ARROW_FUNCTION_EXPRESSION, ArrowFunctionExpression)                                 \
    _(ANNOTATION_DECLARATION, AnnotationDeclaration)                                      \
    _(ANNOTATION_USAGE, AnnotationUsage)                                                  \
    _(ASSERT_STATEMENT, AssertStatement)                                                  \
    _(AWAIT_EXPRESSION, AwaitExpression)                                                  \
    _(BIGINT_LITERAL, BigIntLiteral)                                                      \
    _(BINARY_EXPRESSION, BinaryExpression)                                                \
    _(BLOCK_STATEMENT, BlockStatement)                                                    \
    _(BOOLEAN_LITERAL, BooleanLiteral)                                                    \
    _(BREAK_STATEMENT, BreakStatement)                                                    \
    _(CALL_EXPRESSION, CallExpression)                                                    \
    _(CATCH_CLAUSE, CatchClause)                                                          \
    _(CHAIN_EXPRESSION, ChainExpression)                                                  \
    _(CHAR_LITERAL, CharLiteral)                                                          \
    _(CLASS_DEFINITION, ClassDefinition)                                                  \
    _(CLASS_DECLARATION, ClassDeclaration)                                                \
    _(CLASS_FROM_EXPRESSION, ClassFromExpression)                                         \
    _(CLASS_EXPRESSION, ClassExpression)                                                  \
    _(CLASS_PROPERTY, ClassProperty)                                                      \
    _(CLASS_STATIC_BLOCK, ClassStaticBlock)                                               \
    _(CONDITIONAL_EXPRESSION, ConditionalExpression)                                      \
    _(CONTINUE_STATEMENT, ContinueStatement)                                              \
    _(DEBUGGER_STATEMENT, DebuggerStatement)                                              \
    _(DECORATOR, Decorator)                                                               \
    _(DIRECT_EVAL, DirectEvalExpression)                                                  \
    _(DO_WHILE_STATEMENT, DoWhileStatement)                                               \
    _(EMPTY_STATEMENT, EmptyStatement)                                                    \
    _(EXPORT_ALL_DECLARATION, ExportAllDeclaration)                                       \
    _(EXPORT_DEFAULT_DECLARATION, ExportDefaultDeclaration)                               \
    _(EXPORT_NAMED_DECLARATION, ExportNamedDeclaration)                                   \
    _(EXPORT_SPECIFIER, ExportSpecifier)                                                  \
    _(EXPRESSION_STATEMENT, ExpressionStatement)                                          \
    _(FOR_IN_STATEMENT, ForInStatement)                                                   \
    _(FOR_OF_STATEMENT, ForOfStatement)                                                   \
    _(FOR_UPDATE_STATEMENT, ForUpdateStatement)                                           \
    _(FUNCTION_DECLARATION, FunctionDeclaration)                                          \
    _(FUNCTION_EXPRESSION, FunctionExpression)                                            \
    _(IDENTIFIER, Identifier)                                                             \
    _(DUMMYNODE, DummyNode)                                                               \
    _(IF_STATEMENT, IfStatement)                                                          \
    _(IMPORT_DECLARATION, ImportDeclaration)                                              \
    _(IMPORT_EXPRESSION, ImportExpression)                                                \
    _(IMPORT_DEFAULT_SPECIFIER, ImportDefaultSpecifier)                                   \
    _(IMPORT_NAMESPACE_SPECIFIER, ImportNamespaceSpecifier)                               \
    _(IMPORT_SPECIFIER, ImportSpecifier)                                                  \
    _(LABELLED_STATEMENT, LabelledStatement)                                              \
    _(MEMBER_EXPRESSION, MemberExpression)                                                \
    _(META_PROPERTY_EXPRESSION, MetaProperty)                                             \
    _(METHOD_DEFINITION, MethodDefinition)                                                \
    _(NAMED_TYPE, NamedType)                                                              \
    _(NEW_EXPRESSION, NewExpression)                                                      \
    _(NULL_LITERAL, NullLiteral)                                                          \
    _(UNDEFINED_LITERAL, UndefinedLiteral)                                                \
    _(NUMBER_LITERAL, NumberLiteral)                                                      \
    _(OMITTED_EXPRESSION, OmittedExpression)                                              \
    _(OVERLOAD_DECLARATION, OverloadDeclaration)                                          \
    _(PREFIX_ASSERTION_EXPRESSION, PrefixAssertionExpression)                             \
    _(PROPERTY, Property)                                                                 \
    _(REGEXP_LITERAL, RegExpLiteral)                                                      \
    _(REEXPORT_STATEMENT, ETSReExportDeclaration)                                         \
    _(RETURN_STATEMENT, ReturnStatement)                                                  \
    _(SCRIPT_FUNCTION, ScriptFunction)                                                    \
    _(SEQUENCE_EXPRESSION, SequenceExpression)                                            \
    _(STRING_LITERAL, StringLiteral)                                                      \
    _(ETS_NON_NULLISH_TYPE, ETSNonNullishTypeNode)                                        \
    _(ETS_NULL_TYPE, ETSNullType)                                                         \
    _(ETS_UNDEFINED_TYPE, ETSUndefinedType)                                               \
    _(ETS_NEVER_TYPE, ETSNeverType)                                                       \
    _(ETS_STRING_LITERAL_TYPE, ETSStringLiteralType)                                      \
    _(ETS_INTRINSIC_NODE_TYPE, ETSIntrinsicNode)                                          \
    _(ETS_FUNCTION_TYPE, ETSFunctionType)                                                 \
    _(ETS_WILDCARD_TYPE, ETSWildcardType)                                                 \
    _(ETS_PRIMITIVE_TYPE, ETSPrimitiveType)                                               \
    _(ETS_PACKAGE_DECLARATION, ETSPackageDeclaration)                                     \
    _(ETS_CLASS_LITERAL, ETSClassLiteral)                                                 \
    _(ETS_TYPE_REFERENCE, ETSTypeReference)                                               \
    _(ETS_TYPE_REFERENCE_PART, ETSTypeReferencePart)                                      \
    _(ETS_UNION_TYPE, ETSUnionType)                                                       \
    _(ETS_KEYOF_TYPE, ETSKeyofType)                                                       \
    _(ETS_NEW_ARRAY_INSTANCE_EXPRESSION, ETSNewArrayInstanceExpression)                   \
    _(ETS_NEW_MULTI_DIM_ARRAY_INSTANCE_EXPRESSION, ETSNewMultiDimArrayInstanceExpression) \
    _(ETS_NEW_CLASS_INSTANCE_EXPRESSION, ETSNewClassInstanceExpression)                   \
    _(ETS_IMPORT_DECLARATION, ETSImportDeclaration)                                       \
    _(ETS_PARAMETER_EXPRESSION, ETSParameterExpression)                                   \
    _(ETS_TUPLE, ETSTuple)                                                                \
    _(ETS_MODULE, ETSModule)                                                              \
    _(SUPER_EXPRESSION, SuperExpression)                                                  \
    _(STRUCT_DECLARATION, ETSStructDeclaration)                                           \
    _(SWITCH_CASE_STATEMENT, SwitchCaseStatement)                                         \
    _(SWITCH_STATEMENT, SwitchStatement)                                                  \
    _(TS_ENUM_DECLARATION, TSEnumDeclaration)                                             \
    _(TS_ENUM_MEMBER, TSEnumMember)                                                       \
    _(TS_EXTERNAL_MODULE_REFERENCE, TSExternalModuleReference)                            \
    _(TS_NUMBER_KEYWORD, TSNumberKeyword)                                                 \
    _(TS_ANY_KEYWORD, TSAnyKeyword)                                                       \
    _(TS_STRING_KEYWORD, TSStringKeyword)                                                 \
    _(TS_BOOLEAN_KEYWORD, TSBooleanKeyword)                                               \
    _(TS_VOID_KEYWORD, TSVoidKeyword)                                                     \
    _(TS_UNDEFINED_KEYWORD, TSUndefinedKeyword)                                           \
    _(TS_UNKNOWN_KEYWORD, TSUnknownKeyword)                                               \
    _(TS_OBJECT_KEYWORD, TSObjectKeyword)                                                 \
    _(TS_BIGINT_KEYWORD, TSBigintKeyword)                                                 \
    _(TS_NEVER_KEYWORD, TSNeverKeyword)                                                   \
    _(TS_NON_NULL_EXPRESSION, TSNonNullExpression)                                        \
    _(TS_NULL_KEYWORD, TSNullKeyword)                                                     \
    _(TS_ARRAY_TYPE, TSArrayType)                                                         \
    _(TS_UNION_TYPE, TSUnionType)                                                         \
    _(TS_TYPE_LITERAL, TSTypeLiteral)                                                     \
    _(TS_PROPERTY_SIGNATURE, TSPropertySignature)                                         \
    _(TS_METHOD_SIGNATURE, TSMethodSignature)                                             \
    _(TS_SIGNATURE_DECLARATION, TSSignatureDeclaration)                                   \
    _(TS_PARENT_TYPE, TSParenthesizedType)                                                \
    _(TS_LITERAL_TYPE, TSLiteralType)                                                     \
    _(TS_INFER_TYPE, TSInferType)                                                         \
    _(TS_CONDITIONAL_TYPE, TSConditionalType)                                             \
    _(TS_IMPORT_TYPE, TSImportType)                                                       \
    _(TS_INTERSECTION_TYPE, TSIntersectionType)                                           \
    _(TS_MAPPED_TYPE, TSMappedType)                                                       \
    _(TS_MODULE_BLOCK, TSModuleBlock)                                                     \
    _(TS_THIS_TYPE, TSThisType)                                                           \
    _(TS_TYPE_OPERATOR, TSTypeOperator)                                                   \
    _(TS_TYPE_PARAMETER, TSTypeParameter)                                                 \
    _(TS_TYPE_PARAMETER_DECLARATION, TSTypeParameterDeclaration)                          \
    _(TS_TYPE_PARAMETER_INSTANTIATION, TSTypeParameterInstantiation)                      \
    _(TS_TYPE_PREDICATE, TSTypePredicate)                                                 \
    _(TS_PARAMETER_PROPERTY, TSParameterProperty)                                         \
    _(TS_MODULE_DECLARATION, TSModuleDeclaration)                                         \
    _(TS_IMPORT_EQUALS_DECLARATION, TSImportEqualsDeclaration)                            \
    _(TS_FUNCTION_TYPE, TSFunctionType)                                                   \
    _(TS_CONSTRUCTOR_TYPE, TSConstructorType)                                             \
    _(TS_TYPE_ALIAS_DECLARATION, TSTypeAliasDeclaration)                                  \
    _(TS_TYPE_REFERENCE, TSTypeReference)                                                 \
    _(TS_QUALIFIED_NAME, TSQualifiedName)                                                 \
    _(TS_INDEXED_ACCESS_TYPE, TSIndexedAccessType)                                        \
    _(TS_INTERFACE_DECLARATION, TSInterfaceDeclaration)                                   \
    _(TS_INTERFACE_BODY, TSInterfaceBody)                                                 \
    _(TS_INTERFACE_HERITAGE, TSInterfaceHeritage)                                         \
    _(TS_TUPLE_TYPE, TSTupleType)                                                         \
    _(TS_NAMED_TUPLE_MEMBER, TSNamedTupleMember)                                          \
    _(TS_INDEX_SIGNATURE, TSIndexSignature)                                               \
    _(TS_TYPE_QUERY, TSTypeQuery)                                                         \
    _(TS_AS_EXPRESSION, TSAsExpression)                                                   \
    _(TS_CLASS_IMPLEMENTS, TSClassImplements)                                             \
    _(TS_TYPE_ASSERTION, TSTypeAssertion)                                                 \
    _(TAGGED_TEMPLATE_EXPRESSION, TaggedTemplateExpression)                               \
    _(TEMPLATE_ELEMENT, TemplateElement)                                                  \
    _(TEMPLATE_LITERAL, TemplateLiteral)                                                  \
    _(THIS_EXPRESSION, ThisExpression)                                                    \
    _(TYPEOF_EXPRESSION, TypeofExpression)                                                \
    _(THROW_STATEMENT, ThrowStatement)                                                    \
    _(TRY_STATEMENT, TryStatement)                                                        \
    _(UNARY_EXPRESSION, UnaryExpression)                                                  \
    _(UPDATE_EXPRESSION, UpdateExpression)                                                \
    _(VARIABLE_DECLARATION, VariableDeclaration)                                          \
    _(VARIABLE_DECLARATOR, VariableDeclarator)                                            \
    _(WHILE_STATEMENT, WhileStatement)                                                    \
    _(YIELD_EXPRESSION, YieldExpression)                                                  \
    _(OPAQUE_TYPE_NODE, OpaqueTypeNode)                                                   \
    _(BLOCK_EXPRESSION, BlockExpression)                                                  \
    _(BROKEN_TYPE_NODE, BrokenTypeNode)

#define AST_NODE_REINTERPRET_MAPPING(_)                                                   \
    _(ARRAY_EXPRESSION, ARRAY_PATTERN, ArrayExpression, ArrayPattern)                     \
    _(ASSIGNMENT_EXPRESSION, ASSIGNMENT_PATTERN, AssignmentExpression, AssignmentPattern) \
    _(OBJECT_EXPRESSION, OBJECT_PATTERN, ObjectExpression, ObjectPattern)                 \
    _(SPREAD_ELEMENT, REST_ELEMENT, SpreadElement, RestElement)
// NOLINTEND(cppcoreguidelines-macro-usage)

#endif
