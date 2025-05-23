/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

export enum FaultID {
  AnyType,
  SymbolType,
  ObjectLiteralNoContextType,
  ArrayLiteralNoContextType,
  ComputedPropertyName,
  LiteralAsPropertyName,
  TypeQuery,
  IsOperator,
  DestructuringParameter,
  YieldExpression,
  InterfaceMerging,
  EnumMerging,
  InterfaceExtendsClass,
  IndexMember,
  WithStatement,
  ThrowStatement,
  IndexedAccessType,
  UnknownType,
  ForInStatement,
  InOperator,
  FunctionExpression,
  IntersectionType,
  ObjectTypeLiteral,
  CommaOperator,
  LimitedReturnTypeInference,
  ClassExpression,
  DestructuringAssignment,
  DestructuringDeclaration,
  VarDeclaration,
  CatchWithUnsupportedType,
  DeleteOperator,
  DeclWithDuplicateName,
  UnaryArithmNotNumber,
  ConstructorType,
  ConstructorIface,
  ConstructorFuncs,
  CallSignature,
  TypeAssertion,
  PrivateIdentifier,
  LocalFunction,
  ConditionalType,
  MappedType,
  NamespaceAsObject,
  ClassAsObject,
  ClassAsObjectError,
  NonDeclarationInNamespace,
  GeneratorFunction,
  FunctionContainsThis,
  PropertyAccessByIndex,
  JsxElement,
  EnumMemberNonConstInit,
  ImplementsClass,
  MethodReassignment,
  MultipleStaticBlocks,
  ThisType,
  IntefaceExtendDifProps,
  StructuralIdentity,
  ExportAssignment,
  ImportAssignment,
  GenericCallNoTypeArgs,
  ParameterProperties,
  InstanceofUnsupported,
  ShorthandAmbientModuleDecl,
  WildcardsInModuleName,
  UMDModuleDefinition,
  NewTarget,
  DefiniteAssignment,
  DefiniteAssignmentError,
  Prototype,
  GlobalThis,
  GlobalThisError,
  UtilityType,
  PropertyDeclOnFunction,
  FunctionApplyCall,
  FunctionBind,
  FunctionBindError,
  ConstAssertion,
  ImportAssertion,
  SpreadOperator,
  LimitedStdLibApi,
  ErrorSuppression,
  StrictDiagnostic,
  ImportAfterStatement,
  EsObjectType,
  EsObjectTypeError,
  SendableClassInheritance,
  SendablePropType,
  SendableDefiniteAssignment,
  SendableGenericTypes,
  SendableCapturedVars,
  SendableClassDecorator,
  SendableObjectInitialization,
  SendableComputedPropName,
  SendableAsExpr,
  SharedNoSideEffectImport,
  SharedModuleExports,
  SharedModuleNoWildcardExport,
  NoTsImportEts,
  SendableTypeInheritance,
  SendableTypeExported,
  NoTsReExportEts,
  NoNameSpaceImportEtsToTs,
  NoSideEffectImportEtsToTs,
  SendableExplicitFieldType,
  SendableFunctionImportedVariables,
  SendableFunctionDecorator,
  SendableTypeAliasDecorator,
  SendableTypeAliasDeclaration,
  SendableFunctionAssignment,
  SendableFunctionOverloadDecorator,
  SendableFunctionProperty,
  SendableFunctionAsExpr,
  SendableDecoratorLimited,
  SharedModuleExportsWarning,
  SendableBetaCompatible,
  ObjectLiteralProperty,
  OptionalMethod,
  ImportType,
  DynamicCtorCall,
  // this should always be last enum
  LAST_ID
}
