/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

import * as path from 'node:path';
import * as ts from 'typescript';
import { FaultID } from './Problems';
import { TypeScriptLinterConfig } from './TypeScriptLinterConfig';
import type { Autofix } from './autofixes/Autofixer';
import { Autofixer } from './autofixes/Autofixer';
import { PROMISE_METHODS, SYMBOL, SYMBOL_CONSTRUCTOR, TsUtils } from './utils/TsUtils';
import { FUNCTION_HAS_NO_RETURN_ERROR_CODE } from './utils/consts/FunctionHasNoReturnErrorCode';
import { LIMITED_STANDARD_UTILITY_TYPES } from './utils/consts/LimitedStandardUtilityTypes';
import { LIKE_FUNCTION } from './utils/consts/LikeFunction';
import { METHOD_DECLARATION } from './utils/consts/MethodDeclaration';
import { METHOD_SIGNATURE } from './utils/consts/MethodSignature';
import { OPTIONAL_METHOD } from './utils/consts/OptionalMethod';
import {
  STRINGLITERAL_NUMBER,
  STRINGLITERAL_STRING,
  STRINGLITERAL_INT,
  STRINGLITERAL_BYTE,
  STRINGLITERAL_SHORT,
  STRINGLITERAL_CHAR,
  STRINGLITERAL_LONG,
  STRINGLITERAL_FROM,
  STRINGLITERAL_ARRAY
} from './utils/consts/StringLiteral';
import {
  NON_INITIALIZABLE_PROPERTY_CLASS_DECORATORS,
  NON_INITIALIZABLE_PROPERTY_DECORATORS,
  NON_INITIALIZABLE_PROPERTY_DECORATORS_TSC
} from './utils/consts/NonInitializablePropertyDecorators';
import { NON_RETURN_FUNCTION_DECORATORS } from './utils/consts/NonReturnFunctionDecorators';
import { PROPERTY_HAS_NO_INITIALIZER_ERROR_CODE } from './utils/consts/PropertyHasNoInitializerErrorCode';
import {
  CONCURRENT_DECORATOR,
  ISCONCURRENT,
  SENDABLE_DECORATOR,
  SENDABLE_DECORATOR_NODES,
  SENDABLE_FUNCTION_UNSUPPORTED_STAGES_IN_API12,
  SENDBALE_FUNCTION_START_VERSION,
  TASKPOOL
} from './utils/consts/SendableAPI';
import { DEFAULT_COMPATIBLE_SDK_VERSION, DEFAULT_COMPATIBLE_SDK_VERSION_STAGE } from './utils/consts/VersionInfo';
import { forEachNodeInSubtree } from './utils/functions/ForEachNodeInSubtree';
import { hasPredecessor } from './utils/functions/HasPredecessor';
import { isStdLibrarySymbol, isStdLibraryType } from './utils/functions/IsStdLibrary';
import { isStruct, isStructDeclaration } from './utils/functions/IsStruct';
import {
  LibraryTypeCallDiagnosticChecker,
  ErrorType as DiagnosticCheckerErrorType
} from './utils/functions/LibraryTypeCallDiagnosticChecker';
import {
  ALLOWED_STD_SYMBOL_API,
  LIMITED_STD_API,
  LIMITED_STD_GLOBAL_API,
  LIMITED_STD_OBJECT_API,
  LIMITED_STD_PROXYHANDLER_API,
  LIMITED_STD_REFLECT_API,
  MODULE_IMPORTS,
  ARKTSUTILS_MODULES,
  ARKTSUTILS_LOCKS_MEMBER
} from './utils/consts/LimitedStdAPI';
import { SupportedStdCallApiChecker } from './utils/functions/SupportedStdCallAPI';
import { identiferUseInValueContext } from './utils/functions/identiferUseInValueContext';
import { isAssignmentOperator } from './utils/functions/isAssignmentOperator';
import { StdClassVarDecls } from './utils/consts/StdClassVariableDeclarations';
import type { LinterOptions } from './LinterOptions';
import { BUILTIN_GENERIC_CONSTRUCTORS } from './utils/consts/BuiltinGenericConstructor';
import { DEFAULT_DECORATOR_WHITE_LIST } from './utils/consts/DefaultDecoratorWhitelist';
import { INVALID_IDENTIFIER_KEYWORDS } from './utils/consts/InValidIndentifierKeywords';
import { WORKER_MODULES, WORKER_TEXT } from './utils/consts/WorkerAPI';
import { COLLECTIONS_TEXT, COLLECTIONS_MODULES } from './utils/consts/CollectionsAPI';
import { ASON_TEXT, ASON_MODULES, ARKTS_UTILS_TEXT, JSON_TEXT, ASON_WHITE_SET } from './utils/consts/ArkTSUtilsAPI';
import { interanlFunction } from './utils/consts/InternalFunction';
import { ETS_PART, PATH_SEPARATOR } from './utils/consts/OhmUrl';
import {
  DOUBLE_DOLLAR_IDENTIFIER,
  THIS_IDENTIFIER,
  STATE_STYLES,
  CustomDecoratorName,
  observedDecoratorName,
  skipImportDecoratorName,
  ENTRY_DECORATOR_NAME,
  PROVIDE_DECORATOR_NAME,
  PROVIDE_ALLOW_OVERRIDE_PROPERTY_NAME,
  ARKUI_PACKAGE_NAME,
  MAKE_OBSERVED,
  ARKUI_STATE_MANAGEMENT,
  PropDecoratorName,
  PropFunctionName,
  StorageTypeName,
  customLayoutFunctionName
} from './utils/consts/ArkuiConstants';
import { arkuiImportList } from './utils/consts/ArkuiImportList';
import type { IdentifierAndArguments, ForbidenAPICheckResult } from './utils/consts/InteropAPI';
import {
  NONE,
  OBJECT_LITERAL,
  OBJECT_PROPERTIES,
  REFLECT_LITERAL,
  REFLECT_PROPERTIES
} from './utils/consts/InteropAPI';
import { EXTNAME_TS, EXTNAME_D_TS, EXTNAME_JS } from './utils/consts/ExtensionName';
import { ARKTS_IGNORE_DIRS_OH_MODULES } from './utils/consts/ArktsIgnorePaths';
import type { ApiInfo, ApiListItem } from './utils/consts/SdkWhitelist';
import { ApiList, SdkProblem, SdkNameInfo } from './utils/consts/SdkWhitelist';
import * as apiWhiteList from './data/SdkWhitelist.json';
import * as builtinWhiteList from './data/BuiltinList.json';
import {
  BuiltinProblem,
  SYMBOL_ITERATOR,
  BUILTIN_DISABLE_CALLSIGNATURE,
  GET_OWN_PROPERTY_NAMES_TEXT
} from './utils/consts/BuiltinWhiteList';
import {
  USE_SHARED,
  USE_CONCURRENT,
  ESLIB_SHAREDMEMORY_FILENAME,
  ESLIB_SHAREDARRAYBUFFER,
  TASKPOOL_MODULES
} from './utils/consts/ConcurrentAPI';
import {
  DEPRECATED_TASKPOOL_METHOD_SETCLONELIST,
  DEPRECATED_TASKPOOL_METHOD_SETTRANSFERLIST,
  STDLIB_TASK_CLASS_NAME,
  STDLIB_TASKPOOL_OBJECT_NAME
} from './utils/consts/TaskpoolAPI';
import { BaseTypeScriptLinter } from './BaseTypeScriptLinter';
import type { ArrayAccess, UncheckedIdentifier } from './utils/consts/RuntimeCheckAPI';
import { NUMBER_LITERAL } from './utils/consts/RuntimeCheckAPI';
import { globalApiAssociatedInfo } from './utils/consts/AssociatedInfo';
import { ARRAY_API_LIST } from './utils/consts/ArraysAPI';
import {
  ABILITY_KIT,
  ASYNC_LIFECYCLE_SDK_LIST,
  ON_DESTROY,
  ON_DISCONNECT,
  PROMISE,
  SERVICE_EXTENSION_ABILITY,
  VOID
} from './utils/consts/AsyncLifecycleSDK';
import { ERROR_PROP_LIST } from './utils/consts/ErrorProp';
import { D_ETS, D_TS } from './utils/consts/TsSuffix';
import { arkTsBuiltInTypeName } from './utils/consts/ArkuiImportList';
import { ERROR_TASKPOOL_PROP_LIST } from './utils/consts/ErrorProp';

interface InterfaceSymbolTypeResult {
  propNames: string[];
  typeNames: string[];
  allProps: Map<string, string>;
}
interface InterfaceSymbolTypePropertyNames {
  propertyNames: string[];
  typeNames: string[];
}

export class TypeScriptLinter extends BaseTypeScriptLinter {
  supportedStdCallApiChecker: SupportedStdCallApiChecker;

  autofixer: Autofixer | undefined;
  private fileExportDeclCaches: Set<ts.Node> | undefined;

  private useStatic?: boolean;

  private readonly compatibleSdkVersion: number;
  private readonly compatibleSdkVersionStage: string;
  private static sharedModulesCache: Map<string, boolean>;
  static nameSpaceFunctionCache: Map<string, Set<string>>;
  private readonly constVariableInitCache: Map<ts.Symbol, number | null> = new Map();
  static funcMap: Map<string, Map<string, Set<ApiInfo>>> = new Map<string, Map<string, Set<ApiInfo>>>();
  private interfaceMap: Map<string, Set<ApiInfo>> = new Map<string, Set<ApiInfo>>();
  static pathMap: Map<string, Set<ApiInfo>>;
  static indexedTypeSet: Set<ApiListItem>;
  static globalApiInfo: Map<string, Set<ApiListItem>>;
  static symbotIterSet: Set<string>;
  static missingAttributeSet: Set<string>;
  static literalAsPropertyNameTypeSet: Set<ApiListItem>;
  private localApiListItem: ApiListItem | undefined = undefined;
  static constructorFuncsSet: Set<ApiListItem>;
  static ConstructorIfaceSet: Set<ApiListItem>;

  static initGlobals(): void {
    TypeScriptLinter.sharedModulesCache = new Map<string, boolean>();
    TypeScriptLinter.nameSpaceFunctionCache = new Map<string, Set<string>>();
    TypeScriptLinter.pathMap = new Map<string, Set<ApiInfo>>();
    TypeScriptLinter.globalApiInfo = new Map<string, Set<ApiListItem>>();
    TypeScriptLinter.funcMap = new Map<string, Map<string, Set<ApiInfo>>>();
    TypeScriptLinter.symbotIterSet = new Set<string>();
    TypeScriptLinter.missingAttributeSet = new Set<string>();
    TypeScriptLinter.initSdkWhitelist();
    TypeScriptLinter.initSdkBuiltinInfo();
    TypeScriptLinter.initBuiltinlist();
  }

  initSdkInfo(): void {
    this.interfaceMap = new Map<string, Set<ApiInfo>>();
  }

  static initSdkBuiltinInfo(): void {
    const list: ApiList = new ApiList(builtinWhiteList);
    if (list?.api_list?.length > 0) {
      for (const item of list.api_list) {
        switch (item.api_info.problem) {
          case BuiltinProblem.MissingAttributes:
            TypeScriptLinter.missingAttributeSet.add(item.file_path);
            break;
          case BuiltinProblem.SymbolIterator:
            TypeScriptLinter.symbotIterSet.add(item.file_path);
            break;
          case BuiltinProblem.LimitedThisArg:
            TypeScriptLinter.initSdkBuiltinThisArgsWhitelist(item);
            break;
          default:
        }
      }
    }
  }

  static initSdkBuiltinThisArgsWhitelist(item: ApiListItem): void {
    if (item.file_path === '' || !item.api_info.api_name) {
      return;
    }

    let funcApiInfos: Map<string, Set<ApiInfo>> | undefined = TypeScriptLinter.funcMap.get(item.api_info.api_name);
    if (!funcApiInfos) {
      funcApiInfos = new Map<string, Set<ApiInfo>>();
      TypeScriptLinter.funcMap.set(item.api_info.api_name, funcApiInfos);
    }
    TypeScriptLinter.addOrUpdateData(funcApiInfos, item.file_path, item.api_info);
  }

  private initEtsHandlers(): void {

    /*
     * some syntax elements are ArkTs-specific and are only implemented inside patched
     * compiler, so we initialize those handlers if corresponding properties do exist
     */
    const etsComponentExpression: ts.SyntaxKind | undefined = ts.SyntaxKind.EtsComponentExpression;
    if (etsComponentExpression) {
      this.handlersMap.set(etsComponentExpression, this.handleEtsComponentExpression);
    }
  }

  private static addSdkIndexedTypeSetData(item: ApiListItem): void {
    if (item.api_info.problem === SdkProblem.IndexedAccessType) {
      TypeScriptLinter.indexedTypeSet.add(item);
    }
  }

  private static addSdkliteralAsPropertyNameTypeSetData(item: ApiListItem): void {
    if (item.api_info.problem === SdkProblem.LiteralAsPropertyName) {
      TypeScriptLinter.literalAsPropertyNameTypeSet.add(item);
    }
  }

  private static addSdkConstructorFuncsSetData(item: ApiListItem): void {
    if (item.api_info.problem === SdkProblem.ConstructorFuncs) {
      TypeScriptLinter.constructorFuncsSet.add(item);
    }
  }

  private static addGlobalApiInfosCollocetionData(item: ApiListItem): void {
    const problemType = item.api_info.problem;
    const isGlobal = item.is_global;
    if (isGlobal) {
      if (!TypeScriptLinter.globalApiInfo.has(problemType)) {
        TypeScriptLinter.globalApiInfo.set(problemType, new Set<ApiListItem>());
      }
      const setApiListItem = TypeScriptLinter.globalApiInfo.get(problemType);
      setApiListItem?.add(item);
    }
  }

  private static addSdkConstructorIfaceSetData(item: ApiListItem): void {
    if (item.api_info.problem === SdkProblem.ConstructorIface) {
      TypeScriptLinter.ConstructorIfaceSet.add(item);
    }
  }

  private static initSdkWhitelist(): void {
    TypeScriptLinter.indexedTypeSet = new Set<ApiListItem>();
    TypeScriptLinter.literalAsPropertyNameTypeSet = new Set<ApiListItem>();
    TypeScriptLinter.constructorFuncsSet = new Set<ApiListItem>();
    const list: ApiList = new ApiList(apiWhiteList);
    TypeScriptLinter.ConstructorIfaceSet = new Set<ApiListItem>();
    if (list?.api_list?.length > 0) {
      for (const item of list.api_list) {
        if (item.file_path !== '') {
          TypeScriptLinter.addOrUpdateData(TypeScriptLinter.pathMap, `'${item.file_path}'`, item.api_info);
        }
        item.import_path.forEach((path) => {
          TypeScriptLinter.addOrUpdateData(TypeScriptLinter.pathMap, `'${path}'`, item.api_info);
        });
        TypeScriptLinter.addSdkIndexedTypeSetData(item);
        TypeScriptLinter.addSdkliteralAsPropertyNameTypeSetData(item);
        TypeScriptLinter.addSdkConstructorFuncsSetData(item);
        TypeScriptLinter.addGlobalApiInfosCollocetionData(item);
        TypeScriptLinter.addSdkConstructorIfaceSetData(item);
      }
    }
  }

  private static initBuiltinlist(): void {
    const list: ApiList = new ApiList(builtinWhiteList);
    if (list?.api_list?.length > 0) {
      for (const item of list.api_list) {
        TypeScriptLinter.addGlobalApiInfosCollocetionData(item);
      }
    }
  }

  private static addOrUpdateData(map: Map<string, Set<ApiInfo>>, path: string, data: ApiInfo): void {
    let apiInfos = map.get(path);
    if (!apiInfos) {
      apiInfos = new Set<ApiInfo>();
      map.set(path, apiInfos);
    }
    apiInfos.add(data);
  }

  constructor(
    tsTypeChecker: ts.TypeChecker,
    options: LinterOptions,
    sourceFile: ts.SourceFile,
    readonly tscStrictDiagnostics?: Map<string, ts.Diagnostic[]>
  ) {
    super(tsTypeChecker, options, sourceFile);
    this.supportedStdCallApiChecker = new SupportedStdCallApiChecker(this.tsUtils, this.tsTypeChecker);
    this.compatibleSdkVersion = options.compatibleSdkVersion || DEFAULT_COMPATIBLE_SDK_VERSION;
    this.compatibleSdkVersionStage = options.compatibleSdkVersionStage || DEFAULT_COMPATIBLE_SDK_VERSION_STAGE;
    this.initEtsHandlers();
    this.initSdkInfo();
  }

  readonly handlersMap = new Map([
    [ts.SyntaxKind.ObjectLiteralExpression, this.handleObjectLiteralExpression],
    [ts.SyntaxKind.ArrayLiteralExpression, this.handleArrayLiteralExpression],
    [ts.SyntaxKind.Parameter, this.handleParameter],
    [ts.SyntaxKind.EnumDeclaration, this.handleEnumDeclaration],
    [ts.SyntaxKind.InterfaceDeclaration, this.handleInterfaceDeclaration],
    [ts.SyntaxKind.TryStatement, this.handleTryStatement],
    [ts.SyntaxKind.ThrowStatement, this.handleThrowStatement],
    [ts.SyntaxKind.ImportClause, this.handleImportClause],
    [ts.SyntaxKind.ForStatement, this.handleForStatement],
    [ts.SyntaxKind.ForInStatement, this.handleForInStatement],
    [ts.SyntaxKind.ForOfStatement, this.handleForOfStatement],
    [ts.SyntaxKind.ImportDeclaration, this.handleImportDeclaration],
    [ts.SyntaxKind.PropertyAccessExpression, this.handlePropertyAccessExpression],
    [ts.SyntaxKind.PropertyDeclaration, this.handlePropertyDeclaration],
    [ts.SyntaxKind.PropertyAssignment, this.handlePropertyAssignment],
    [ts.SyntaxKind.PropertySignature, this.handlePropertySignature],
    [ts.SyntaxKind.FunctionExpression, this.handleFunctionExpression],
    [ts.SyntaxKind.ArrowFunction, this.handleArrowFunction],
    [ts.SyntaxKind.CatchClause, this.handleCatchClause],
    [ts.SyntaxKind.FunctionDeclaration, this.handleFunctionDeclaration],
    [ts.SyntaxKind.PrefixUnaryExpression, this.handlePrefixUnaryExpression],
    [ts.SyntaxKind.BinaryExpression, this.handleBinaryExpression],
    [ts.SyntaxKind.VariableDeclarationList, this.handleVariableDeclarationList],
    [ts.SyntaxKind.VariableDeclaration, this.handleVariableDeclaration],
    [ts.SyntaxKind.ClassDeclaration, this.handleClassDeclaration],
    [ts.SyntaxKind.ModuleDeclaration, this.handleModuleDeclaration],
    [ts.SyntaxKind.TypeAliasDeclaration, this.handleTypeAliasDeclaration],
    [ts.SyntaxKind.ImportSpecifier, this.handleImportSpecifier],
    [ts.SyntaxKind.NamespaceImport, this.handleNamespaceImport],
    [ts.SyntaxKind.TypeAssertionExpression, this.handleTypeAssertionExpression],
    [ts.SyntaxKind.MethodDeclaration, this.handleMethodDeclaration],
    [ts.SyntaxKind.TupleType, this.handleTupleType],
    [ts.SyntaxKind.MethodSignature, this.handleMethodSignature],
    [ts.SyntaxKind.ClassStaticBlockDeclaration, this.handleClassStaticBlockDeclaration],
    [ts.SyntaxKind.Identifier, this.handleIdentifier],
    [ts.SyntaxKind.ElementAccessExpression, this.handleElementAccessExpression],
    [ts.SyntaxKind.EnumMember, this.handleEnumMember],
    [ts.SyntaxKind.TypeReference, this.handleTypeReference],
    [ts.SyntaxKind.ExportAssignment, this.handleExportAssignment],
    [ts.SyntaxKind.CallExpression, this.handleCallExpression],
    [ts.SyntaxKind.MetaProperty, this.handleMetaProperty],
    [ts.SyntaxKind.NewExpression, this.handleNewExpression],
    [ts.SyntaxKind.AsExpression, this.handleAsExpression],
    [ts.SyntaxKind.SpreadElement, this.handleSpreadOp],
    [ts.SyntaxKind.SpreadAssignment, this.handleSpreadOp],
    [ts.SyntaxKind.GetAccessor, this.handleGetAccessor],
    [ts.SyntaxKind.SetAccessor, this.handleSetAccessor],
    [ts.SyntaxKind.StringLiteral, this.handleStringLiteral],
    [ts.SyntaxKind.ConstructSignature, this.handleConstructSignature],
    [ts.SyntaxKind.ExpressionWithTypeArguments, this.handleExpressionWithTypeArguments],
    [ts.SyntaxKind.ComputedPropertyName, this.handleComputedPropertyName],
    [ts.SyntaxKind.Constructor, this.handleConstructorDeclaration],
    [ts.SyntaxKind.PrivateIdentifier, this.handlePrivateIdentifier],
    [ts.SyntaxKind.IndexSignature, this.handleIndexSignature],
    [ts.SyntaxKind.TypeLiteral, this.handleTypeLiteral],
    [ts.SyntaxKind.ExportKeyword, this.handleExportKeyword],
    [ts.SyntaxKind.ExportDeclaration, this.handleExportDeclaration],
    [ts.SyntaxKind.ReturnStatement, this.handleReturnStatement],
    [ts.SyntaxKind.Decorator, this.handleDecorator],
    [ts.SyntaxKind.ImportType, this.handleImportType],
    [ts.SyntaxKind.AsteriskAsteriskToken, this.handleExponentOperation],
    [ts.SyntaxKind.VoidExpression, this.handleVoidExpression],
    [ts.SyntaxKind.AsteriskAsteriskEqualsToken, this.handleExponentOperation],
    [ts.SyntaxKind.RegularExpressionLiteral, this.handleRegularExpressionLiteral],
    [ts.SyntaxKind.DebuggerStatement, this.handleDebuggerStatement],
    [ts.SyntaxKind.SwitchStatement, this.handleSwitchStatement],
    [ts.SyntaxKind.UnionType, this.handleUnionType],
    [ts.SyntaxKind.ArrayType, this.handleArrayType],
    [ts.SyntaxKind.LiteralType, this.handleLimitedLiteralType],
    [ts.SyntaxKind.NonNullExpression, this.handleNonNullExpression],
    [ts.SyntaxKind.HeritageClause, this.handleHeritageClause],
    [ts.SyntaxKind.TaggedTemplateExpression, this.handleTaggedTemplatesExpression],
    [ts.SyntaxKind.StructDeclaration, this.handleStructDeclaration],
    [ts.SyntaxKind.TypeOfExpression, this.handleInterOpImportJsOnTypeOfNode],
    [ts.SyntaxKind.AwaitExpression, this.handleAwaitExpression],
    [ts.SyntaxKind.PostfixUnaryExpression, this.handlePostfixUnaryExpression],
    [ts.SyntaxKind.BigIntLiteral, this.handleBigIntLiteral],
    [ts.SyntaxKind.NumericLiteral, this.handleNumericLiteral]
  ]);

  lint(): void {
    if (this.options.enableAutofix || this.options.migratorMode) {
      this.autofixer = new Autofixer(this.tsTypeChecker, this.tsUtils, this.sourceFile, this.options.cancellationToken);
    }

    this.useStatic = this.tsUtils.isArkts12File(this.sourceFile);
    this.fileExportDeclCaches = undefined;
    this.extractImportedNames(this.sourceFile);
    this.visitSourceFile(this.sourceFile);
    this.handleCommentDirectives(this.sourceFile);
    this.processInterfacesToImport(this.sourceFile);
  }

  private visitSourceFile(sf: ts.SourceFile): void {
    const callback = (node: ts.Node): void => {
      this.fileStats.visitedNodes++;
      if (isStructDeclaration(node)) {
        // early exit via exception if cancellation was requested
        this.options.cancellationToken?.throwIfCancellationRequested();
      }
      const incrementedType = TypeScriptLinterConfig.incrementOnlyTokens.get(node.kind);
      if (incrementedType !== undefined) {
        this.incrementCounters(node, incrementedType);
      } else {
        const handler = this.handlersMap.get(node.kind);
        if (handler !== undefined) {

          /*
           * possibly requested cancellation will be checked in a limited number of handlers
           * checked nodes are selected as construct nodes, similar to how TSC does
           */
          handler.call(this, node);
        }
      }
    };
    const stopCondition = (node: ts.Node): boolean => {
      if (!node) {
        return true;
      }
      if (this.options.incrementalLintInfo?.shouldSkipCheck(node)) {
        return true;
      }
      // Skip synthetic constructor in Struct declaration.
      if (node.parent && isStructDeclaration(node.parent) && ts.isConstructorDeclaration(node)) {
        return true;
      }
      if (TypeScriptLinterConfig.terminalTokens.has(node.kind)) {
        return true;
      }
      return false;
    };
    forEachNodeInSubtree(sf, callback, stopCondition);
  }

  private countInterfaceExtendsDifferentPropertyTypes(
    node: ts.Node,
    prop2type: Map<string, string>,
    propName: string,
    type: ts.TypeNode | undefined
  ): void {
    if (type) {
      const methodType = type.getText();
      const propType = prop2type.get(propName);
      if (!propType) {
        prop2type.set(propName, methodType);
      } else if (propType !== methodType) {
        this.incrementCounters(node, FaultID.IntefaceExtendDifProps);
      }
    }
  }

  private countDeclarationsWithDuplicateName(tsNode: ts.Node, tsDeclNode: ts.Node, tsDeclKind?: ts.SyntaxKind): void {
    const symbol = this.tsTypeChecker.getSymbolAtLocation(tsNode);

    /*
     * If specific declaration kind is provided, check against it.
     * Otherwise, use syntax kind of corresponding declaration node.
     */
    if (!!symbol && TsUtils.symbolHasDuplicateName(symbol, tsDeclKind ?? tsDeclNode.kind)) {
      this.incrementCounters(tsDeclNode, FaultID.DeclWithDuplicateName);
    }
  }

  private countClassMembersWithDuplicateName(tsClassDecl: ts.ClassDeclaration): void {
    for (const currentMember of tsClassDecl.members) {
      if (this.tsUtils.classMemberHasDuplicateName(currentMember, tsClassDecl, false)) {
        this.incrementCounters(currentMember, FaultID.DeclWithDuplicateName);
      }
    }
  }

  private isPrototypePropertyAccess(
    tsPropertyAccess: ts.PropertyAccessExpression,
    propAccessSym: ts.Symbol | undefined,
    baseExprSym: ts.Symbol | undefined,
    baseExprType: ts.Type
  ): boolean {
    if (!(ts.isIdentifier(tsPropertyAccess.name) && tsPropertyAccess.name.text === 'prototype')) {
      return false;
    }

    // #13600: Relax prototype check when expression comes from interop.
    let curPropAccess: ts.Node = tsPropertyAccess;
    while (curPropAccess && ts.isPropertyAccessExpression(curPropAccess)) {
      const baseExprSym = this.tsUtils.trueSymbolAtLocation(curPropAccess.expression);
      if (this.tsUtils.isLibrarySymbol(baseExprSym)) {
        return false;
      }
      curPropAccess = curPropAccess.expression;
    }

    if (ts.isIdentifier(curPropAccess) && curPropAccess.text !== 'prototype') {
      const type = this.tsTypeChecker.getTypeAtLocation(curPropAccess);
      if (TsUtils.isAnyType(type)) {
        return false;
      }
    }

    // Check if property symbol is 'Prototype'
    if (TsUtils.isPrototypeSymbol(propAccessSym)) {
      return true;
    }
    // Check if symbol of LHS-expression is Class or Function.
    if (TsUtils.isTypeSymbol(baseExprSym) || TsUtils.isFunctionSymbol(baseExprSym)) {
      return true;
    }

    /*
     * Check if type of LHS expression Function type or Any type.
     * The latter check is to cover cases with multiple prototype
     * chain (as the 'Prototype' property should be 'Any' type):
     *      X.prototype.prototype.prototype = ...
     */
    const baseExprTypeNode = this.tsTypeChecker.typeToTypeNode(baseExprType, undefined, ts.NodeBuilderFlags.None);
    return baseExprTypeNode && ts.isFunctionTypeNode(baseExprTypeNode) || TsUtils.isAnyType(baseExprType);
  }

  private interfaceInheritanceLint(node: ts.Node, heritageClauses: ts.NodeArray<ts.HeritageClause>): void {
    for (const hClause of heritageClauses) {
      if (hClause.token !== ts.SyntaxKind.ExtendsKeyword) {
        continue;
      }
      const prop2type = new Map<string, string>();
      for (const tsTypeExpr of hClause.types) {
        const tsExprType = this.tsTypeChecker.getTypeAtLocation(tsTypeExpr.expression);
        if (tsExprType.isClass()) {
          this.incrementCounters(tsTypeExpr, FaultID.InterfaceExtendsClass);
        } else if (tsExprType.isClassOrInterface()) {
          this.lintForInterfaceExtendsDifferentPorpertyTypes(node, tsExprType, prop2type);
        }
      }
    }
  }

  private lintForInterfaceExtendsDifferentPorpertyTypes(
    node: ts.Node,
    tsExprType: ts.Type,
    prop2type: Map<string, string>
  ): void {
    const props = tsExprType.getProperties();
    for (const p of props) {
      if (!p.declarations) {
        continue;
      }
      const decl: ts.Declaration = p.declarations[0];
      const isPropertyDecl = ts.isPropertySignature(decl) || ts.isPropertyDeclaration(decl);
      const isMethodDecl = ts.isMethodSignature(decl) || ts.isMethodDeclaration(decl);
      if (isMethodDecl || isPropertyDecl) {
        this.countInterfaceExtendsDifferentPropertyTypes(node, prop2type, p.name, decl.type);
      }
    }
  }

  private handleObjectLiteralExpression(node: ts.Node): void {
    const objectLiteralExpr = node as ts.ObjectLiteralExpression;
    // If object literal is a part of destructuring assignment, then don't process it further.
    if (TsUtils.isDestructuringAssignmentLHS(objectLiteralExpr)) {
      return;
    }

    const objectLiteralType = this.tsTypeChecker.getContextualType(objectLiteralExpr);
    if (objectLiteralType && this.tsUtils.typeContainsSendableClassOrInterface(objectLiteralType)) {
      this.incrementCounters(node, FaultID.SendableObjectInitialization);
    } else if (
      // issue 13082: Allow initializing struct instances with object literal.
      !this.tsUtils.isStructObjectInitializer(objectLiteralExpr) &&
      !this.tsUtils.isDynamicLiteralInitializer(objectLiteralExpr) &&
      !this.tsUtils.isObjectLiteralAssignable(objectLiteralType, objectLiteralExpr)
    ) {
      const autofix = this.autofixer?.fixUntypedObjectLiteral(objectLiteralExpr, objectLiteralType);
      this.incrementCounters(node, FaultID.ObjectLiteralNoContextType, autofix);
    }

    if (this.options.arkts2) {
      this.handleObjectLiteralProperties(objectLiteralType, objectLiteralExpr);
    }
  }

  static ifValidObjectLiteralProperty(
    prop: ts.ObjectLiteralElementLike,
    objLitExpr: ts.ObjectLiteralExpression
  ): boolean {
    return (
      ts.isPropertyAssignment(prop) ||
      ts.isShorthandPropertyAssignment(prop) &&
        (ts.isCallExpression(objLitExpr.parent) || ts.isNewExpression(objLitExpr.parent))
    );
  }

  private handleObjectLiteralProperties(
    objectLiteralType: ts.Type | undefined,
    objectLiteralExpr: ts.ObjectLiteralExpression
  ): void {
    let objLiteralAutofix: Autofix[] | undefined;
    const invalidProps = objectLiteralExpr.properties.filter((prop) => {
      return !TypeScriptLinter.ifValidObjectLiteralProperty(prop, objectLiteralExpr);
    });

    if (
      invalidProps.some((prop) => {
        return ts.isMethodDeclaration(prop) || ts.isAccessor(prop);
      })
    ) {
      objLiteralAutofix = this.autofixer?.fixTypedObjectLiteral(objectLiteralExpr, objectLiteralType);
    }

    for (const prop of invalidProps) {
      const autofix = ts.isShorthandPropertyAssignment(prop) ?
        this.autofixer?.fixShorthandPropertyAssignment(prop) :
        objLiteralAutofix;
      this.incrementCounters(prop, FaultID.ObjectLiteralProperty, autofix);
    }
  }

  private handleArrayLiteralExpression(node: ts.Node): void {

    /*
     * If array literal is a part of destructuring assignment, then
     * don't process it further.
     */
    if (TsUtils.isDestructuringAssignmentLHS(node as ts.ArrayLiteralExpression)) {
      return;
    }
    const arrayLitNode = node as ts.ArrayLiteralExpression;
    const arrayLitType = this.tsTypeChecker.getContextualType(arrayLitNode);
    if (arrayLitType && this.tsUtils.typeContainsSendableClassOrInterface(arrayLitType)) {
      this.incrementCounters(node, FaultID.SendableObjectInitialization);
      return;
    }

    this.checkArrayElementsAndReportErrors(node, arrayLitNode, arrayLitType);

    this.handleObjectLiteralAssignmentToClass(arrayLitNode);
  }

  private checkArrayElementsAndReportErrors(
    node: ts.Node,
    arrayLitNode: ts.ArrayLiteralExpression,
    arrayLitType: undefined | ts.Type
  ): void {
    const parent = arrayLitNode.parent;
    const arrayLitElements = arrayLitNode.elements;
    const arrayElementIsEmpty = arrayLitElements.length === 0;
    let emptyContextTypeForArrayLiteral = false;

    /*
     * check that array literal consists of inferrable types
     * e.g. there is no element which is untyped object literals
     */
    const isPromiseEmptyArray = this.checkPromiseEmptyArray(parent, arrayElementIsEmpty);
    const isEmptyArray = this.options.arkts2 && !arrayLitType && arrayElementIsEmpty;
    if (isPromiseEmptyArray) {
      this.incrementCounters(arrayLitNode, FaultID.NosparseArray);
    } else if (isEmptyArray) {
      this.incrementCounters(node, FaultID.NosparseArray);
    }

    for (const element of arrayLitElements) {
      const elementContextType = this.tsTypeChecker.getContextualType(element);
      if (ts.isObjectLiteralExpression(element)) {
        if (
          !this.tsUtils.isDynamicLiteralInitializer(arrayLitNode) &&
          !this.tsUtils.isObjectLiteralAssignable(elementContextType, element)
        ) {
          emptyContextTypeForArrayLiteral = true;
          break;
        }
      }
      if (elementContextType) {
        this.checkAssignmentMatching(element, elementContextType, element, true);
      }
      if (this.options.arkts2 && ts.isOmittedExpression(element)) {
        this.incrementCounters(element, FaultID.NosparseArray);
      }
    }
    if (emptyContextTypeForArrayLiteral) {
      this.incrementCounters(node, FaultID.ArrayLiteralNoContextType);
    }
  }

  private checkPromiseEmptyArray(parent: ts.Node, arrayElementIsEmpty: boolean): boolean {
    if (this.options.arkts2 && ts.isCallExpression(parent) && arrayElementIsEmpty) {
      const callExpr = parent;
      const methodName = TypeScriptLinter.getPromiseMethodName(callExpr.expression);
      if (methodName && PROMISE_METHODS.has(methodName)) {
        return true;
      }
      return false;
    }
    return false;
  }

  private static getPromiseMethodName(node: ts.Expression): string | undefined {
    if (ts.isPropertyAccessExpression(node) && ts.isIdentifier(node.expression) && node.expression.text === 'Promise') {
      return node.name.text;
    }
    return undefined;
  }

  private handleStructDeclaration(node: ts.StructDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }
    this.handleStructDeclarationForLayout(node);
    this.handleInvalidIdentifier(node);
  }

  private handleParameter(node: ts.Node): void {
    const tsParam = node as ts.ParameterDeclaration;
    TsUtils.getDecoratorsIfInSendableClass(tsParam)?.forEach((decorator) => {
      this.incrementCounters(decorator, FaultID.SendableClassDecorator);
    });
    this.handleDeclarationDestructuring(tsParam);
    this.handleDeclarationInferredType(tsParam);
    this.handleInvalidIdentifier(tsParam);
    this.handleSdkGlobalApi(tsParam);
    const typeNode = tsParam.type;
    if (this.options.arkts2 && typeNode && TsUtils.typeContainsVoid(typeNode)) {
      const autofix = this.autofixer?.fixLimitedVoidType(tsParam);
      this.incrementCounters(typeNode, FaultID.LimitedVoidType, autofix);
    }
    this.handlePropertyDescriptorInScenarios(tsParam);
  }

  private handleEnumDeclaration(node: ts.Node): void {
    const enumNode = node as ts.EnumDeclaration;
    this.countDeclarationsWithDuplicateName(enumNode.name, enumNode);
    const enumSymbol = this.tsUtils.trueSymbolAtLocation(enumNode.name);
    if (!enumSymbol) {
      return;
    }
    const enumDecls = enumSymbol.getDeclarations();
    if (!enumDecls) {
      return;
    }
    if (this.options.arkts2) {
      this.handleInvalidIdentifier(enumNode);
    }

    /*
     * Since type checker merges all declarations with the same name
     * into one symbol, we need to check that there's more than one
     * enum declaration related to that specific symbol.
     * See 'countDeclarationsWithDuplicateName' method for details.
     */
    let enumDeclCount = 0;
    const enumDeclsInFile: ts.Declaration[] = [];
    const nodeSrcFile = enumNode.getSourceFile();
    for (const decl of enumDecls) {
      if (decl.kind === ts.SyntaxKind.EnumDeclaration) {
        if (nodeSrcFile === decl.getSourceFile()) {
          enumDeclsInFile.push(decl);
        }
        enumDeclCount++;
      }
    }

    if (enumDeclCount > 1) {
      const autofix = this.autofixer?.fixEnumMerging(enumSymbol, enumDeclsInFile);
      this.incrementCounters(node, FaultID.EnumMerging, autofix);
    }
  }

  private handleInterfaceDeclaration(node: ts.Node): void {
    // early exit via exception if cancellation was requested
    this.options.cancellationToken?.throwIfCancellationRequested();

    const interfaceNode = node as ts.InterfaceDeclaration;

    if (this.options.arkts2) {
      this.handleInvalidIdentifier(interfaceNode);
    }

    const iSymbol = this.tsUtils.trueSymbolAtLocation(interfaceNode.name);
    const iDecls = iSymbol ? iSymbol.getDeclarations() : null;
    if (iDecls) {

      /*
       * Since type checker merges all declarations with the same name
       * into one symbol, we need to check that there's more than one
       * interface declaration related to that specific symbol.
       * See 'countDeclarationsWithDuplicateName' method for details.
       */
      let iDeclCount = 0;
      for (const decl of iDecls) {
        if (decl.kind === ts.SyntaxKind.InterfaceDeclaration) {
          iDeclCount++;
        }
      }
      if (iDeclCount > 1) {
        this.incrementCounters(node, FaultID.InterfaceMerging);
      }
    }
    if (interfaceNode.heritageClauses) {
      this.interfaceInheritanceLint(node, interfaceNode.heritageClauses);
    }
    this.countDeclarationsWithDuplicateName(interfaceNode.name, interfaceNode);
  }

  private handleTryStatement(node: ts.TryStatement): void {
    if (!this.options.arkts2) {
      return;
    }

    for (const stmt of node.tryBlock.statements) {
      if (!ts.isExpressionStatement(stmt)) {
        continue;
      }
      const callExpr = stmt.expression;
      if (!ts.isCallExpression(callExpr)) {
        continue;
      }
      const ident = callExpr.expression;
      if (!ts.isIdentifier(ident)) {
        continue;
      }

      this.handleTsInterop(ident, () => {
        this.tsFunctionInteropHandler(callExpr);
      });

      this.handleJsInterop(ident, () => {
        this.jsFunctionInteropHandler(callExpr);
      });
    }
  }

  private tsFunctionInteropHandler(callExpr: ts.CallExpression): void {
    this.checkInteropFunctionThrows(callExpr, FaultID.InteropTSFunctionInvoke);
  }

  private jsFunctionInteropHandler(callExpr: ts.CallExpression): void {
    this.checkInteropFunctionThrows(callExpr, FaultID.InteropJSFunctionInvoke);
  }

  private checkInteropFunctionThrows(callExpr: ts.CallExpression, faultId: FaultID): void {
    const signature = this.tsTypeChecker.getResolvedSignature(callExpr);
    if (!signature) {
      return;
    }

    if (!signature.declaration) {
      return;
    }

    const functionSymbol = this.getFunctionSymbol(signature.declaration);
    const functionDeclaration = functionSymbol?.valueDeclaration;
    if (!functionDeclaration) {
      return;
    }

    if (!TypeScriptLinter.isFunctionLike(functionDeclaration)) {
      return;
    }
    if (this.containsThrowNonError(functionDeclaration)) {
      this.incrementCounters(callExpr, faultId);
    }
  }

  private containsThrowNonError(node: ts.FunctionDeclaration | ts.MethodDeclaration | ts.FunctionExpression): boolean {
    if (!node.body) {
      return false;
    }

    const statements = node.body.statements;
    for (const stmt of statements) {
      if (!ts.isThrowStatement(stmt)) {
        continue;
      }
      return this.tsUtils.checkStatementForErrorClass(stmt);
    }
    return false;
  }

  private handleThrowStatement(node: ts.Node): void {
    const throwStmt = node as ts.ThrowStatement;
    const throwExprType = this.tsTypeChecker.getTypeAtLocation(throwStmt.expression);
    if (
      !throwExprType.isClassOrInterface() ||
      !this.tsUtils.isOrDerivedFrom(throwExprType, this.tsUtils.isStdErrorType)
    ) {
      this.incrementCounters(node, FaultID.ThrowStatement);
    }
  }

  private checkForLoopDestructuring(forInit: ts.ForInitializer): void {
    if (ts.isVariableDeclarationList(forInit) && forInit.declarations.length === 1) {
      const varDecl = forInit.declarations[0];
      if (
        this.options.useRtLogic &&
        (ts.isArrayBindingPattern(varDecl.name) || ts.isObjectBindingPattern(varDecl.name))
      ) {
        this.incrementCounters(varDecl, FaultID.DestructuringDeclaration);
      }
    }
    if (ts.isArrayLiteralExpression(forInit) || ts.isObjectLiteralExpression(forInit)) {
      this.incrementCounters(forInit, FaultID.DestructuringAssignment);
    }
  }

  /*
   * this should report the point of access to the array
   * and also should report the identifier type
   */
  private checkElementAccessOfArray(statement: ts.Node): ArrayAccess | false {
    if (ts.isElementAccessExpression(statement)) {
      return this.isElementAccessOfArray(statement);
    }

    for (const children of statement.getChildren()) {
      return this.checkElementAccessOfArray(children);
    }
    return false;
  }

  private isElementAccessOfArray(expr: ts.ElementAccessExpression): false | ArrayAccess {
    if (!ts.isIdentifier(expr.expression)) {
      return false;
    }
    const type = this.tsTypeChecker.getTypeAtLocation(expr.expression);
    if (!this.tsUtils.isArray(type)) {
      return false;
    }
    const accessArgument = expr.argumentExpression;
    if (ts.isNumericLiteral(accessArgument)) {
      return {
        pos: expr.getEnd(),
        accessingIdentifier: NUMBER_LITERAL,
        arrayIdent: expr.expression
      };
    }

    if (ts.isIdentifier(accessArgument)) {
      return {
        pos: expr.getEnd(),
        accessingIdentifier: accessArgument,
        arrayIdent: expr.expression
      };
    }
    return false;
  }

  private handleForStatement(node: ts.Node): void {
    const tsForStmt = node as ts.ForStatement;
    const tsForInit = tsForStmt.initializer;
    if (tsForInit) {
      this.checkForLoopDestructuring(tsForInit);
    }
  }

  private checkConditionForArrayAccess(condition: ts.Expression, arraySymbol: ts.Symbol): UncheckedIdentifier {
    if (!ts.isBinaryExpression(condition)) {
      return undefined;
    }
    const { left, right } = condition;

    if (ts.isBinaryExpression(left)) {
      return this.checkConditionForArrayAccess(left, arraySymbol);
    }
    if (ts.isBinaryExpression(right)) {
      return this.checkConditionForArrayAccess(right, arraySymbol);
    }

    if (this.isArrayLengthAccess(left, arraySymbol)) {
      if (ts.isNumericLiteral(right)) {
        return NUMBER_LITERAL;
      }
      if (!ts.isIdentifier(right)) {
        return undefined;
      }
      return right;
    }

    if (this.isArrayLengthAccess(right, arraySymbol)) {
      if (ts.isNumericLiteral(left)) {
        return NUMBER_LITERAL;
      }
      if (!ts.isIdentifier(left)) {
        return undefined;
      }
      return left;
    }

    return undefined;
  }

  private isArrayLengthAccess(expr: ts.Expression, arraySymbol: ts.Symbol): boolean {
    if (!ts.isPropertyAccessExpression(expr)) {
      return false;
    }
    if (this.tsUtils.trueSymbolAtLocation(expr.expression) !== arraySymbol) {
      return false;
    }
    if (expr.name.text !== 'length') {
      return false;
    }

    return true;
  }

  private checkBodyHasArrayAccess(loopBody: ts.Block): ArrayAccess | undefined {
    let arrayAccessResult: undefined | ArrayAccess;
    // check if this element access expression is of an array.
    for (const child of loopBody.statements) {
      const result = this.checkElementAccessOfArray(child);
      if (!result) {
        continue;
      }
      arrayAccessResult = result;
    }
    return arrayAccessResult;
  }

  private handleForInStatement(node: ts.Node): void {
    const tsForInStmt = node as ts.ForInStatement;
    const tsForInInit = tsForInStmt.initializer;
    this.checkForLoopDestructuring(tsForInInit);
    this.incrementCounters(node, FaultID.ForInStatement);
  }

  private handleForOfStatement(node: ts.Node): void {
    const tsForOfStmt = node as ts.ForOfStatement;
    const tsForOfInit = tsForOfStmt.initializer;
    this.checkForLoopDestructuring(tsForOfInit);
    this.handleForOfJsArray(tsForOfStmt);
  }

  private updateDataSdkJsonInfo(importDeclNode: ts.ImportDeclaration, importClause: ts.ImportClause): void {
    const sdkInfo = TypeScriptLinter.pathMap.get(importDeclNode.moduleSpecifier.getText());
    if (!sdkInfo) {
      return;
    }
    if (importClause.name) {
      const importClauseName = importClause.name.text;
      sdkInfo.forEach((info) => {
        TypeScriptLinter.addOrUpdateData(this.interfaceMap, importClauseName, info);
      });
    }
    if (importClause.namedBindings) {
      const namedImports = importClause.namedBindings as ts.NamedImports;
      if (!namedImports.elements) {
        return;
      }
      namedImports.elements.forEach((element) => {
        const elementName = element.name.getText();
        sdkInfo.forEach((info) => {
          TypeScriptLinter.addOrUpdateData(this.interfaceMap, elementName, info);
        });
      });
    }
  }

  private handleImportDeclaration(node: ts.Node): void {
    // early exit via exception if cancellation was requested
    this.options.cancellationToken?.throwIfCancellationRequested();
    const importDeclNode = node as ts.ImportDeclaration;
    this.handleImportModule(importDeclNode);
    if (this.options.arkts2) {
      const importClause = importDeclNode.importClause;
      if (!importClause || !importClause.name && !importClause.namedBindings) {
        this.incrementCounters(node, FaultID.NoSideEffectImport);
      } else {
        this.updateDataSdkJsonInfo(importDeclNode, importClause);
      }
    }
    if (importDeclNode.parent.statements) {
      for (const stmt of importDeclNode.parent.statements) {
        if (stmt === importDeclNode) {
          break;
        }
        if (!ts.isImportDeclaration(stmt)) {
          this.incrementCounters(node, FaultID.ImportAfterStatement);
          break;
        }
      }
    }

    const expr = importDeclNode.moduleSpecifier;
    if (expr.kind === ts.SyntaxKind.StringLiteral) {
      if (importDeclNode.assertClause) {
        this.incrementCounters(importDeclNode.assertClause, FaultID.ImportAssertion);
      }
      const stringLiteral = expr as ts.StringLiteral;
      this.handleSdkSendable(stringLiteral);
    }

    // handle no side effect import in sendable module
    this.handleSharedModuleNoSideEffectImport(importDeclNode);
    this.handleInvalidIdentifier(importDeclNode);
    this.checkStdLibConcurrencyImport(importDeclNode);
    this.handleInterOpImportJs(importDeclNode);
    this.checkForDeprecatedModules(node);
  }

  private checkForDeprecatedModules(node: ts.Node): void {
    if (!ts.isImportDeclaration(node)) {
      return;
    }

    const deprecatedModules = ['@ohos.file.sendablePhotoAccessHelper'];

    const importDecl = node;
    const moduleSpecifier = importDecl.moduleSpecifier;

    if (ts.isStringLiteral(moduleSpecifier)) {
      const moduleName = moduleSpecifier.text;
      if (deprecatedModules.includes(moduleName)) {
        this.incrementCounters(moduleSpecifier, FaultID.SdkTypeQuery);
      }
    }
  }

  private handleSdkSendable(tsStringLiteral: ts.StringLiteral): void {
    if (!this.options.arkts2) {
      return;
    }

    const moduleSpecifierValue = tsStringLiteral.getText();
    const sdkInfos = TypeScriptLinter.pathMap.get(moduleSpecifierValue);

    if (!sdkInfos || sdkInfos.size === 0) {
      return;
    }
    if (moduleSpecifierValue.includes('sendable')) {
      this.incrementCounters(tsStringLiteral, FaultID.SendablePropTypeFromSdk);
    }
  }

  private handleImportModule(importDeclNode: ts.ImportDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const modulePath = importDeclNode.moduleSpecifier.getText().slice(1, -1);
    if (modulePath.startsWith('./') || modulePath.startsWith('../')) {

      /*
       * Reason for this method to check the oh module imports,
       * We do not use relative paths when importing from OhModules,
       * So we do not check the relative paths
       */
      return;
    }
    if (!importDeclNode.importClause) {
      return;
    }

    const pathParts = modulePath.split(PATH_SEPARATOR);
    const etsIdx = pathParts.indexOf(ETS_PART);

    if (this.options.wholeProjectPath) {
      if (TsUtils.checkFileExists(etsIdx !== 0, importDeclNode, modulePath, this.options.wholeProjectPath)) {
        return;
      }
    }

    if (TsUtils.isValidOhModulePath(modulePath) || !TsUtils.isOhModule(modulePath)) {
      // Valid or paths that we do not check because they are not ohModules
      return;
    }

    if (etsIdx === 0) {
      const autofix = this.autofixer?.addDefaultModuleToPath(pathParts, importDeclNode);
      this.incrementCounters(importDeclNode, FaultID.OhmUrlFullPath, autofix);
      return;
    }

    const autofix = this.autofixer?.fixImportPath(pathParts, etsIdx, importDeclNode);
    this.incrementCounters(importDeclNode, FaultID.OhmUrlFullPath, autofix);
  }

  private handleSharedModuleNoSideEffectImport(node: ts.ImportDeclaration): void {
    // check 'use shared'
    if (TypeScriptLinter.inSharedModule(node) && !node.importClause) {
      this.incrementCounters(node, FaultID.SharedNoSideEffectImport);
    }
  }

  private static inSharedModule(node: ts.Node): boolean {
    const sourceFile: ts.SourceFile = node.getSourceFile();
    const modulePath = path.normalize(sourceFile.fileName);
    if (TypeScriptLinter.sharedModulesCache.has(modulePath)) {
      return TypeScriptLinter.sharedModulesCache.get(modulePath)!;
    }
    const isSharedModule: boolean = TsUtils.isSharedModule(sourceFile);
    TypeScriptLinter.sharedModulesCache.set(modulePath, isSharedModule);
    return isSharedModule;
  }

  private handlePropertyAccessExpression(node: ts.Node): void {
    this.handleMakeObserved(node as ts.PropertyAccessExpression);
    this.handleStateStyles(node as ts.PropertyAccessExpression);
    this.handleDoubleDollar(node);
    this.handleQuotedHyphenPropsDeprecated(node as ts.PropertyAccessExpression);
    this.handleSdkTypeQuery(node as ts.PropertyAccessExpression);
    this.checkUnionTypes(node as ts.PropertyAccessExpression);
    this.handleLimitedVoidTypeFromSdkOnPropertyAccessExpression(node as ts.PropertyAccessExpression);
    this.checkDepricatedIsConcurrent(node as ts.PropertyAccessExpression);
    this.propertyAccessExpressionForBuiltin(node as ts.PropertyAccessExpression);
    this.checkConstrutorAccess(node as ts.PropertyAccessExpression);
    this.handleTaskPoolDeprecatedUsages(node as ts.PropertyAccessExpression);
    this.handleNoTuplesArraysForPropertyAccessExpression(node as ts.PropertyAccessExpression);
    if (ts.isCallExpression(node.parent) && node === node.parent.expression) {
      return;
    }
    const propertyAccessNode = node as ts.PropertyAccessExpression;
    const exprSym = this.tsUtils.trueSymbolAtLocation(propertyAccessNode);
    const baseExprSym = this.tsUtils.trueSymbolAtLocation(propertyAccessNode.expression);
    const baseExprType = this.tsTypeChecker.getTypeAtLocation(propertyAccessNode.expression);
    this.handleTsInterop(propertyAccessNode, () => {
      const type = this.tsTypeChecker.getTypeAtLocation(propertyAccessNode.expression);
      this.checkUsageOfTsTypes(type, propertyAccessNode.expression);
    });
    this.propertyAccessExpressionForInterop(propertyAccessNode);
    if (this.isPrototypePropertyAccess(propertyAccessNode, exprSym, baseExprSym, baseExprType)) {
      this.incrementCounters(propertyAccessNode.name, FaultID.Prototype);
    }
    if (
      !this.options.arkts2 &&
      !!exprSym &&
      this.tsUtils.isStdSymbolAPI(exprSym) &&
      !ALLOWED_STD_SYMBOL_API.includes(exprSym.getName())
    ) {
      this.incrementCounters(propertyAccessNode, FaultID.SymbolType);
    }
    if (this.options.advancedClassChecks && this.tsUtils.isClassObjectExpression(propertyAccessNode.expression)) {
      this.incrementCounters(propertyAccessNode.expression, FaultID.ClassAsObject);
    }
    if (!!baseExprSym && TsUtils.symbolHasEsObjectType(baseExprSym)) {
      const faultId = this.options.arkts2 ? FaultID.EsValueTypeError : FaultID.EsValueType;
      this.incrementCounters(propertyAccessNode, faultId);
    }
    if (TsUtils.isSendableFunction(baseExprType) || this.tsUtils.hasSendableTypeAlias(baseExprType)) {
      this.incrementCounters(propertyAccessNode, FaultID.SendableFunctionProperty);
    }
    this.checkFunctionProperty(propertyAccessNode, baseExprSym, baseExprType);
    this.handleSdkForConstructorFuncs(propertyAccessNode);
    this.fixJsImportPropertyAccessExpression(node);
  }

  propertyAccessExpressionForBuiltin(decl: ts.PropertyAccessExpression): void {
    if (this.options.arkts2) {
      this.handleSymbolIterator(decl);
      this.handleGetOwnPropertyNames(decl);
      this.handlePropertyDescriptorInScenarios(decl);
    }
  }

  private isJsRelated(node: ts.Expression): boolean {
    if (this.tsUtils.isJsImport(node)) {
      return true;
    }

    if (ts.isNewExpression(node)) {
      return this.tsUtils.isJsImport(node.expression);
    }

    if (ts.isIdentifier(node)) {
      const symbol = this.tsUtils.trueSymbolAtLocation(node);
      if (!symbol) {
        return false;
      }

      const declarations = symbol.getDeclarations();
      if (!declarations || declarations.length === 0) {
        return false;
      }

      for (const declaration of declarations) {
        if (ts.isVariableDeclaration(declaration) && declaration.initializer) {
          return this.isJsRelated(declaration.initializer);
        }
      }
    }

    return false;
  }

  propertyAccessExpressionForInterop(propertyAccessNode: ts.PropertyAccessExpression): void {
    if (!this.useStatic || !this.options.arkts2) {
      return;
    }

    const getFirstObjectNode = (propertyAccessNode: ts.PropertyAccessExpression): ts.Expression => {
      let current: ts.Expression = propertyAccessNode.expression;
      while (ts.isPropertyAccessExpression(current)) {
        current = current.expression;
      }

      return current;
    };

    const firstObjNode = getFirstObjectNode(propertyAccessNode);
    const isJsObject = this.isJsRelated(firstObjNode);
    if (!isJsObject) {
      return;
    }

    if (ts.isBinaryExpression(propertyAccessNode.parent)) {
      const isAssignment = propertyAccessNode.parent.operatorToken.kind === ts.SyntaxKind.EqualsToken;
      const autofix = isAssignment ?
        this.autofixer?.fixInteropBinaryExpression(propertyAccessNode.parent) :
        this.autofixer?.fixInteropPropertyAccessExpression(propertyAccessNode);

      this.incrementCounters(
        isAssignment ? propertyAccessNode.parent : propertyAccessNode,
        FaultID.InteropObjectProperty,
        autofix
      );
    } else {
      const autofix = this.autofixer?.fixInteropPropertyAccessExpression(propertyAccessNode);
      this.incrementCounters(propertyAccessNode, FaultID.InteropObjectProperty, autofix);
    }
  }

  private checkDepricatedIsConcurrent(node: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    if (!ts.isCallExpression(node.parent)) {
      return;
    }
    const methodName = node.name.getText();

    if (methodName !== ISCONCURRENT) {
      return;
    }
    const symbol = this.tsUtils.trueSymbolAtLocation(node.expression);
    if (!symbol) {
      return;
    }
    if (symbol.name === TASKPOOL) {
      const decl = TsUtils.getDeclaration(symbol);

      if (!decl) {
        return;
      }

      const sourceFile = decl.getSourceFile();
      const fileName = path.basename(sourceFile.fileName);

      if (
        TASKPOOL_MODULES.some((moduleName) => {
          return fileName.startsWith(moduleName) && (fileName.endsWith(D_TS) || fileName.endsWith(D_ETS));
        })
      ) {
        this.incrementCounters(node.name, FaultID.IsConcurrentDeprecated);
      }
    }
  }

  checkFunctionProperty(
    node: ts.PropertyAccessExpression,
    baseExprSym: ts.Symbol | undefined,
    baseExprType: ts.Type
  ): void {
    if (!this.options.arkts2) {
      return;
    }

    if (
      baseExprSym && TsUtils.isFunctionSymbol(baseExprSym) ||
      this.tsUtils.isStdFunctionType(baseExprType) ||
      TsUtils.isFunctionalType(baseExprType) && TsUtils.isAnonymousType(baseExprType)
    ) {
      this.incrementCounters(node.expression, FaultID.PropertyDeclOnFunction);
    }
  }

  private checkUsageOfTsTypes(baseType: ts.Type, node: ts.Node): void {
    const typeString = this.tsTypeChecker.typeToString(baseType);
    if (
      TsUtils.isAnyType(baseType) ||
      TsUtils.isUnknownType(baseType) ||
      this.tsUtils.isStdFunctionType(baseType) ||
      typeString === 'symbol'
    ) {
      this.incrementCounters(node, FaultID.InteropDirectAccessToTSTypes);
    }
  }

  checkUnionTypes(propertyAccessNode: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    const baseExprType = this.tsTypeChecker.getTypeAtLocation(propertyAccessNode.expression);
    if (!baseExprType.isUnion() || this.tsTypeChecker.typeToString(baseExprType) === 'ArrayBufferLike') {
      return;
    }
    const allType = baseExprType.types;
    const commonPropertyType = allType.filter((type) => {
      return this.tsUtils.findProperty(type, propertyAccessNode.name.getText()) !== undefined;
    });
    const typeMap = new Map();
    if (commonPropertyType.length === allType.length) {
      allType.forEach((type) => {
        this.handleTypeMember(type, propertyAccessNode.name.getText(), typeMap);
      });
      if (typeMap.size > 1) {
        this.incrementCounters(propertyAccessNode, FaultID.AvoidUnionTypes);
      }
    }
  }

  private handleTypeMember(
    type: ts.Type,
    memberName: string,
    typeMap: Map<string | ts.Type | undefined, string>
  ): void {
    const propertySymbol = this.tsUtils.findProperty(type, memberName);
    if (!propertySymbol?.declarations) {
      return;
    }
    const propertyType = this.tsTypeChecker.getTypeOfSymbolAtLocation(propertySymbol, propertySymbol.declarations[0]);
    const symbol = propertySymbol.valueDeclaration;
    if (!symbol) {
      return;
    }
    if (ts.isMethodDeclaration(symbol)) {
      const returnType = this.getMethodReturnType(propertySymbol);
      typeMap.set(returnType, memberName);
    } else {
      typeMap.set(propertyType, memberName);
    }
  }

  private getMethodReturnType(symbol: ts.Symbol): string | undefined {
    const declaration = symbol.valueDeclaration ?? (symbol.declarations?.[0] as ts.Node | undefined);
    if (!declaration) {
      return undefined;
    }
    const methodType = this.tsTypeChecker.getTypeOfSymbolAtLocation(symbol, declaration);
    const signatures = methodType.getCallSignatures();
    if (signatures.length === 0) {
      return 'void';
    }
    const returnType = signatures[0].getReturnType();
    return this.tsTypeChecker.typeToString(returnType);
  }

  private handleLiteralAsPropertyName(node: ts.PropertyDeclaration | ts.PropertySignature): void {
    const propName = node.name;
    if (!!propName && (ts.isNumericLiteral(propName) || this.options.arkts2 && ts.isStringLiteral(propName))) {
      const autofix = this.autofixer?.fixLiteralAsPropertyNamePropertyName(propName);
      this.incrementCounters(node.name, FaultID.LiteralAsPropertyName, autofix);
    }
  }

  private handlePropertyDeclaration(node: ts.PropertyDeclaration): void {
    const propName = node.name;
    this.handleLiteralAsPropertyName(node);
    const decorators = ts.getDecorators(node);
    this.filterOutDecoratorsDiagnostics(
      decorators,
      this.options.useRtLogic ? NON_INITIALIZABLE_PROPERTY_DECORATORS : NON_INITIALIZABLE_PROPERTY_DECORATORS_TSC,
      { begin: propName.getStart(), end: propName.getStart() },
      PROPERTY_HAS_NO_INITIALIZER_ERROR_CODE
    );
    const classDecorators = ts.getDecorators(node.parent);
    const propType = node.type?.getText();
    if (this.options.arkts2 && node.type && TsUtils.typeContainsVoid(node.type)) {
      const autofix = this.autofixer?.fixLimitedVoidType(node);
      this.incrementCounters(node.type, FaultID.LimitedVoidType, autofix);
    }
    this.filterOutDecoratorsDiagnostics(
      classDecorators,
      NON_INITIALIZABLE_PROPERTY_CLASS_DECORATORS,
      { begin: propName.getStart(), end: propName.getStart() },
      PROPERTY_HAS_NO_INITIALIZER_ERROR_CODE,
      propType
    );
    if (node.type && node.initializer) {
      this.checkAssignmentMatching(node, this.tsTypeChecker.getTypeAtLocation(node.type), node.initializer, true);
      this.checkFunctionTypeCompatible(node.type, node.initializer);
    }
    this.handleDeclarationInferredType(node);
    this.handleDefiniteAssignmentAssertion(node);
    this.handleSendableClassProperty(node);
    this.checkAssignmentNumericSemanticslyPro(node);
    this.handleInvalidIdentifier(node);
    this.handleStructPropertyDecl(node);
    this.handlePropertyDeclarationForProp(node);
    this.handleSdkGlobalApi(node);
    this.handleObjectLiteralAssignmentToClass(node);
  }

  private handleSendableClassProperty(node: ts.PropertyDeclaration): void {
    const classNode = node.parent;
    if (!ts.isClassDeclaration(classNode) || !TsUtils.hasSendableDecorator(classNode)) {
      return;
    }
    const typeNode = node.type;
    if (!typeNode) {
      const autofix = this.autofixer?.fixSendableExplicitFieldType(node);
      this.incrementCounters(node, FaultID.SendableExplicitFieldType, autofix);
      return;
    }
    TsUtils.getDecoratorsIfInSendableClass(node)?.forEach((decorator) => {
      this.incrementCounters(decorator, FaultID.SendableClassDecorator);
    });
    if (!this.tsUtils.isSendableTypeNode(typeNode)) {
      this.incrementCounters(node, FaultID.SendablePropType);
    }
  }

  private handlePropertyAssignment(node: ts.PropertyAssignment): void {
    this.handleDollarBind(node);
    this.handlePropertyAssignmentForProp(node);

    this.handleQuotedHyphenPropsDeprecated(node);
    const propName = node.name;
    if (!propName || !(ts.isNumericLiteral(propName) || this.options.arkts2 && ts.isStringLiteral(propName))) {
      return;
    }

    /*
     * We can use literals as property names only when creating Record or any interop instances.
     * We can also initialize with constant string literals.
     * Assignment with string enum values is handled in handleComputedPropertyName
     */
    let isRecordObjectInitializer = false;
    let isLibraryType = false;
    let isDynamic = false;
    const objectLiteralType = this.tsTypeChecker.getContextualType(node.parent);
    if (objectLiteralType) {
      isRecordObjectInitializer = this.tsUtils.checkTypeSet(objectLiteralType, this.tsUtils.isStdRecordType);
      isLibraryType = this.tsUtils.isLibraryType(objectLiteralType);
    }

    isDynamic = isLibraryType || this.tsUtils.isDynamicLiteralInitializer(node.parent);
    if (!isRecordObjectInitializer && !isDynamic) {
      const autofix = this.autofixer?.fixLiteralAsPropertyNamePropertyAssignment(node);
      this.incrementCounters(node.name, FaultID.LiteralAsPropertyName, autofix);
    }
  }

  private static getAllClassesFromSourceFile(sourceFile: ts.SourceFile): ts.ClassDeclaration[] {
    const allClasses: ts.ClassDeclaration[] = [];
    function visit(node: ts.Node): void {
      if (ts.isClassDeclaration(node)) {
        allClasses.push(node);
      }
      ts.forEachChild(node, visit);
    }
    visit(sourceFile);
    return allClasses;
  }

  private static getAllInterfaceFromSourceFile(sourceFile: ts.SourceFile): ts.InterfaceDeclaration[] {
    const allInterfaces: ts.InterfaceDeclaration[] = [];
    function visit(node: ts.Node): void {
      if (ts.isInterfaceDeclaration(node)) {
        allInterfaces.push(node);
      }
      ts.forEachChild(node, visit);
    }
    visit(sourceFile);
    return allInterfaces;
  }

  private handlePropertySignature(node: ts.PropertySignature): void {
    this.handleInterfaceProperty(node);
    this.handleLiteralAsPropertyName(node);
    this.handleSendableInterfaceProperty(node);
    this.handleInvalidIdentifier(node);
    const typeNode = node.type;
    if (this.options.arkts2 && typeNode && typeNode.kind === ts.SyntaxKind.VoidKeyword) {
      this.incrementCounters(typeNode, FaultID.LimitedVoidType);
    }
  }

  private handleInterfaceProperty(node: ts.PropertySignature): void {
    if (this.options.arkts2 && ts.isInterfaceDeclaration(node.parent)) {
      if (node.type && ts.isFunctionTypeNode(node.type)) {
        const interfaceName = node.parent.name.getText();
        const propertyName = node.name.getText();
        const allClasses = TypeScriptLinter.getAllClassesFromSourceFile(this.sourceFile);
        const allInterfaces = TypeScriptLinter.getAllInterfaceFromSourceFile(this.sourceFile);
        this.visitClassMembers(allClasses, interfaceName, propertyName);
        this.visitInterfaceMembers(allInterfaces, interfaceName, propertyName);
      }
    }
  }

  private visitInterfaceMembers(
    interfaces: ts.InterfaceDeclaration[],
    interfaceName: string,
    propertyName: string
  ): void {
    void this;
    interfaces.some((interfaceDecl) => {
      const implementsClause = this.getExtendsClause(interfaceDecl);
      if (
        implementsClause?.types.some((type) => {
          return type.expression.getText() === interfaceName;
        })
      ) {
        this.checkInterfaceForProperty(interfaceDecl, propertyName);
      }
    });
  }

  private getExtendsClause(interfaceDecl: ts.InterfaceDeclaration): ts.HeritageClause | undefined {
    void this;
    return interfaceDecl.heritageClauses?.find((clause) => {
      return clause.token === ts.SyntaxKind.ExtendsKeyword;
    });
  }

  private checkInterfaceForProperty(interfaceDecl: ts.InterfaceDeclaration, propertyName: string): void {
    for (const member of interfaceDecl.members) {
      if (ts.isMethodSignature(member) && member.name.getText() === propertyName) {
        this.incrementCounters(member, FaultID.MethodOverridingField);
      }
    }
  }

  private getImplementsClause(classDecl: ts.ClassDeclaration): ts.HeritageClause | undefined {
    void this;
    return classDecl.heritageClauses?.find((clause) => {
      return clause.token === ts.SyntaxKind.ImplementsKeyword;
    });
  }

  private checkClassForProperty(classDecl: ts.ClassDeclaration, propertyName: string): void {
    for (const member of classDecl.members) {
      if (ts.isMethodDeclaration(member) && member.name.getText() === propertyName) {
        this.incrementCounters(member, FaultID.MethodOverridingField);
      }
    }
  }

  private visitClassMembers(classes: ts.ClassDeclaration[], interfaceName: string, propertyName: string): void {
    void this;
    classes.some((classDecl) => {
      const implementsClause = this.getImplementsClause(classDecl);
      if (
        implementsClause?.types.some((type) => {
          return type.expression.getText() === interfaceName;
        })
      ) {
        this.checkClassForProperty(classDecl, propertyName);
      }
    });
  }

  private handleSendableInterfaceProperty(node: ts.PropertySignature): void {
    const typeNode = node.type;
    if (!typeNode) {
      return;
    }
    const interfaceNode = node.parent;
    const interfaceNodeType = this.tsTypeChecker.getTypeAtLocation(interfaceNode);
    if (!ts.isInterfaceDeclaration(interfaceNode) || !this.tsUtils.isSendableClassOrInterface(interfaceNodeType)) {
      return;
    }
    if (!this.tsUtils.isSendableTypeNode(typeNode)) {
      this.incrementCounters(node, FaultID.SendablePropType);
    }
  }

  private filterOutDecoratorsDiagnostics(
    decorators: readonly ts.Decorator[] | undefined,
    expectedDecorators: readonly string[],
    range: { begin: number; end: number },
    code: number,
    propType?: string
  ): void {
    // Filter out non-initializable property decorators from strict diagnostics.
    if (this.tscStrictDiagnostics && this.sourceFile) {
      if (
        decorators?.some((decorator) => {
          const decoratorName = TsUtils.getDecoratorName(decorator);
          // special case for property of type CustomDialogController of the @CustomDialog-decorated class
          if (expectedDecorators.includes(NON_INITIALIZABLE_PROPERTY_CLASS_DECORATORS[0])) {
            return expectedDecorators.includes(decoratorName) && propType === 'CustomDialogController';
          }
          return expectedDecorators.includes(decoratorName);
        })
      ) {
        this.filterOutDiagnostics(range, code);
      }
    }
  }

  private filterOutDiagnostics(range: { begin: number; end: number }, code: number): void {
    // Filter out strict diagnostics within the given range with the given code.
    if (!this.tscStrictDiagnostics || !this.sourceFile) {
      return;
    }
    const file = path.normalize(this.sourceFile.fileName);
    const tscDiagnostics = this.tscStrictDiagnostics.get(file);
    if (tscDiagnostics) {
      const filteredDiagnostics = tscDiagnostics.filter((val) => {
        if (val.code !== code) {
          return true;
        }
        if (val.start === undefined) {
          return true;
        }
        if (val.start < range.begin) {
          return true;
        }
        if (val.start > range.end) {
          return true;
        }
        return false;
      });
      this.tscStrictDiagnostics.set(file, filteredDiagnostics);
    }
  }

  private static isClassLikeOrIface(node: ts.Node): boolean {
    return ts.isClassLike(node) || ts.isInterfaceDeclaration(node);
  }

  private handleFunctionExpression(node: ts.Node): void {
    const funcExpr = node as ts.FunctionExpression;
    const isGenerator = funcExpr.asteriskToken !== undefined;
    const [hasUnfixableReturnType, newRetTypeNode] = this.handleMissingReturnType(funcExpr);
    const autofix = this.autofixer?.fixFunctionExpression(
      funcExpr,
      newRetTypeNode,
      ts.getModifiers(funcExpr),
      isGenerator,
      hasUnfixableReturnType
    );
    this.incrementCounters(funcExpr, FaultID.FunctionExpression, autofix);
    if (isGenerator) {
      this.incrementCounters(funcExpr, FaultID.GeneratorFunction);
    }
    if (!hasPredecessor(funcExpr, TypeScriptLinter.isClassLikeOrIface)) {
      this.reportThisKeywordsInScope(funcExpr.body);
    }
    if (hasUnfixableReturnType) {
      this.incrementCounters(funcExpr, FaultID.LimitedReturnTypeInference);
    }
    this.handleLimitedVoidFunction(funcExpr);
  }

  private handleArrowFunction(node: ts.Node): void {
    const arrowFunc = node as ts.ArrowFunction;
    if (!hasPredecessor(arrowFunc, TypeScriptLinter.isClassLikeOrIface)) {
      this.reportThisKeywordsInScope(arrowFunc.body);
    }
    const contextType = this.tsTypeChecker.getContextualType(arrowFunc);
    if (!(contextType && this.tsUtils.isLibraryType(contextType))) {
      if (!arrowFunc.type) {
        this.handleMissingReturnType(arrowFunc);
      }
    }
    this.checkDefaultParamBeforeRequired(arrowFunc);
    this.handleLimitedVoidFunction(arrowFunc);
  }

  private handleFunctionDeclaration(node: ts.Node): void {
    // early exit via exception if cancellation was requested
    this.options.cancellationToken?.throwIfCancellationRequested();

    const tsFunctionDeclaration = node as ts.FunctionDeclaration;
    if (!tsFunctionDeclaration.type) {
      this.handleMissingReturnType(tsFunctionDeclaration);
    }
    if (tsFunctionDeclaration.name) {
      this.countDeclarationsWithDuplicateName(tsFunctionDeclaration.name, tsFunctionDeclaration);
    }
    if (tsFunctionDeclaration.body) {
      this.reportThisKeywordsInScope(tsFunctionDeclaration.body);
    }
    const funcDeclParent = tsFunctionDeclaration.parent;
    if (!ts.isSourceFile(funcDeclParent) && !ts.isModuleBlock(funcDeclParent)) {
      const autofix = this.autofixer?.fixNestedFunction(tsFunctionDeclaration);
      this.incrementCounters(tsFunctionDeclaration, FaultID.LocalFunction, autofix);
    }
    if (tsFunctionDeclaration.asteriskToken) {
      this.incrementCounters(node, FaultID.GeneratorFunction);
    }
    if (TsUtils.hasSendableDecoratorFunctionOverload(tsFunctionDeclaration)) {
      if (!this.isSendableDecoratorValid(tsFunctionDeclaration)) {
        return;
      }
      TsUtils.getNonSendableDecorators(tsFunctionDeclaration)?.forEach((decorator) => {
        this.incrementCounters(decorator, FaultID.SendableFunctionDecorator);
      });
      if (!TsUtils.hasSendableDecorator(tsFunctionDeclaration)) {
        const autofix = this.autofixer?.addSendableDecorator(tsFunctionDeclaration);
        this.incrementCounters(tsFunctionDeclaration, FaultID.SendableFunctionOverloadDecorator, autofix);
      }
      this.scanCapturedVarsInSendableScope(
        tsFunctionDeclaration,
        tsFunctionDeclaration,
        FaultID.SendableFunctionImportedVariables
      );
    }
    this.handleTSOverload(tsFunctionDeclaration);
    this.checkAssignmentNumericSemanticsFuntion(tsFunctionDeclaration);
    this.handleInvalidIdentifier(tsFunctionDeclaration);
    this.checkDefaultParamBeforeRequired(tsFunctionDeclaration);
    this.handleLimitedVoidFunction(tsFunctionDeclaration);
  }

  private handleMissingReturnType(
    funcLikeDecl: ts.FunctionLikeDeclaration | ts.MethodSignature
  ): [boolean, ts.TypeNode | undefined] {
    if (this.options.useRtLogic && funcLikeDecl.type) {
      return [false, funcLikeDecl.type];
    }

    // Note: Return type can't be inferred for function without body.
    const isSignature = ts.isMethodSignature(funcLikeDecl);
    if (isSignature || !funcLikeDecl.body) {
      // Ambient flag is not exposed, so we apply dirty hack to make it visible
      const isDeclareDeclaration = TsUtils.isAmbientNode(funcLikeDecl);
      if ((isSignature || isDeclareDeclaration) && !funcLikeDecl.type) {
        this.incrementCounters(funcLikeDecl, FaultID.LimitedReturnTypeInference);
      }
      return [false, undefined];
    }

    return this.tryAutofixMissingReturnType(funcLikeDecl);
  }

  private tryAutofixMissingReturnType(funcLikeDecl: ts.FunctionLikeDeclaration): [boolean, ts.TypeNode | undefined] {
    if (!funcLikeDecl.body) {
      return [false, undefined];
    }

    let autofix: Autofix[] | undefined;
    let newRetTypeNode: ts.TypeNode | undefined;
    const isFuncExpr = ts.isFunctionExpression(funcLikeDecl);

    /*
     * Currently, ArkTS can't infer return type of function, when expression
     * in the return statement is a call to a function or method whose return
     * value type is omitted. In that case, we attempt to prepare an autofix.
     */
    let hasLimitedRetTypeInference = this.hasLimitedTypeInferenceFromReturnExpr(funcLikeDecl.body);
    const tsSignature = this.tsTypeChecker.getSignatureFromDeclaration(funcLikeDecl);
    if (tsSignature) {
      const tsRetType = this.tsTypeChecker.getReturnTypeOfSignature(tsSignature);
      if (
        !tsRetType ||
        !this.options.arkts2 && TsUtils.isUnsupportedType(tsRetType) ||
        this.options.arkts2 && this.tsUtils.isUnsupportedTypeArkts2(tsRetType)
      ) {
        hasLimitedRetTypeInference = true;
      } else if (hasLimitedRetTypeInference) {
        newRetTypeNode = this.tsTypeChecker.typeToTypeNode(tsRetType, funcLikeDecl, ts.NodeBuilderFlags.None);
        if (this.autofixer !== undefined && newRetTypeNode && !isFuncExpr) {
          autofix = this.autofixer.fixMissingReturnType(funcLikeDecl, newRetTypeNode);
        }
      }
    }

    /*
     * Don't report here if in function expression context.
     * See handleFunctionExpression for details.
     */
    if (hasLimitedRetTypeInference && !isFuncExpr) {
      this.incrementCounters(funcLikeDecl, FaultID.LimitedReturnTypeInference, autofix);
    }

    return [hasLimitedRetTypeInference && !newRetTypeNode, newRetTypeNode];
  }

  private hasLimitedTypeInferenceFromReturnExpr(funBody: ts.ConciseBody): boolean {
    let hasLimitedTypeInference = false;
    const callback = (node: ts.Node): void => {
      if (hasLimitedTypeInference) {
        return;
      }
      if (
        ts.isReturnStatement(node) &&
        node.expression &&
        this.tsUtils.isCallToFunctionWithOmittedReturnType(TsUtils.unwrapParenthesized(node.expression))
      ) {
        hasLimitedTypeInference = true;
      }
    };
    // Don't traverse other nested function-like declarations.
    const stopCondition = (node: ts.Node): boolean => {
      return (
        ts.isFunctionDeclaration(node) ||
        ts.isFunctionExpression(node) ||
        ts.isMethodDeclaration(node) ||
        ts.isAccessor(node) ||
        ts.isArrowFunction(node)
      );
    };
    if (ts.isBlock(funBody)) {
      forEachNodeInSubtree(funBody, callback, stopCondition);
    } else {
      const tsExpr = TsUtils.unwrapParenthesized(funBody);
      hasLimitedTypeInference = this.tsUtils.isCallToFunctionWithOmittedReturnType(tsExpr);
    }
    return hasLimitedTypeInference;
  }

  private isValidTypeForUnaryArithmeticOperator(type: ts.Type): boolean {
    const typeFlags = type.getFlags();
    const numberLiteralFlags = ts.TypeFlags.BigIntLiteral | ts.TypeFlags.NumberLiteral;
    const numberLikeFlags = ts.TypeFlags.BigIntLike | ts.TypeFlags.NumberLike;
    const isNumberLike = !!(typeFlags & (numberLiteralFlags | numberLikeFlags));

    const isAllowedNumericType = this.tsUtils.isStdBigIntType(type) || this.tsUtils.isStdNumberType(type);

    return isNumberLike || isAllowedNumericType;
  }

  private handleInteropOperand(tsUnaryArithm: ts.PrefixUnaryExpression): void {
    const processPropertyAccess = (expr: ts.PropertyAccessExpression | ts.ParenthesizedExpression): void => {
      const propertyAccess = ts.isParenthesizedExpression(expr) ? expr.expression : expr;

      if (ts.isPropertyAccessExpression(propertyAccess)) {
        const exprSym = this.tsUtils.trueSymbolAtLocation(propertyAccess.expression);
        const declaration = exprSym?.declarations?.[0];
        this.checkAndProcessDeclaration(declaration, tsUnaryArithm);
      }
    };

    if (ts.isPropertyAccessExpression(tsUnaryArithm.operand) || ts.isParenthesizedExpression(tsUnaryArithm.operand)) {
      processPropertyAccess(tsUnaryArithm.operand);
    }
  }

  private checkAndProcessDeclaration(
    declaration: ts.Declaration | undefined,
    tsUnaryArithm: ts.PrefixUnaryExpression
  ): void {
    if (declaration?.getSourceFile().fileName.endsWith(EXTNAME_JS)) {
      if (
        [
          ts.SyntaxKind.PlusToken,
          ts.SyntaxKind.ExclamationToken,
          ts.SyntaxKind.TildeToken,
          ts.SyntaxKind.MinusToken
        ].includes(tsUnaryArithm.operator)
      ) {
        const autofix = this.autofixer?.fixInteropInterfaceConvertNum(tsUnaryArithm);
        this.incrementCounters(tsUnaryArithm, FaultID.InteropNoHaveNum, autofix);
      }
    }
  }

  private handlePostfixUnaryExpression(node: ts.Node): void {
    const unaryExpr = node as ts.PostfixUnaryExpression;
    if (unaryExpr.operator === ts.SyntaxKind.PlusPlusToken || unaryExpr.operator === ts.SyntaxKind.MinusMinusToken) {
      this.checkAutoIncrementDecrement(unaryExpr);
    }
  }

  private handlePrefixUnaryExpression(node: ts.Node): void {
    const tsUnaryArithm = node as ts.PrefixUnaryExpression;
    if (this.useStatic && this.options.arkts2) {
      const tsUnaryArithm = node as ts.PrefixUnaryExpression;
      this.handleInteropOperand(tsUnaryArithm);
    }
    const tsUnaryOp = tsUnaryArithm.operator;
    const tsUnaryOperand = tsUnaryArithm.operand;
    if (
      tsUnaryOp === ts.SyntaxKind.PlusToken ||
      tsUnaryOp === ts.SyntaxKind.MinusToken ||
      tsUnaryOp === ts.SyntaxKind.TildeToken
    ) {
      const tsOperatndType = this.tsTypeChecker.getTypeAtLocation(tsUnaryOperand);
      const isTilde = tsUnaryOp === ts.SyntaxKind.TildeToken;
      const isInvalidTilde =
        isTilde && ts.isNumericLiteral(tsUnaryOperand) && !this.tsUtils.isIntegerConstantValue(tsUnaryOperand);
      if (!this.isValidTypeForUnaryArithmeticOperator(tsOperatndType) || isInvalidTilde) {
        this.incrementCounters(node, FaultID.UnaryArithmNotNumber);
      }
    }
    if (
      tsUnaryArithm.operator === ts.SyntaxKind.PlusPlusToken ||
      tsUnaryArithm.operator === ts.SyntaxKind.MinusMinusToken
    ) {
      this.checkAutoIncrementDecrement(tsUnaryArithm);
    }
  }

  private handleBinaryExpression(node: ts.Node): void {
    const tsBinaryExpr = node as ts.BinaryExpression;
    const tsLhsExpr = tsBinaryExpr.left;
    const tsRhsExpr = tsBinaryExpr.right;
    if (isAssignmentOperator(tsBinaryExpr.operatorToken)) {
      this.processBinaryAssignment(tsBinaryExpr, tsLhsExpr, tsRhsExpr);
    }
    const leftOperandType = this.tsTypeChecker.getTypeAtLocation(tsLhsExpr);
    const typeNode = this.tsUtils.getVariableDeclarationTypeNode(tsLhsExpr);
    switch (tsBinaryExpr.operatorToken.kind) {
      // FaultID.BitOpWithWrongType - removed as rule #61
      case ts.SyntaxKind.CommaToken:
        this.processBinaryComma(tsBinaryExpr);
        break;
      case ts.SyntaxKind.InstanceOfKeyword:
        this.processBinaryInstanceOf(node, tsLhsExpr, leftOperandType);
        this.handleInstanceOfExpression(tsBinaryExpr);
        break;
      case ts.SyntaxKind.InKeyword:
        this.incrementCounters(tsBinaryExpr.operatorToken, FaultID.InOperator);
        break;
      case ts.SyntaxKind.EqualsToken:
        this.handleTsInterop(tsLhsExpr, () => {
          this.checkUsageOfTsTypes(leftOperandType, tsBinaryExpr);
        });
        this.checkAssignmentMatching(tsBinaryExpr, leftOperandType, tsRhsExpr);
        this.checkFunctionTypeCompatible(typeNode, tsRhsExpr);
        this.handleEsObjectAssignment(tsBinaryExpr, typeNode, tsRhsExpr);
        this.handleSdkGlobalApi(tsBinaryExpr);
        break;
      case ts.SyntaxKind.AmpersandAmpersandEqualsToken:
      case ts.SyntaxKind.QuestionQuestionEqualsToken:
      case ts.SyntaxKind.BarBarEqualsToken:
        if (this.options.arkts2) {
          this.incrementCounters(tsBinaryExpr.operatorToken, FaultID.UnsupportOperator);
        }
        break;
      default:
    }
    this.checkInterOpImportJsDataCompare(tsBinaryExpr);
    this.checkInteropEqualityJudgment(tsBinaryExpr);
    this.handleNumericBigintCompare(tsBinaryExpr);
    this.handleArkTSPropertyAccess(tsBinaryExpr);
    this.handleObjectLiteralAssignmentToClass(tsBinaryExpr);
    this.handleAssignmentNotsLikeSmartType(tsBinaryExpr);
  }

  private checkInterOpImportJsDataCompare(expr: ts.BinaryExpression): void {
    if (!this.useStatic || !this.options.arkts2 || !TypeScriptLinter.isComparisonOperator(expr.operatorToken.kind)) {
      return;
    }

    const processExpression = (expr: ts.Expression): void => {
      const symbol = this.tsUtils.trueSymbolAtLocation(expr);
      if (this.isJsFileSymbol(symbol) || this.isJsFileExpression(expr)) {
        const autofix = this.autofixer?.fixInteropOperators(expr);
        this.incrementCounters(expr, FaultID.InterOpImportJsDataCompare, autofix);
      }
    };

    processExpression(expr.left);
    processExpression(expr.right);
  }

  private static isComparisonOperator(kind: ts.SyntaxKind): boolean {
    return [
      ts.SyntaxKind.GreaterThanToken,
      ts.SyntaxKind.LessThanToken,
      ts.SyntaxKind.GreaterThanEqualsToken,
      ts.SyntaxKind.LessThanEqualsToken
    ].includes(kind);
  }

  private isJsFileSymbol(symbol: ts.Symbol | undefined): boolean {
    if (!symbol) {
      return false;
    }

    const declaration = symbol.declarations?.[0];
    if (!declaration || !ts.isVariableDeclaration(declaration)) {
      return false;
    }

    const initializer = declaration.initializer;
    return initializer ? this.isJsFileExpression(initializer) : false;
  }

  private isJsFileExpression(expr: ts.Expression): boolean {
    if (ts.isPropertyAccessExpression(expr)) {
      const initializerSym = this.tsUtils.trueSymbolAtLocation(expr.expression);
      return initializerSym?.declarations?.[0]?.getSourceFile()?.fileName.endsWith(EXTNAME_JS) ?? false;
    }
    return expr.getSourceFile()?.fileName.endsWith(EXTNAME_JS) ?? false;
  }

  private checkInteropEqualityJudgment(tsBinaryExpr: ts.BinaryExpression): void {
    if (this.useStatic && this.options.arkts2) {
      switch (tsBinaryExpr.operatorToken.kind) {
        case ts.SyntaxKind.EqualsEqualsToken:
        case ts.SyntaxKind.ExclamationEqualsToken:
        case ts.SyntaxKind.EqualsEqualsEqualsToken:
        case ts.SyntaxKind.ExclamationEqualsEqualsToken:
          if (this.tsUtils.isJsImport(tsBinaryExpr.left) || this.tsUtils.isJsImport(tsBinaryExpr.right)) {
            const autofix = this.autofixer?.fixInteropEqualityOperator(tsBinaryExpr, tsBinaryExpr.operatorToken.kind);
            this.incrementCounters(tsBinaryExpr, FaultID.InteropEqualityJudgment, autofix);
          }
          break;
        default:
      }
    }
  }

  private handleTsInterop(nodeToCheck: ts.Node, handler: { (): void }): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    const declarationNode = this.tsUtils.getDeclarationNode(nodeToCheck);
    if (!declarationNode) {
      return;
    }

    const fileName = declarationNode.getSourceFile().fileName;
    if (fileName.includes(ARKTS_IGNORE_DIRS_OH_MODULES)) {
      return;
    }
    if (!fileName.endsWith(EXTNAME_TS)) {
      return;
    }

    if (fileName.endsWith(EXTNAME_D_TS)) {
      return;
    }

    handler();
  }

  private handleJsInterop(nodeToCheck: ts.Node, handler: { (): void }): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    const declarationNode = this.tsUtils.getDeclarationNode(nodeToCheck);
    if (!declarationNode) {
      return;
    }

    const fileName = declarationNode.getSourceFile().fileName;
    if (fileName.includes(ARKTS_IGNORE_DIRS_OH_MODULES)) {
      return;
    }
    if (!fileName.endsWith(EXTNAME_JS)) {
      return;
    }

    if (fileName.endsWith(EXTNAME_D_TS)) {
      return;
    }

    handler();
  }

  private processBinaryAssignment(
    binaryExpr: ts.BinaryExpression,
    tsLhsExpr: ts.Expression,
    tsRhsExpr: ts.Expression
  ): void {
    this.handleDestructuringAssignment(binaryExpr, tsLhsExpr, tsRhsExpr);

    if (ts.isPropertyAccessExpression(tsLhsExpr)) {
      const tsLhsSymbol = this.tsUtils.trueSymbolAtLocation(tsLhsExpr);
      const tsLhsBaseSymbol = this.tsUtils.trueSymbolAtLocation(tsLhsExpr.expression);
      if (tsLhsSymbol && tsLhsSymbol.flags & ts.SymbolFlags.Method) {
        this.incrementCounters(tsLhsExpr, FaultID.MethodReassignment);
      }
      if (
        !this.options.arkts2 &&
        TsUtils.isMethodAssignment(tsLhsSymbol) &&
        tsLhsBaseSymbol &&
        (tsLhsBaseSymbol.flags & ts.SymbolFlags.Function) !== 0
      ) {
        this.incrementCounters(tsLhsExpr, FaultID.PropertyDeclOnFunction);
      }
    }
  }

  private checkAssignmentNumericSemanticsly(node: ts.VariableDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }
    const initializer = node.initializer;
    const name = node.name;
    if (node.type || !initializer || !ts.isIdentifier(name)) {
      return;
    }

    // Early return if the variable is imported from JS
    if (this.tsUtils.isPossiblyImportedFromJS(name) || this.tsUtils.isPossiblyImportedFromJS(initializer)) {
      return;
    }

    if (
      ts.isBinaryExpression(initializer) &&
      ts.isCallExpression(initializer.left) &&
      TsUtils.isAppStorageAccess(initializer.left)
    ) {
      return;
    }

    const sym = this.tsTypeChecker.getSymbolAtLocation(name);
    if (!sym) {
      return;
    }

    const type = this.tsTypeChecker.getTypeOfSymbolAtLocation(sym, name);
    const typeText = this.tsTypeChecker.typeToString(type);
    const isEnum = this.isNumericEnumType(type);
    if (TsUtils.isNumberLike(type, typeText, isEnum)) {
      const autofix = this.autofixer?.fixVariableDeclaration(node, isEnum);
      this.incrementCounters(node, FaultID.NumericSemantics, autofix);
    }
  }

  private isEnumType(type: ts.Type): boolean {
    if (type.flags & ts.TypeFlags.Enum) {
      return true;
    }

    if (type.symbol?.flags & ts.SymbolFlags.Enum) {
      return true;
    }

    if (type.flags & ts.TypeFlags.EnumLiteral) {
      return true;
    }

    if (type.isUnion()) {
      return type.types.some((t) => {
        return this.isEnumType(t);
      });
    }
    return false;
  }

  private isNumericEnumType(type: ts.Type): boolean {
    if (!this.isEnumType(type)) {
      return false;
    }
    const declarations = type.symbol?.getDeclarations() || [];
    const enumMemberDecl = declarations.find(ts.isEnumMember);
    if (enumMemberDecl) {
      const value = this.tsTypeChecker.getConstantValue(enumMemberDecl);
      return typeof value === STRINGLITERAL_NUMBER;
    }

    const enumDecl = declarations.find(ts.isEnumDeclaration);
    if (enumDecl) {
      return enumDecl.members.every((member) => {
        const memberType = this.tsTypeChecker.getTypeAtLocation(member.name);
        return (memberType.flags & ts.TypeFlags.NumberLike) !== 0;
      });
    }
    return false;
  }

  private checkAssignmentNumericSemanticsFuntion(node: ts.FunctionDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }
    for (const param of node.parameters) {
      if (param.type) {
        continue;
      }
      const sym = this.tsTypeChecker.getSymbolAtLocation(param.name);
      if (!sym) {
        continue;
      }

      const type = this.tsTypeChecker.getTypeOfSymbolAtLocation(sym, param.name);
      const typeText = this.tsTypeChecker.typeToString(type);
      if (typeText === STRINGLITERAL_NUMBER) {
        const autofix = this.autofixer?.fixParameter(param);
        if (autofix) {
          this.incrementCounters(node, FaultID.NumericSemantics, autofix);
        }
      }
    }
    if (!node.type) {
      const signature = this.tsTypeChecker.getSignatureFromDeclaration(node);
      if (!signature) {
        return;
      }
      const retType = this.tsTypeChecker.getReturnTypeOfSignature(signature);
      if ((retType.getFlags() & ts.TypeFlags.Number) !== 0) {
        const returnTypeNode = this.tsTypeChecker.typeToTypeNode(retType, node, ts.NodeBuilderFlags.None);
        if (!returnTypeNode) {
          return;
        }
        const autofix = this.autofixer?.fixMissingReturnType(node, returnTypeNode);
        this.incrementCounters(node, FaultID.NumericSemantics, autofix);
      }
    }
  }

  private checkAssignmentNumericSemanticslyPro(node: ts.PropertyDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const initializer = node.initializer;
    const name = node.name;
    if (node.type || !initializer || !ts.isIdentifier(name)) {
      return;
    }

    const isNumberArray = ts.isArrayLiteralExpression(initializer) && TypeScriptLinter.isNumberArray(initializer);
    const isNumber = !isNumberArray && TypeScriptLinter.isNumericInitializer(initializer);

    const sym = this.tsTypeChecker.getSymbolAtLocation(name);
    if (!sym) {
      return;
    }

    if (!isNumber && !isNumberArray) {
      return;
    }
    const type = this.tsTypeChecker.getTypeOfSymbolAtLocation(sym, name);
    const typeText = this.tsTypeChecker.typeToString(type);
    const typeFlags = type.flags;
    if (isNumber && (typeText === STRINGLITERAL_NUMBER || (typeFlags & ts.TypeFlags.NumberLiteral) !== 0)) {
      const autofix = this.autofixer?.fixPropertyDeclaration(node);
      this.incrementCounters(node, FaultID.NumericSemantics, autofix);
    }
    this.checkAssignmentNumericSemanticsArray(node, isNumberArray);
  }

  checkAssignmentNumericSemanticsArray(node: ts.PropertyDeclaration, isNumberArray: boolean): void {
    if (isNumberArray) {
      const autofix = this.autofixer?.fixPropertyDeclarationNumericSemanticsArray(node);
      this.incrementCounters(node, FaultID.NumericSemantics, autofix);
    }
  }

  private static isNumericInitializer(node: ts.Node): boolean {
    if (ts.isNumericLiteral(node)) {
      return true;
    }
    if (
      ts.isPrefixUnaryExpression(node) &&
      node.operator === ts.SyntaxKind.MinusToken &&
      ts.isNumericLiteral(node.operand)
    ) {
      return true;
    }
    return false;
  }

  private static isNumberArray(arrayLiteral: ts.ArrayLiteralExpression): boolean {
    return arrayLiteral.elements.every((element) => {
      if (ts.isSpreadElement(element)) {
        return false;
      }
      return TypeScriptLinter.isNumericInitializer(element);
    });
  }

  private handleDestructuringAssignment(node: ts.Node, tsLhsExpr: ts.Expression, tsRhsExpr: ts.Expression): void {
    if (ts.isObjectLiteralExpression(tsLhsExpr)) {
      const autofix = this.autofixer?.fixObjectLiteralExpressionDestructAssignment(node as ts.BinaryExpression);
      this.incrementCounters(node, FaultID.DestructuringAssignment, autofix);
    } else if (ts.isArrayLiteralExpression(tsLhsExpr)) {
      const rhsType = this.tsTypeChecker.getTypeAtLocation(tsRhsExpr);
      const isArrayOrTuple =
        this.tsUtils.isOrDerivedFrom(rhsType, this.tsUtils.isArray) ||
        this.tsUtils.isOrDerivedFrom(rhsType, TsUtils.isTuple);
      const hasNestedObjectDestructuring = TsUtils.hasNestedObjectDestructuring(tsLhsExpr);

      if (
        !this.options.useRelaxedRules ||
        !isArrayOrTuple ||
        hasNestedObjectDestructuring ||
        TsUtils.destructuringAssignmentHasSpreadOperator(tsLhsExpr)
      ) {
        const autofix = this.autofixer?.fixArrayBindingPatternAssignment(node as ts.BinaryExpression, isArrayOrTuple);
        this.incrementCounters(node, FaultID.DestructuringAssignment, autofix);
      }
    }
  }

  private processBinaryComma(tsBinaryExpr: ts.BinaryExpression): void {
    // CommaOpertor is allowed in 'for' statement initalizer and incrementor
    let tsExprNode: ts.Node = tsBinaryExpr;
    let tsParentNode = tsExprNode.parent;
    while (tsParentNode && tsParentNode.kind === ts.SyntaxKind.BinaryExpression) {
      tsExprNode = tsParentNode;
      tsParentNode = tsExprNode.parent;
      if ((tsExprNode as ts.BinaryExpression).operatorToken.kind === ts.SyntaxKind.CommaToken) {
        // Need to return if one comma enclosed in expression with another comma to avoid multiple reports on one line
        return;
      }
    }
    if (tsParentNode && tsParentNode.kind === ts.SyntaxKind.ForStatement) {
      const tsForNode = tsParentNode as ts.ForStatement;
      if (tsExprNode === tsForNode.initializer || tsExprNode === tsForNode.incrementor) {
        return;
      }
    }
    if (tsParentNode && tsParentNode.kind === ts.SyntaxKind.ExpressionStatement) {
      const autofix = this.autofixer?.fixCommaOperator(tsExprNode);
      this.incrementCounters(tsExprNode, FaultID.CommaOperator, autofix);
      return;
    }

    this.incrementCounters(tsBinaryExpr as ts.Node, FaultID.CommaOperator);
  }

  private processBinaryInstanceOf(node: ts.Node, tsLhsExpr: ts.Expression, leftOperandType: ts.Type): void {
    const leftExpr = TsUtils.unwrapParenthesized(tsLhsExpr);
    const leftSymbol = this.tsUtils.trueSymbolAtLocation(leftExpr);

    /*
     * In ETS, the left-hand side expression may be of any reference type, otherwise
     * a compile-time error occurs. In addition, the left operand in ETS cannot be a type.
     */
    if (tsLhsExpr.kind === ts.SyntaxKind.ThisKeyword) {
      return;
    }

    if (TsUtils.isPrimitiveType(leftOperandType) || ts.isTypeNode(leftExpr) || TsUtils.isTypeSymbol(leftSymbol)) {
      this.incrementCounters(node, FaultID.InstanceofUnsupported);
    }
  }

  private handleVariableDeclarationList(node: ts.Node): void {
    const varDeclFlags = ts.getCombinedNodeFlags(node);
    if (!(varDeclFlags & (ts.NodeFlags.Let | ts.NodeFlags.Const))) {
      const autofix = this.autofixer?.fixVarDeclaration(node as ts.VariableDeclarationList);
      this.incrementCounters(node, FaultID.VarDeclaration, autofix);
    }
  }

  private handleVariableDeclaration(node: ts.Node): void {
    const tsVarDecl = node as ts.VariableDeclaration;
    this.handleVariableDeclarationForProp(tsVarDecl);
    if (
      !this.options.useRtLogic ||
      ts.isVariableDeclarationList(tsVarDecl.parent) && ts.isVariableStatement(tsVarDecl.parent.parent)
    ) {
      this.handleDeclarationDestructuring(tsVarDecl);
    }

    // Check variable declaration for duplicate name.
    this.checkVarDeclForDuplicateNames(tsVarDecl.name);

    if (tsVarDecl.type && tsVarDecl.initializer) {
      this.checkAssignmentMatching(
        tsVarDecl,
        this.tsTypeChecker.getTypeAtLocation(tsVarDecl.type),
        tsVarDecl.initializer
      );
      this.checkFunctionTypeCompatible(tsVarDecl.type, tsVarDecl.initializer);
    }
    this.handleEsValueDeclaration(tsVarDecl);
    this.handleDeclarationInferredType(tsVarDecl);
    this.handleDefiniteAssignmentAssertion(tsVarDecl);
    this.handleLimitedVoidType(tsVarDecl);
    this.handleInvalidIdentifier(tsVarDecl);
    this.checkAssignmentNumericSemanticsly(tsVarDecl);
    this.checkTypeFromSdk(tsVarDecl.type);
    this.handleNoStructuralTyping(tsVarDecl);
    this.handleObjectLiteralforUnionTypeInterop(tsVarDecl);
    this.handleObjectLiteralAssignmentToClass(tsVarDecl);
    this.handleObjectLiteralAssignment(tsVarDecl);
    this.handlePropertyDescriptorInScenarios(tsVarDecl);
    this.handleSdkGlobalApi(tsVarDecl);
  }

  private checkTypeFromSdk(type: ts.TypeNode | undefined): void {
    if (!this.options.arkts2 || !type) {
      return;
    }

    const fullTypeName = type.getText();
    const nameArr = fullTypeName.split('.');
    const sdkInfos = this.interfaceMap.get(nameArr[0]);
    if (!sdkInfos || sdkInfos.size === 0) {
      return;
    }

    for (const sdkInfo of sdkInfos) {
      if (sdkInfo.api_name && nameArr.includes(sdkInfo.api_name)) {
        this.incrementCounters(type, FaultID.LimitedVoidTypeFromSdk);
        return;
      }
    }
  }

  private static extractUsedObjectType(tsVarDecl: ts.VariableDeclaration): InterfaceSymbolTypePropertyNames | null {
    const result = {
      propertyNames: [] as string[],
      typeNames: [] as string[]
    };

    if (!this.isObjectLiteralWithProperties(tsVarDecl)) {
      return null;
    }

    this.processObjectLiteralProperties(tsVarDecl.initializer as ts.ObjectLiteralExpression, result);
    return result.propertyNames.length > 0 ? result : null;
  }

  private static isObjectLiteralWithProperties(tsVarDecl: ts.VariableDeclaration): boolean {
    return (
      tsVarDecl.initializer !== undefined &&
      ts.isObjectLiteralExpression(tsVarDecl.initializer) &&
      tsVarDecl.initializer.properties.length > 0
    );
  }

  private static processObjectLiteralProperties(
    objectLiteral: ts.ObjectLiteralExpression,
    result: { propertyNames: string[]; typeNames: string[] }
  ): void {
    objectLiteral.properties.forEach((property) => {
      if (!ts.isPropertyAssignment(property)) {
        return;
      }

      const propertyName = property.name.getText();
      result.propertyNames.push(propertyName);

      if (ts.isNewExpression(property.initializer)) {
        const typeName = property.initializer.expression.getText();
        result.typeNames.push(typeName);
      }
    });
  }

  private interfaceSymbolType(tsVarDecl: ts.VariableDeclaration): InterfaceSymbolTypeResult | null {
    if (!tsVarDecl.type) {
      return null;
    }

    const typeSymbol = this.getTypeSymbol(tsVarDecl);
    if (!typeSymbol) {
      return null;
    }

    const interfaceType = this.getInterfaceType(tsVarDecl);
    if (!interfaceType) {
      return null;
    }

    return this.collectInterfaceProperties(interfaceType, tsVarDecl);
  }

  private getTypeSymbol(tsVarDecl: ts.VariableDeclaration): ts.Symbol | null {
    const typeNode = ts.isTypeReferenceNode(tsVarDecl.type!) ? tsVarDecl.type.typeName : tsVarDecl.type;
    return this.tsTypeChecker.getSymbolAtLocation(typeNode!) ?? null;
  }

  private getInterfaceType(tsVarDecl: ts.VariableDeclaration): ts.InterfaceType | null {
    const type = this.tsTypeChecker.getTypeAtLocation(tsVarDecl.type!);
    return type && (type as ts.ObjectType).objectFlags & ts.ObjectFlags.Interface ? (type as ts.InterfaceType) : null;
  }

  private collectInterfaceProperties(
    interfaceType: ts.InterfaceType,
    tsVarDecl: ts.VariableDeclaration
  ): InterfaceSymbolTypeResult {
    const result = {
      propNames: [] as string[],
      typeNames: [] as string[],
      allProps: new Map<string, string>()
    };

    this.collectPropertiesRecursive(interfaceType, result, tsVarDecl);
    return result;
  }

  private collectPropertiesRecursive(
    type: ts.Type,
    result: {
      propNames: string[];
      typeNames: string[];
      allProps: Map<string, string>;
    },
    tsVarDecl: ts.VariableDeclaration
  ): void {
    type.getProperties().forEach((property) => {
      this.collectProperty(property, result, tsVarDecl);
    });

    if ('getBaseTypes' in type) {
      type.getBaseTypes()?.forEach((baseType) => {
        this.collectPropertiesRecursive(baseType, result, tsVarDecl);
      });
    }
  }

  private collectProperty(
    property: ts.Symbol,
    result: {
      propNames: string[];
      typeNames: string[];
      allProps: Map<string, string>;
    },
    tsVarDecl: ts.VariableDeclaration
  ): void {
    const propName = property.getName();
    const propType = this.tsTypeChecker.getTypeOfSymbolAtLocation(
      property,
      property.valueDeclaration || tsVarDecl.type!
    );
    const typeString = this.tsTypeChecker.typeToString(propType);

    if (!result.allProps.has(propName)) {
      result.propNames.push(propName);
      result.typeNames.push(typeString);
      result.allProps.set(propName, typeString);
    }
  }

  handleNoStructuralTyping(tsVarDecl: ts.VariableDeclaration): void {
    const { interfaceInfo, actualUsage } = this.getTypeComparisonData(tsVarDecl);
    if (!interfaceInfo || !actualUsage) {
      return;
    }
    if (!this.options.arkts2) {
      return;
    }
    const actualMap = TypeScriptLinter.createActualTypeMap(actualUsage);
    const hasMismatch = TypeScriptLinter.checkTypeMismatches(interfaceInfo, actualMap);

    if (hasMismatch) {
      this.incrementCounters(tsVarDecl, FaultID.StructuralIdentity);
    }
  }

  private getTypeComparisonData(tsVarDecl: ts.VariableDeclaration): {
    interfaceInfo: { propNames: string[]; typeNames: string[]; allProps: Map<string, string> } | null;
    actualUsage: {
      propertyNames: string[];
      typeNames: string[];
    } | null;
  } {
    return {
      interfaceInfo: this.interfaceSymbolType(tsVarDecl),
      actualUsage: TypeScriptLinter.extractUsedObjectType(tsVarDecl)
    };
  }

  private static createActualTypeMap(actualUsage: {
    propertyNames: string[];
    typeNames: string[];
  }): Map<string, string> {
    const actualMap = new Map<string, string>();
    actualUsage.propertyNames.forEach((prop, index) => {
      if (actualUsage.typeNames[index]) {
        actualMap.set(prop, actualUsage.typeNames[index]);
      }
    });
    return actualMap;
  }

  private static checkTypeMismatches(
    interfaceInfo: { allProps: Map<string, string> },
    actualMap: Map<string, string>
  ): boolean {
    let hasMismatch = false;

    interfaceInfo.allProps.forEach((expectedType, prop) => {
      if (!actualMap.has(prop)) {
        return;
      }

      const actualType = actualMap.get(prop)!;
      if (expectedType !== actualType) {
        hasMismatch = true;
      }
    });

    return hasMismatch;
  }

  private handleDeclarationDestructuring(decl: ts.VariableDeclaration | ts.ParameterDeclaration): void {
    const faultId = ts.isVariableDeclaration(decl) ? FaultID.DestructuringDeclaration : FaultID.DestructuringParameter;
    if (ts.isObjectBindingPattern(decl.name)) {
      const autofix = ts.isVariableDeclaration(decl) ?
        this.autofixer?.fixObjectBindingPatternDeclarations(decl) :
        undefined;
      this.incrementCounters(decl, faultId, autofix);
    } else if (ts.isArrayBindingPattern(decl.name)) {
      // Array destructuring is allowed only for Arrays/Tuples and without spread operator.
      const rhsType = this.tsTypeChecker.getTypeAtLocation(decl.initializer ?? decl.name);
      const isArrayOrTuple =
        rhsType &&
        (this.tsUtils.isOrDerivedFrom(rhsType, this.tsUtils.isArray) ||
          this.tsUtils.isOrDerivedFrom(rhsType, TsUtils.isTuple));
      const hasNestedObjectDestructuring = TsUtils.hasNestedObjectDestructuring(decl.name);

      if (
        !this.options.useRelaxedRules ||
        !isArrayOrTuple ||
        hasNestedObjectDestructuring ||
        TsUtils.destructuringDeclarationHasSpreadOperator(decl.name)
      ) {
        const autofix = ts.isVariableDeclaration(decl) ?
          this.autofixer?.fixArrayBindingPatternDeclarations(decl, isArrayOrTuple) :
          undefined;
        this.incrementCounters(decl, faultId, autofix);
      }
    }
  }

  private checkVarDeclForDuplicateNames(tsBindingName: ts.BindingName): void {
    if (ts.isIdentifier(tsBindingName)) {
      // The syntax kind of the declaration is defined here by the parent of 'BindingName' node.
      this.countDeclarationsWithDuplicateName(tsBindingName, tsBindingName, tsBindingName.parent.kind);
      return;
    }
    for (const tsBindingElem of tsBindingName.elements) {
      if (ts.isOmittedExpression(tsBindingElem)) {
        continue;
      }

      this.checkVarDeclForDuplicateNames(tsBindingElem.name);
    }
  }

  private handleEsValueDeclaration(node: ts.VariableDeclaration): void {
    const isDeclaredESValue = !!node.type && TsUtils.isEsValueType(node.type);
    const initalizerTypeNode = node.initializer && this.tsUtils.getVariableDeclarationTypeNode(node.initializer);
    const isInitializedWithESValue = !!initalizerTypeNode && TsUtils.isEsValueType(initalizerTypeNode);
    const isLocal = TsUtils.isInsideBlock(node);
    if ((isDeclaredESValue || isInitializedWithESValue) && !isLocal) {
      const faultId = this.options.arkts2 ? FaultID.EsValueTypeError : FaultID.EsValueType;
      this.incrementCounters(node, faultId);
      return;
    }

    if (node.initializer) {
      this.handleEsObjectAssignment(node, node.type, node.initializer);
    }
  }

  private handleEsObjectAssignment(node: ts.Node, nodeDeclType: ts.TypeNode | undefined, initializer: ts.Node): void {
    const isTypeAnnotated = !!nodeDeclType;
    const isDeclaredESValue = isTypeAnnotated && TsUtils.isEsValueType(nodeDeclType);
    const initalizerTypeNode = this.tsUtils.getVariableDeclarationTypeNode(initializer);
    const isInitializedWithESValue = !!initalizerTypeNode && TsUtils.isEsValueType(initalizerTypeNode);
    if (isTypeAnnotated && !isDeclaredESValue && isInitializedWithESValue) {
      const faultId = this.options.arkts2 ? FaultID.EsValueTypeError : FaultID.EsValueType;
      this.incrementCounters(node, faultId);
      return;
    }

    if (isDeclaredESValue && !this.tsUtils.isValueAssignableToESValue(initializer)) {
      const faultId = this.options.arkts2 ? FaultID.EsValueTypeError : FaultID.EsValueType;
      this.incrementCounters(node, faultId);
    }
  }

  private handleCatchClause(node: ts.Node): void {
    const tsCatch = node as ts.CatchClause;

    /*
     * In TS catch clause doesn't permit specification of the exception varible type except 'any' or 'unknown'.
     * It is not compatible with ETS 'catch' where the exception variable has to be of type
     * Error or derived from it.
     * So each 'catch' which has explicit type for the exception object goes to problems.
     */
    if (tsCatch.variableDeclaration?.type) {
      const autofix = this.autofixer?.dropTypeOnVarDecl(tsCatch.variableDeclaration);
      this.incrementCounters(node, FaultID.CatchWithUnsupportedType, autofix);
    }

    if (this.options.arkts2 && tsCatch.variableDeclaration?.name) {
      const varDeclName = tsCatch.variableDeclaration?.name.getText();
      tsCatch.block.statements.forEach((statement) => {
        this.checkTsLikeCatchType(statement, varDeclName);
      });
    }
  }

  private checkTsLikeCatchType(node: ts.Node, variableDeclarationName: string): void {
    if (!node) {
      return;
    }
    for (const child of node.getChildren()) {
      if (ts.isPropertyAccessExpression(child)) {
        if (child.expression.getText() === variableDeclarationName && !ERROR_PROP_LIST.has(child.name.getText())) {
          this.incrementCounters(child, FaultID.TsLikeCatchType);
        }
      }
      this.checkTsLikeCatchType(child, variableDeclarationName);
    }
  }

  private handleClassExtends(tsClassDecl: ts.ClassDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }
    const allClasses = TypeScriptLinter.getAllClassesFromSourceFile(this.sourceFile);
    const classMap = new Map<string, ts.ClassDeclaration>();
    allClasses.forEach((classDecl) => {
      if (classDecl.name && !classDecl.heritageClauses) {
        classMap.set(classDecl.name.getText(), classDecl);
      }
    });
    if (!tsClassDecl.heritageClauses) {
      return;
    }
    tsClassDecl.heritageClauses.forEach((clause) => {
      clause.types.forEach((type) => {
        const baseClassName = type.expression.getText();
        const baseClass = classMap.get(baseClassName);
        if (baseClass && ts.isClassDeclaration(baseClass)) {
          this.checkMembersConsistency(tsClassDecl, baseClass);
        }
      });
    });
  }

  private checkMembersConsistency(derivedClass: ts.ClassDeclaration, baseClass: ts.ClassDeclaration): void {
    const baseMethods = new Set<string>();
    baseClass.members.forEach((member) => {
      if (ts.isMethodDeclaration(member)) {
        baseMethods.add(member.name.getText());
      }
    });
    derivedClass.members.forEach((member) => {
      const memberName = member.name?.getText();
      if (memberName && baseMethods.has(memberName)) {
        if (ts.isPropertyDeclaration(member)) {
          this.incrementCounters(member, FaultID.MethodOverridingField);
        }
      }
    });
  }

  private handleClassDeclaration(node: ts.Node): void {
    // early exit via exception if cancellation was requested
    this.options.cancellationToken?.throwIfCancellationRequested();

    const tsClassDecl = node as ts.ClassDeclaration;
    this.handleClassExtends(tsClassDecl);
    if (tsClassDecl.name) {
      this.countDeclarationsWithDuplicateName(tsClassDecl.name, tsClassDecl);
    }
    this.countClassMembersWithDuplicateName(tsClassDecl);

    const isSendableClass = TsUtils.hasSendableDecorator(tsClassDecl);
    if (isSendableClass) {
      TsUtils.getNonSendableDecorators(tsClassDecl)?.forEach((decorator) => {
        this.incrementCounters(decorator, FaultID.SendableClassDecorator);
      });
      tsClassDecl.typeParameters?.forEach((typeParamDecl) => {
        this.checkSendableTypeParameter(typeParamDecl);
      });
    }

    if (tsClassDecl.heritageClauses) {
      for (const hClause of tsClassDecl.heritageClauses) {
        if (!hClause) {
          continue;
        }
        this.checkClassDeclarationHeritageClause(hClause, isSendableClass);
      }
    }

    // Check captured variables for sendable class
    if (isSendableClass) {
      tsClassDecl.members.forEach((classMember) => {
        this.scanCapturedVarsInSendableScope(classMember, tsClassDecl, FaultID.SendableCapturedVars);
      });
    }

    this.processClassStaticBlocks(tsClassDecl);
    this.handleInvalidIdentifier(tsClassDecl);
    this.handleSdkMethod(tsClassDecl);
    this.handleNotsLikeSmartType(tsClassDecl);
  }

  private static findFinalExpression(typeNode: ts.TypeNode): ts.Node {
    let currentNode = typeNode;

    /*
     * CC-OFFNXT(no_explicit_any) std lib
     * Handle comment directive '@ts-nocheck'
     */
    while ((currentNode as any).expression) {

      /*
       * CC-OFFNXT(no_explicit_any) std lib
       * Handle comment directive '@ts-nocheck'
       */
      currentNode = (currentNode as any).expression;
    }
    return currentNode;
  }

  private processSdkMethodClauseTypes(
    tsClassDecl: ts.ClassDeclaration,
    heritageClause: ts.HeritageClause,
    methodName?: string
  ): boolean {
    return heritageClause.types.some((type) => {
      const parentName = ts.isPropertyAccessExpression(type.expression) ?
        type.expression.name.text :
        type.expression.getText();
      const fullTypeName = TypeScriptLinter.findFinalExpression(type).getText();
      const sdkInfos = this.interfaceMap.get(fullTypeName);
      if (!sdkInfos || sdkInfos.size === 0) {
        return false;
      }

      return Array.from(sdkInfos).some((sdkInfo) => {
        if (sdkInfo.api_type !== METHOD_SIGNATURE && sdkInfo.api_type !== METHOD_DECLARATION) {
          return false;
        }

        if (!methodName && sdkInfo.parent_api[0].api_name === parentName) {
          this.processSdkInfoWithMembers(sdkInfo, tsClassDecl.members, tsClassDecl);
          return false;
        }

        const symbol = this.tsTypeChecker.getSymbolAtLocation(type.expression);
        return TypeScriptLinter.isHeritageClauseisThirdPartyBySymbol(symbol) && sdkInfo.api_name === methodName;
      });
    });
  }

  private handleSdkMethod(tsClassDecl: ts.ClassDeclaration): void {
    if (
      !this.options.arkts2 ||
      !tsClassDecl.heritageClauses ||
      tsClassDecl.heritageClauses.length === 0 ||
      !tsClassDecl.members ||
      tsClassDecl.members.length === 0
    ) {
      return;
    }

    for (const heritageClause of tsClassDecl.heritageClauses) {
      if (!heritageClause.types || heritageClause.types.length === 0) {
        continue;
      }
      this.processSdkMethodClauseTypes(tsClassDecl, heritageClause);
    }
  }

  private processSdkInfoWithMembers(
    sdkInfo: ApiInfo,
    members: ts.NodeArray<ts.ClassElement>,
    tsClassDecl: ts.ClassDeclaration
  ): void {
    for (const member of members) {
      if (!ts.isMethodDeclaration(member)) {
        continue;
      }

      const memberName = member.name?.getText();
      if (sdkInfo.api_name === memberName) {
        if (
          !TypeScriptLinter.areParametersEqual(sdkInfo.api_func_args ?? [], member.parameters) &&
          !TypeScriptLinter.areGenericsParametersEqual(sdkInfo.api_func_args ?? [], tsClassDecl)
        ) {
          return;
        }
        this.incrementCounters(
          member,
          sdkInfo.problem === OPTIONAL_METHOD ? FaultID.OptionalMethodFromSdk : FaultID.LimitedVoidTypeFromSdk
        );
      }
    }
  }

  private static areParametersEqual(
    sdkFuncArgs: { name: string; type: string }[],
    memberParams: ts.NodeArray<ts.ParameterDeclaration>
  ): boolean {
    const apiParamCout = sdkFuncArgs.length;
    const memberParamCout = memberParams.length;
    if (apiParamCout > memberParamCout && sdkFuncArgs[memberParamCout]) {
      return false;
    }

    for (let i = 0; i < apiParamCout; i++) {
      const typeName = memberParams[i]?.type?.getText();
      if (!typeName?.match(sdkFuncArgs[i].type)) {
        return false;
      }
    }
    return true;
  }

  private processLimitedVoidTypeFromSdkOnClassDeclaration(
    tsClassDecl: ts.ClassDeclaration,
    methodName?: string
  ): boolean {
    if (
      !this.options.arkts2 ||
      !tsClassDecl.heritageClauses ||
      tsClassDecl.heritageClauses.length === 0 ||
      !tsClassDecl.members ||
      tsClassDecl.members.length === 0
    ) {
      return false;
    }
    let res: boolean = false;
    for (const heritageClause of tsClassDecl.heritageClauses) {
      if (heritageClause.types?.length) {
        res = this.processSdkMethodClauseTypes(tsClassDecl, heritageClause, methodName);
        break;
      }
    }
    return res;
  }

  private static isHeritageClauseisThirdPartyBySymbol(symbol: ts.Symbol | undefined): boolean {
    if (!symbol) {
      return false;
    }
    const declarations = symbol.getDeclarations();
    if (declarations && declarations.length > 0) {
      const firstDeclaration = declarations[0];
      if (ts.isImportSpecifier(firstDeclaration)) {
        return true;
      }
    }
    return false;
  }

  private handleLimitedVoidTypeFromSdkOnPropertyAccessExpression(node: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    const sym = this.getOriginalSymbol(node.name);
    if (!sym) {
      return;
    }
    const methodName = node.name.getText();
    const declaration = sym.declarations?.[0];
    if (declaration && ts.isClassDeclaration(declaration.parent)) {
      if (this.processLimitedVoidTypeFromSdkOnClassDeclaration(declaration.parent, methodName)) {
        this.incrementCounters(node, FaultID.LimitedVoidTypeFromSdk);
      }
    }
  }

  private static areGenericsParametersEqual(
    sdkFuncArgs: { name: string; type: string }[],
    node: ts.ClassDeclaration
  ): boolean {
    if (!ts.isClassDeclaration(node)) {
      return false;
    }
    const apiParamCout = sdkFuncArgs.length;
    const typeParameters = node.typeParameters;
    if (!typeParameters) {
      return false;
    }
    typeParameters.forEach((typeParam) => {
      if (!typeParam.constraint) {
        return false;
      }
      for (let i = 0; i < apiParamCout; i++) {
        if (!typeParam.constraint.getText().match(sdkFuncArgs[i].type)) {
          return false;
        }
      }
      return true;
    });
    return true;
  }

  private handleNotSupportCustomDecorators(decorator: ts.Decorator): void {
    if (!this.options.arkts2) {
      return;
    }

    let decoratorName;
    if (ts.isCallExpression(decorator.expression)) {
      decoratorName = decorator.expression.expression.getText(this.sourceFile);
    } else {
      decoratorName = decorator.expression.getText(this.sourceFile);
    }
    if (!DEFAULT_DECORATOR_WHITE_LIST.includes(decoratorName)) {
      this.incrementCounters(decorator, FaultID.DecoratorsNotSupported);
    }
  }

  private checkClassDeclarationHeritageClause(hClause: ts.HeritageClause, isSendableClass: boolean): void {
    for (const tsTypeExpr of hClause.types) {

      /*
       * Always resolve type from 'tsTypeExpr' node, not from 'tsTypeExpr.expression' node,
       * as for the latter, type checker will return incorrect type result for classes in
       * 'extends' clause. Additionally, reduce reference, as mostly type checker returns
       * the TypeReference type objects for classes and interfaces.
       */
      const tsExprType = TsUtils.reduceReference(this.tsTypeChecker.getTypeAtLocation(tsTypeExpr));
      const isSendableBaseType = this.tsUtils.isSendableClassOrInterface(tsExprType);
      if (tsExprType.isClass() && hClause.token === ts.SyntaxKind.ImplementsKeyword) {
        this.incrementCounters(tsTypeExpr, FaultID.ImplementsClass);
      }
      if (!isSendableClass) {
        // Non-Sendable class can not implements sendable interface / extends sendable class
        if (isSendableBaseType) {
          const autofix = this.autofixer?.addClassSendableDecorator(hClause, tsTypeExpr);
          this.incrementCounters(tsTypeExpr, FaultID.SendableClassInheritance, autofix);
        }
        continue;
      }

      /*
       * Sendable class can implements any interface / extends only sendable class
       * Sendable class can not extends sendable class variable(local / import)
       */
      if (hClause.token === ts.SyntaxKind.ExtendsKeyword) {
        if (!isSendableBaseType) {
          this.incrementCounters(tsTypeExpr, FaultID.SendableClassInheritance);
          continue;
        }
        if (!this.tsUtils.isValidSendableClassExtends(tsTypeExpr)) {
          this.incrementCounters(tsTypeExpr, FaultID.SendableClassInheritance);
        }
      }
    }
  }

  private checkSendableTypeParameter(typeParamDecl: ts.TypeParameterDeclaration): void {
    const defaultTypeNode = typeParamDecl.default;
    if (defaultTypeNode) {
      if (!this.tsUtils.isSendableTypeNode(defaultTypeNode)) {
        this.incrementCounters(defaultTypeNode, FaultID.SendableGenericTypes);
      }
    }
  }

  private processClassStaticBlocks(classDecl: ts.ClassDeclaration): void {
    let staticBlocksCntr = 0;
    const staticBlockNodes: ts.Node[] = [];
    for (const element of classDecl.members) {
      if (ts.isClassStaticBlockDeclaration(element)) {
        if (this.options.arkts2 && this.useStatic) {
          this.incrementCounters(element, FaultID.NoStaticOnClass);
        }
        staticBlockNodes[staticBlocksCntr] = element;
        staticBlocksCntr++;
      }
    }
    if (staticBlocksCntr > 1) {
      const autofix = this.autofixer?.fixMultipleStaticBlocks(staticBlockNodes);
      // autofixes for all additional static blocks are the same
      for (let i = 1; i < staticBlocksCntr; i++) {
        this.incrementCounters(staticBlockNodes[i], FaultID.MultipleStaticBlocks, autofix);
      }
    }
  }

  private handleModuleDeclaration(node: ts.Node): void {
    // early exit via exception if cancellation was requested
    this.options.cancellationToken?.throwIfCancellationRequested();

    const tsModuleDecl = node as ts.ModuleDeclaration;

    this.countDeclarationsWithDuplicateName(tsModuleDecl.name, tsModuleDecl);

    if (this.options.arkts2) {
      this.handleInvalidIdentifier(tsModuleDecl);
    }

    const tsModuleBody = tsModuleDecl.body;
    const tsModifiers = ts.getModifiers(tsModuleDecl);
    if (tsModuleBody) {
      if (ts.isModuleBlock(tsModuleBody)) {
        this.handleModuleBlock(tsModuleBody);
      }
    }

    if (
      this.options.arkts2 &&
      tsModuleBody &&
      ts.isModuleBlock(tsModuleBody) &&
      tsModuleDecl.flags & ts.NodeFlags.Namespace
    ) {
      this.handleNameSpaceModuleBlock(tsModuleBody, (tsModuleDecl.name as ts.Identifier).escapedText.toString());
    }

    if (
      !(tsModuleDecl.flags & ts.NodeFlags.Namespace) &&
      TsUtils.hasModifier(tsModifiers, ts.SyntaxKind.DeclareKeyword)
    ) {
      this.incrementCounters(tsModuleDecl, FaultID.ShorthandAmbientModuleDecl);
    }

    if (ts.isStringLiteral(tsModuleDecl.name) && tsModuleDecl.name.text.includes('*')) {
      this.incrementCounters(tsModuleDecl, FaultID.WildcardsInModuleName);
    }
  }

  private handleNameSpaceModuleBlock(moduleBlock: ts.ModuleBlock, nameSpace: string): void {
    if (!TypeScriptLinter.nameSpaceFunctionCache.has(nameSpace)) {
      TypeScriptLinter.nameSpaceFunctionCache.set(nameSpace, new Set<string>());
    }

    const nameSet = TypeScriptLinter.nameSpaceFunctionCache.get(nameSpace)!;

    for (const statement of moduleBlock.statements) {
      const names = TypeScriptLinter.getDeclarationNames(statement);
      for (const name of names) {
        if (nameSet.has(name)) {
          this.incrementCounters(statement, FaultID.NoDuplicateFunctionName);
        } else {
          nameSet.add(name);
        }
      }
    }
  }

  private static getDeclarationNames(statement: ts.Statement): Set<string> {
    const names = new Set<string>();

    if (
      ts.isFunctionDeclaration(statement) && statement.name && statement.body ||
      ts.isClassDeclaration(statement) && statement.name ||
      ts.isInterfaceDeclaration(statement) && statement.name ||
      ts.isEnumDeclaration(statement) && statement.name
    ) {
      names.add(statement.name.text);
      return names;
    }

    if (ts.isVariableStatement(statement)) {
      for (const decl of statement.declarationList.declarations) {
        if (ts.isIdentifier(decl.name)) {
          names.add(decl.name.text);
        }
      }
    }

    return names;
  }

  private handleModuleBlock(moduleBlock: ts.ModuleBlock): void {
    for (const tsModuleStmt of moduleBlock.statements) {
      switch (tsModuleStmt.kind) {
        case ts.SyntaxKind.VariableStatement:
        case ts.SyntaxKind.FunctionDeclaration:
        case ts.SyntaxKind.ClassDeclaration:
        case ts.SyntaxKind.InterfaceDeclaration:
        case ts.SyntaxKind.TypeAliasDeclaration:
        case ts.SyntaxKind.EnumDeclaration:
        case ts.SyntaxKind.ExportDeclaration:
          break;

        /*
         * Nested namespace declarations are prohibited
         * but there is no cookbook recipe for it!
         */
        case ts.SyntaxKind.ModuleDeclaration:
          break;
        default:
          this.incrementCounters(tsModuleStmt, FaultID.NonDeclarationInNamespace);
          break;
      }
    }
  }

  private handleTypeAliasDeclaration(node: ts.Node): void {
    const tsTypeAlias = node as ts.TypeAliasDeclaration;
    this.countDeclarationsWithDuplicateName(tsTypeAlias.name, tsTypeAlias);
    this.handleInvalidIdentifier(tsTypeAlias);
    if (TsUtils.hasSendableDecorator(tsTypeAlias)) {
      if (!this.isSendableDecoratorValid(tsTypeAlias)) {
        return;
      }
      TsUtils.getNonSendableDecorators(tsTypeAlias)?.forEach((decorator) => {
        this.incrementCounters(decorator, FaultID.SendableTypeAliasDecorator);
      });
      if (!ts.isFunctionTypeNode(tsTypeAlias.type)) {
        this.incrementCounters(tsTypeAlias.type, FaultID.SendableTypeAliasDeclaration);
      }
    }
    if (this.options.arkts2 && tsTypeAlias.type.kind === ts.SyntaxKind.VoidKeyword) {
      this.incrementCounters(tsTypeAlias.type, FaultID.LimitedVoidType);
    }
  }

  private handleTupleType(node: ts.TupleTypeNode): void {
    if (!this.options.arkts2) {
      return;
    }

    node.elements.forEach((elementType) => {
      if (elementType.kind === ts.SyntaxKind.VoidKeyword) {
        this.incrementCounters(elementType, FaultID.LimitedVoidType);
      }
    });
  }

  private handleImportClause(node: ts.Node): void {
    const tsImportClause = node as ts.ImportClause;
    if (this.options.arkts2 && tsImportClause.isLazy) {
      const autofix = this.autofixer?.fixImportClause(tsImportClause);
      this.incrementCounters(node, FaultID.ImportLazyIdentifier, autofix);
    }
    if (tsImportClause.name) {
      this.countDeclarationsWithDuplicateName(tsImportClause.name, tsImportClause);
    }
  }

  private handleImportSpecifier(node: ts.Node): void {
    const importSpec = node as ts.ImportSpecifier;
    this.countDeclarationsWithDuplicateName(importSpec.name, importSpec);
  }

  private handleNamespaceImport(node: ts.Node): void {
    const tsNamespaceImport = node as ts.NamespaceImport;
    this.countDeclarationsWithDuplicateName(tsNamespaceImport.name, tsNamespaceImport);
  }

  private handleTypeAssertionExpression(node: ts.Node): void {
    const tsTypeAssertion = node as ts.TypeAssertion;
    if (tsTypeAssertion.type.getText() === 'const') {
      this.incrementCounters(tsTypeAssertion, FaultID.ConstAssertion);
    } else {
      const autofix = this.autofixer?.fixTypeAssertion(tsTypeAssertion);
      this.incrementCounters(node, FaultID.TypeAssertion, autofix);
    }
  }

  private handleMethodDeclaration(node: ts.Node): void {
    const tsMethodDecl = node as ts.MethodDeclaration;
    TsUtils.getDecoratorsIfInSendableClass(tsMethodDecl)?.forEach((decorator) => {
      this.incrementCounters(decorator, FaultID.SendableClassDecorator);
    });
    let isStatic = false;
    if (tsMethodDecl.modifiers) {
      for (const mod of tsMethodDecl.modifiers) {
        if (mod.kind === ts.SyntaxKind.StaticKeyword) {
          isStatic = true;
          break;
        }
      }
    }
    if (tsMethodDecl.body && isStatic) {
      this.reportThisKeywordsInScope(tsMethodDecl.body);
    }
    if (!tsMethodDecl.type) {
      this.handleMissingReturnType(tsMethodDecl);
    }
    if (tsMethodDecl.asteriskToken) {
      this.incrementCounters(node, FaultID.GeneratorFunction);
    }
    this.filterOutDecoratorsDiagnostics(
      ts.getDecorators(tsMethodDecl),
      NON_RETURN_FUNCTION_DECORATORS,
      { begin: tsMethodDecl.parameters.end, end: tsMethodDecl.body?.getStart() ?? tsMethodDecl.parameters.end },
      FUNCTION_HAS_NO_RETURN_ERROR_CODE
    );
    if (this.options.arkts2 && tsMethodDecl.questionToken) {
      this.incrementCounters(tsMethodDecl.questionToken, FaultID.OptionalMethod);
    }
    this.handleInvalidIdentifier(tsMethodDecl);
    if (!this.tsUtils.isAbstractMethodInAbstractClass(node)) {
      this.handleTSOverload(tsMethodDecl);
    }
    this.checkDefaultParamBeforeRequired(tsMethodDecl);
    this.handleMethodInherit(tsMethodDecl);
    this.handleSdkGlobalApi(tsMethodDecl);
    this.handleLimitedVoidFunction(tsMethodDecl);
    this.checkVoidLifecycleReturn(tsMethodDecl);
  }

  private handleLimitedVoidFunction(node: ts.FunctionLikeDeclaration): void {
    const typeNode = node.type;
    if (!typeNode || !ts.isUnionTypeNode(typeNode)) {
      return;
    }
    const containsVoid = typeNode.types.some((t) => {
      return t.kind === ts.SyntaxKind.VoidKeyword;
    });
    if (this.options.arkts2 && containsVoid) {
      const autofix = this.autofixer?.fixLimitedVoidTypeFunction(node);
      this.incrementCounters(typeNode, FaultID.LimitedVoidType, autofix);
    }
  }

  private checkDefaultParamBeforeRequired(node: ts.FunctionLikeDeclarationBase): void {
    if (!this.options.arkts2) {
      return;
    }

    const params = node.parameters;
    let seenRequired = false;

    for (let i = params.length - 1; i >= 0; i--) {
      const param = params[i];

      const isOptional = !!param.initializer || !!param.questionToken;

      if (!isOptional) {
        seenRequired = true;
        continue;
      }

      if (seenRequired && param.initializer) {
        this.incrementCounters(param.name, FaultID.DefaultArgsBehindRequiredArgs);
      }
    }
  }

  private handleMethodInherit(node: ts.MethodDeclaration): void {
    if (!this.options.arkts2 || !node.name || !ts.isIdentifier(node.name)) {
      return;
    }

    const classDecl = node.parent;
    if (!ts.isClassDeclaration(classDecl)) {
      return;
    }

    const classType = this.tsTypeChecker.getTypeAtLocation(classDecl);
    const allBaseTypes = this.getAllBaseTypes(classType, classDecl);
    if (!allBaseTypes || allBaseTypes.length === 0) {
      return;
    }

    const methodName = node.name.text;

    for (const baseType of allBaseTypes) {
      const baseMethod = baseType.getProperty(methodName);
      if (!baseMethod) {
        continue;
      }

      const baseMethodDecl = baseMethod.declarations?.find(
        (d) => {
          return (ts.isMethodDeclaration(d) || ts.isMethodSignature(d)) &&
          this.tsTypeChecker.getTypeAtLocation(d.parent) === baseType;
        }
      ) as ts.MethodDeclaration | ts.MethodSignature;

      if (!baseMethodDecl) {
        continue;
      }

      this.checkMethodParameters(node, baseMethodDecl);

      this.checkMethodReturnType(node, baseMethodDecl);

      break;
    }
  }

  private getAllBaseTypes(type: ts.Type, classDecl: ts.ClassDeclaration): ts.Type[] | undefined {
    const baseClasses = type.getBaseTypes() || [];
    if (!classDecl.heritageClauses) {
      return baseClasses;
    }
    const interfaces: ts.Type[] = [];
    for (const clause of classDecl.heritageClauses) {
      if (clause.token !== ts.SyntaxKind.ImplementsKeyword) {
        continue;
      }
      for (const typeNode of clause.types) {
        const interfaceType = this.tsTypeChecker.getTypeAtLocation(typeNode);
        interfaces.push(interfaceType);
        const parentInterfaces = interfaceType.getBaseTypes();
        if (parentInterfaces) {
          interfaces.push(...parentInterfaces);
        }
      }
    }
    return [...baseClasses, ...interfaces];
  }

  /**
   * Checks method parameter compatibility
   * Derived parameter types must be same or wider than base (contravariance principle)
   */
  private checkMethodParameters(
    derivedMethod: ts.MethodDeclaration,
    baseMethod: ts.MethodDeclaration | ts.MethodSignature
  ): void {
    const derivedParams = derivedMethod.parameters;
    const baseParams = baseMethod.parameters;

    if (derivedParams.length !== baseParams.length) {
      this.incrementCounters(derivedMethod.name, FaultID.MethodInheritRule);
      return;
    }

    const paramCount = Math.min(derivedParams.length, baseParams.length);

    for (let i = 0; i < paramCount; i++) {
      const baseParamType = this.tsTypeChecker.getTypeAtLocation(baseParams[i]);
      const derivedParamType = this.tsTypeChecker.getTypeAtLocation(derivedParams[i]);

      if (!this.isTypeSameOrWider(baseParamType, derivedParamType)) {
        this.incrementCounters(derivedParams[i], FaultID.MethodInheritRule);
      }
    }
  }

  /**
   * Checks return type compatibility
   * Derived return type must be same or narrower than base (covariance principle)
   */
  private checkMethodReturnType(
    derivedMethod: ts.MethodDeclaration,
    baseMethod: ts.MethodDeclaration | ts.MethodSignature
  ): void {
    if (
      this.IsVoidTypeOnActualReturnType(baseMethod) &&
      derivedMethod.type &&
      !this.IsVoidTypeOnActualReturnType(derivedMethod)
    ) {
      this.incrementCounters(derivedMethod.type, FaultID.MethodInheritRule);
      return;
    }

    if (!baseMethod.type || !derivedMethod.type) {
      return;
    }

    const baseReturnType = this.tsTypeChecker.getTypeAtLocation(baseMethod.type);
    const derivedReturnType = this.tsTypeChecker.getTypeAtLocation(derivedMethod.type);

    if (this.isDerivedTypeAssignable(derivedReturnType, baseReturnType)) {
      return;
    }

    if (!this.isTypeAssignable(derivedReturnType, baseReturnType)) {
      this.incrementCounters(derivedMethod.type, FaultID.MethodInheritRule);
    }
  }

  private IsVoidTypeOnActualReturnType(method: ts.MethodDeclaration | ts.MethodSignature): boolean | undefined {
    let type: ts.Type | undefined;
    if (method.type) {
      type = this.tsTypeChecker.getTypeAtLocation(method.type);
    } else {
      const signature = this.tsTypeChecker.getSignatureFromDeclaration(method);
      if (signature) {
        type = this.tsTypeChecker.getReturnTypeOfSignature(signature);
      }
    }
    return type && TsUtils.isVoidType(type);
  }

  /**
   * Child type should include all types of parent type (be same or wider).
   * Returns true if every type in baseType is also included in derivedType.
   */
  private isTypeSameOrWider(baseType: ts.Type, derivedType: ts.Type): boolean {
    const baseTypeSet = new Set(this.flattenUnionTypes(baseType));
    const derivedTypeSet = new Set(this.flattenUnionTypes(derivedType));

    // Check if every type in baseType is also present in derivedType
    for (const typeStr of baseTypeSet) {
      if (!derivedTypeSet.has(typeStr)) {
        return false;
      }
    }

    return true;
  }

  // Checks structural assignability between two types.
  private isTypeAssignable(fromType: ts.Type, toType: ts.Type): boolean {
    if (this.isDerivedTypeAssignable(fromType, toType)) {
      return true;
    }
    const fromTypes = this.flattenUnionTypes(fromType);
    const toTypes = new Set(this.flattenUnionTypes(toType));

    // All types in `fromTypes` should exist in `toTypes` for assignability.
    return fromTypes.every((typeStr) => {
      return toTypes.has(typeStr);
    });
  }

  private isDerivedTypeAssignable(derivedType: ts.Type, baseType: ts.Type): boolean {
    const baseSymbol = baseType.getSymbol();
    const derivedSymbol = derivedType.getSymbol();

    if (!baseSymbol || !derivedSymbol) {
      return false;
    }
    const baseDeclarations = baseSymbol.getDeclarations();
    const derivedDeclarations = derivedSymbol.getDeclarations();

    if (!baseDeclarations || !derivedDeclarations) {
      return false;
    }
    const baseTypeNode = baseDeclarations[0];
    const derivedTypeNode = derivedDeclarations[0];

    if (ts.isClassDeclaration(baseTypeNode) && ts.isClassDeclaration(derivedTypeNode)) {
      const baseTypes = this.tsTypeChecker.getTypeAtLocation(derivedTypeNode).getBaseTypes();
      const baseTypesExtends = baseTypes?.some((t) => {
        return t === baseType;
      });
      if (baseTypesExtends) {
        return true;
      }
    }

    return false;
  }

  // Converts union types into an array of type strings for easy comparison.
  private flattenUnionTypes(type: ts.Type): string[] {
    if (type.isUnion()) {
      return type.types.map((t) => {
        return this.tsTypeChecker.typeToString(t);
      });
    }
    return [this.tsTypeChecker.typeToString(type)];
  }

  private checkClassImplementsMethod(classDecl: ts.ClassDeclaration, methodName: string): boolean {
    for (const member of classDecl.members) {
      if (member.name?.getText() === methodName) {
        if (ts.isPropertyDeclaration(member)) {
          this.incrementCounters(member, FaultID.MethodOverridingField);
        }
      }
    }
    return false;
  }

  private handleMethodSignature(node: ts.MethodSignature): void {
    const tsMethodSign = node;
    if (this.options.arkts2 && ts.isInterfaceDeclaration(node.parent)) {
      const methodName = node.name.getText();
      const interfaceName = node.parent.name.getText();
      const allClasses = TypeScriptLinter.getAllClassesFromSourceFile(this.sourceFile);
      const allInterfaces = TypeScriptLinter.getAllInterfaceFromSourceFile(this.sourceFile);
      allClasses.forEach((classDecl) => {
        if (this.classImplementsInterface(classDecl, interfaceName)) {
          this.checkClassImplementsMethod(classDecl, methodName);
        }
      });
      allInterfaces.forEach((interDecl) => {
        if (this.interfaceExtendsInterface(interDecl, interfaceName)) {
          this.checkInterfaceExtendsMethod(interDecl, methodName);
        }
      });
    }
    if (!tsMethodSign.type) {
      this.handleMissingReturnType(tsMethodSign);
    }
    if (this.options.arkts2 && tsMethodSign.questionToken) {
      this.incrementCounters(tsMethodSign.questionToken, FaultID.OptionalMethod);
    }
    this.handleInvalidIdentifier(tsMethodSign);
  }

  private interfaceExtendsInterface(interDecl: ts.InterfaceDeclaration, interfaceName: string): boolean {
    void this;
    if (!interDecl.heritageClauses) {
      return false;
    }
    return interDecl.heritageClauses.some((clause) => {
      return clause.types.some((type) => {
        return (
          ts.isExpressionWithTypeArguments(type) &&
          ts.isIdentifier(type.expression) &&
          type.expression.text === interfaceName
        );
      });
    });
  }

  private checkInterfaceExtendsMethod(interDecl: ts.InterfaceDeclaration, methodName: string): void {
    for (const member of interDecl.members) {
      if (member.name?.getText() === methodName) {
        if (ts.isPropertySignature(member)) {
          this.incrementCounters(member, FaultID.MethodOverridingField);
        }
      }
    }
  }

  private classImplementsInterface(classDecl: ts.ClassDeclaration, interfaceName: string): boolean {
    void this;
    if (!classDecl.heritageClauses) {
      return false;
    }
    return classDecl.heritageClauses.some((clause) => {
      return clause.types.some((type) => {
        return (
          ts.isExpressionWithTypeArguments(type) &&
          ts.isIdentifier(type.expression) &&
          type.expression.text === interfaceName
        );
      });
    });
  }

  private handleClassStaticBlockDeclaration(node: ts.Node): void {
    const classStaticBlockDecl = node as ts.ClassStaticBlockDeclaration;
    if (!ts.isClassDeclaration(classStaticBlockDecl.parent)) {
      return;
    }
    this.reportThisKeywordsInScope(classStaticBlockDecl.body);
  }

  private handleIdentifier(node: ts.Node): void {
    if (!ts.isIdentifier(node)) {
      return;
    }
    this.handleInterfaceImport(node);
    this.checkAsonSymbol(node);
    const tsIdentifier = node;
    this.handleTsInterop(tsIdentifier, () => {
      const parent = tsIdentifier.parent;
      if (ts.isPropertyAccessExpression(parent) || ts.isImportSpecifier(parent)) {
        return;
      }

      const type = this.tsTypeChecker.getTypeAtLocation(tsIdentifier);
      this.checkUsageOfTsTypes(type, tsIdentifier);
    });

    const tsIdentSym = this.tsUtils.trueSymbolAtLocation(tsIdentifier);
    if (!tsIdentSym) {
      return;
    }

    const isArkTs2 = this.options.arkts2;
    const isGlobalThis = tsIdentifier.text === 'globalThis';

    if (
      isGlobalThis &&
      (tsIdentSym.flags & ts.SymbolFlags.Module) !== 0 &&
      (tsIdentSym.flags & ts.SymbolFlags.Transient) !== 0
    ) {
      this.handleGlobalThisCase(tsIdentifier, isArkTs2);
    } else {
      if (isArkTs2) {
        this.checkLimitedStdlibApi(tsIdentifier, tsIdentSym);
      }
      this.handleRestrictedValues(tsIdentifier, tsIdentSym);
    }

    if (isArkTs2 && this.tsTypeChecker.isArgumentsSymbol(tsIdentSym)) {
      this.incrementCounters(node, FaultID.ArgumentsObject);
    }

    if (isArkTs2) {
      this.checkWorkerSymbol(tsIdentSym, node);
      this.checkCollectionsSymbol(tsIdentSym, node);
      this.checkConcurrencySymbol(tsIdentSym, node);
    }
  }

  private handlePropertyDescriptorInScenarios(node: ts.Node): void {
    if (ts.isVariableDeclaration(node)) {
      const name = node.name;
      this.handlePropertyDescriptor(name);

      const type = node.type;
      if (!type || !ts.isTypeReferenceNode(type)) {
        return;
      }
      const typeName = type.typeName;
      this.handlePropertyDescriptor(typeName);
    }

    if (ts.isParameter(node)) {
      const name = node.name;
      this.handlePropertyDescriptor(name);

      const type = node.type;
      if (!type || !ts.isTypeReferenceNode(type)) {
        return;
      }
      const typeName = type.typeName;
      this.handlePropertyDescriptor(typeName);
    }

    if (ts.isPropertyAccessExpression(node)) {
      const name = node.name;
      this.handlePropertyDescriptor(name);

      const expression = node.expression;
      this.handlePropertyDescriptor(expression);
    }
  }

  private handlePropertyDescriptor(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    const symbol = this.tsUtils.trueSymbolAtLocation(node);
    if (!symbol || !ts.isIdentifier(node)) {
      return;
    }
    const tsIdentifier = node;
    const type = this.tsTypeChecker.getTypeOfSymbolAtLocation(symbol, tsIdentifier);

    const typeSymbol = type.getSymbol();
    const typeName = typeSymbol ? typeSymbol.getName() : symbol.getName();

    const noPropertyDescriptorSet = TypeScriptLinter.globalApiInfo.get(BuiltinProblem.BuiltinNoPropertyDescriptor);
    if (!noPropertyDescriptorSet) {
      return;
    }

    const matchedApi = [...noPropertyDescriptorSet].some((apiInfoItem) => {
      if (apiInfoItem.api_info.parent_api?.length <= 0) {
        return false;
      }
      const apiInfoParentName = apiInfoItem.api_info.parent_api[0].api_name;
      const apiTypeName = apiInfoItem.api_info.method_return_type;
      const isSameApi = apiInfoParentName === typeName || apiTypeName === typeName;
      const decl = TsUtils.getDeclaration(typeSymbol ? typeSymbol : symbol);
      const sourceFileName = path.normalize(decl?.getSourceFile().fileName || '');
      const isSameFile = sourceFileName.endsWith(path.normalize(apiInfoItem.file_path));
      return isSameFile && isSameApi;
    });

    if (matchedApi) {
      this.incrementCounters(tsIdentifier, FaultID.NoPropertyDescriptor);
    }
  }

  private handleGlobalThisCase(node: ts.Identifier, isArkTs2: boolean | undefined): void {
    let faultId = FaultID.GlobalThis;
    let targetNode: ts.Node = node;

    if (!isArkTs2) {
      this.incrementCounters(targetNode, faultId);
      return;
    }
    faultId = FaultID.GlobalThisError;

    if (ts.isPropertyAccessExpression(node.parent)) {
      const parentExpression = node.parent.parent;
      if (
        parentExpression &&
        ts.isBinaryExpression(parentExpression) &&
        parentExpression.operatorToken.kind === ts.SyntaxKind.EqualsToken
      ) {
        targetNode = parentExpression;
      } else {
        targetNode = node.parent;
      }
    }

    this.incrementCounters(targetNode, faultId);
  }

  // hard-coded alternative to TypeScriptLinter.advancedClassChecks
  private isAllowedClassValueContext(tsIdentifier: ts.Identifier): boolean {
    let ctx: ts.Node = tsIdentifier;
    while (ts.isPropertyAccessExpression(ctx.parent) || ts.isQualifiedName(ctx.parent)) {
      ctx = ctx.parent;
    }
    if (ts.isPropertyAssignment(ctx.parent) && ts.isObjectLiteralExpression(ctx.parent.parent)) {
      ctx = ctx.parent.parent;
    }
    if (ts.isArrowFunction(ctx.parent) && ctx.parent.body === ctx) {
      ctx = ctx.parent;
    }

    if (ts.isCallExpression(ctx.parent) || ts.isNewExpression(ctx.parent)) {
      const callee = ctx.parent.expression;
      const isAny = TsUtils.isAnyType(this.tsTypeChecker.getTypeAtLocation(callee));
      const isDynamic = isAny || this.tsUtils.hasLibraryType(callee);
      if (callee !== ctx && isDynamic) {
        return true;
      }
    }
    return false;
  }

  private isStdlibClassVarDecl(ident: ts.Identifier, sym: ts.Symbol): boolean {

    /*
     * Most standard JS classes are defined in TS stdlib as ambient global
     * variables with interface constructor type and require special check
     * when they are being referenced in code.
     */

    if (
      !isStdLibrarySymbol(sym) ||
      !sym.valueDeclaration ||
      !ts.isVariableDeclaration(sym.valueDeclaration) ||
      !TsUtils.isAmbientNode(sym.valueDeclaration)
    ) {
      return false;
    }

    /*
     * issue 24075: TS supports calling the constructor of built-in types
     * as function (without 'new' keyword): `const a = Number('10')`
     * Such cases need to be filtered out.
     */
    if (ts.isCallExpression(ident.parent) && ident.parent.expression === ident) {
      return false;
    }

    const classVarDeclType = StdClassVarDecls.get(sym.name);
    if (!classVarDeclType) {
      return false;
    }
    const declType = this.tsTypeChecker.getTypeAtLocation(ident);
    return declType.symbol && declType.symbol.name === classVarDeclType;
  }

  private handleRestrictedValues(tsIdentifier: ts.Identifier, tsIdentSym: ts.Symbol): void {
    const illegalValues =
      ts.SymbolFlags.ConstEnum |
      ts.SymbolFlags.RegularEnum |
      ts.SymbolFlags.ValueModule |
      (this.options.advancedClassChecks ? 0 : ts.SymbolFlags.Class);

    /*
     * If module name is duplicated by another declaration, this increases the possibility
     * of finding a lot of false positives. Thus, do not check further in that case.
     */
    if ((tsIdentSym.flags & ts.SymbolFlags.ValueModule) !== 0) {
      if (!!tsIdentSym && TsUtils.symbolHasDuplicateName(tsIdentSym, ts.SyntaxKind.ModuleDeclaration)) {
        return;
      }
    }

    if (
      (tsIdentSym.flags & illegalValues) === 0 && !this.isStdlibClassVarDecl(tsIdentifier, tsIdentSym) ||
      isStruct(tsIdentSym) ||
      !identiferUseInValueContext(tsIdentifier, tsIdentSym)
    ) {
      return;
    }

    if ((tsIdentSym.flags & ts.SymbolFlags.Class) !== 0) {
      if (!this.options.advancedClassChecks && this.isAllowedClassValueContext(tsIdentifier)) {
        return;
      }
    }

    this.handleIllegalSymbolUsage(tsIdentifier, tsIdentSym);
  }

  private handleIllegalSymbolUsage(tsIdentifier: ts.Identifier, tsIdentSym: ts.Symbol): void {
    if (tsIdentSym.flags & ts.SymbolFlags.ValueModule) {
      this.incrementCounters(tsIdentifier, FaultID.NamespaceAsObject);
      return;
    }

    const typeName = tsIdentifier.getText();
    const isWrapperObject = typeName === 'Number' || typeName === 'String' || typeName === 'Boolean';
    if (isWrapperObject) {
      return;
    }

    // Special-case element-access cast for autofix: (X as object)["prop"]
    const asExpr = tsIdentifier.parent;
    let elemAccess: ts.ElementAccessExpression | undefined;

    if (
      ts.isAsExpression(asExpr) &&
      ts.isParenthesizedExpression(asExpr.parent) &&
      ts.isElementAccessExpression(asExpr.parent.parent) &&
      ts.isStringLiteral(asExpr.parent.parent.argumentExpression)
    ) {
      // only care if it’s literally “as object” && static-class casts
      if (asExpr.type.getText() === 'object' && tsIdentSym.flags & ts.SymbolFlags.Class) {
        elemAccess = asExpr.parent.parent;
      }
    }

    const autofix = elemAccess ? this.autofixer?.fixPropertyAccessByIndex(elemAccess) : undefined;
    const faultId = this.options.arkts2 ? FaultID.ClassAsObjectError : FaultID.ClassAsObject;
    this.incrementCounters(tsIdentifier, faultId, autofix);
  }

  private isElementAcessAllowed(type: ts.Type, argType: ts.Type): boolean {
    if (type.isUnion()) {
      for (const t of type.types) {
        if (!this.isElementAcessAllowed(t, argType)) {
          return false;
        }
      }
      return true;
    }

    const typeNode = this.tsTypeChecker.typeToTypeNode(type, undefined, ts.NodeBuilderFlags.None);

    if (this.tsUtils.isArkTSCollectionsArrayLikeType(type)) {
      return this.tsUtils.isNumberLikeType(argType);
    }

    return (
      this.tsUtils.isLibraryType(type) ||
      TsUtils.isAnyType(type) ||
      this.tsUtils.isOrDerivedFrom(type, this.tsUtils.isIndexableArray) ||
      this.tsUtils.isOrDerivedFrom(type, TsUtils.isTuple) ||
      this.tsUtils.isOrDerivedFrom(type, this.tsUtils.isStdRecordType) ||
      this.tsUtils.isOrDerivedFrom(type, this.tsUtils.isStringType) ||
      !this.options.arkts2 &&
        (this.tsUtils.isOrDerivedFrom(type, this.tsUtils.isStdMapType) || TsUtils.isIntrinsicObjectType(type)) ||
      TsUtils.isEnumType(type) ||
      // we allow EsObject here beacuse it is reported later using FaultId.EsObjectType
      TsUtils.isEsValueType(typeNode)
    );
  }

  private handleElementAccessExpression(node: ts.Node): void {
    const tsElementAccessExpr = node as ts.ElementAccessExpression;
    const tsElemAccessBaseExprType = this.tsUtils.getNonNullableType(
      this.tsUtils.getTypeOrTypeConstraintAtLocation(tsElementAccessExpr.expression)
    );
    const tsElemAccessArgType = this.tsTypeChecker.getTypeAtLocation(tsElementAccessExpr.argumentExpression);

    if (this.tsUtils.hasEsObjectType(tsElementAccessExpr.expression)) {
      const faultId = this.options.arkts2 ? FaultID.EsValueTypeError : FaultID.EsValueType;
      this.incrementCounters(node, faultId);
    }
    if (this.tsUtils.isOrDerivedFrom(tsElemAccessBaseExprType, this.tsUtils.isIndexableArray)) {
      this.handleIndexNegative(node);
    }
    this.checkPropertyAccessByIndex(tsElementAccessExpr, tsElemAccessBaseExprType, tsElemAccessArgType);
    this.checkArrayUsageWithoutBound(tsElementAccessExpr);
    this.checkArrayIndexType(tsElemAccessBaseExprType, tsElemAccessArgType, tsElementAccessExpr);
    this.fixJsImportElementAccessExpression(tsElementAccessExpr);
    this.checkInterOpImportJsIndex(tsElementAccessExpr);
    this.checkEnumGetMemberValue(tsElementAccessExpr);
  }

  private checkPropertyAccessByIndex(
    tsElementAccessExpr: ts.ElementAccessExpression,
    tsElemAccessBaseExprType: ts.Type,
    tsElemAccessArgType: ts.Type
  ): void {
    const tsElementAccessExprSymbol = this.tsUtils.trueSymbolAtLocation(tsElementAccessExpr.expression);

    const isSet = TsUtils.isSetExpression(tsElementAccessExpr);
    const isSetIndexable =
      isSet &&
      this.tsUtils.isSetIndexableType(
        tsElemAccessBaseExprType,
        tsElemAccessArgType,
        this.tsTypeChecker.getTypeAtLocation((tsElementAccessExpr.parent as ts.BinaryExpression).right)
      );

    const isGet = !isSet;
    const isGetIndexable = isGet && this.tsUtils.isGetIndexableType(tsElemAccessBaseExprType, tsElemAccessArgType);

    if (
      // unnamed types do not have symbol, so need to check that explicitly
      this.tsUtils.isLibrarySymbol(tsElementAccessExprSymbol) ||
      ts.isArrayLiteralExpression(tsElementAccessExpr.expression) ||
      this.isElementAcessAllowed(tsElemAccessBaseExprType, tsElemAccessArgType) ||
      this.options.arkts2 && isGetIndexable ||
      this.options.arkts2 && isSetIndexable
    ) {
      return;
    }

    if (this.isStaticClassAccess(tsElementAccessExpr)) {
      return;
    }

    const autofix = this.autofixer?.fixPropertyAccessByIndex(tsElementAccessExpr);
    this.incrementCounters(tsElementAccessExpr, FaultID.PropertyAccessByIndex, autofix);
  }

  /**
   * Returns true if this element-access is a static-class cast (e.g., (A as object)["foo"]).
   */
  private isStaticClassAccess(expr: ts.ElementAccessExpression): boolean {
    const inner = expr.expression;
    if (
      ts.isParenthesizedExpression(inner) &&
      ts.isAsExpression(inner.expression) &&
      ts.isIdentifier(inner.expression.expression)
    ) {
      const sym = this.tsTypeChecker.getSymbolAtLocation(inner.expression.expression);
      return !!(sym && sym.flags & ts.SymbolFlags.Class);
    }
    return false;
  }

  private checkInterOpImportJsIndex(expr: ts.ElementAccessExpression): void {
    if (!this.useStatic || !this.options.arkts2) {
      return;
    }

    const exprSym = this.tsUtils.trueSymbolAtLocation(expr.expression);
    if (!exprSym) {
      return;
    }

    const exprDecl = TsUtils.getDeclaration(exprSym);
    if (!exprDecl || !ts.isVariableDeclaration(exprDecl)) {
      return;
    }

    const initializer = exprDecl.initializer;
    if (!initializer || !ts.isPropertyAccessExpression(initializer)) {
      return;
    }

    const initSym = this.tsUtils.trueSymbolAtLocation(initializer.expression);
    if (!initSym) {
      return;
    }

    const initDecl = TsUtils.getDeclaration(initSym);
    if (!initDecl?.getSourceFile().fileName.endsWith(EXTNAME_JS)) {
      return;
    }

    if (ts.isBinaryExpression(expr.parent) && expr.parent.operatorToken.kind === ts.SyntaxKind.EqualsToken) {
      const autofix = this.autofixer?.fixInteropArrayBinaryExpression(expr.parent);
      this.incrementCounters(expr.parent, FaultID.InterOpImportJsIndex, autofix);
    } else {
      const autofix = this.autofixer?.fixInteropArrayElementAccessExpression(expr);
      this.incrementCounters(expr, FaultID.InterOpImportJsIndex, autofix);
    }
  }

  private checkArrayIndexType(exprType: ts.Type, argType: ts.Type, expr: ts.ElementAccessExpression): void {
    if (!this.options.arkts2 || !this.tsUtils.isOrDerivedFrom(exprType, this.tsUtils.isIndexableArray)) {
      return;
    }

    const validStringLiteralTypes = [
      STRINGLITERAL_INT,
      STRINGLITERAL_BYTE,
      STRINGLITERAL_SHORT,
      STRINGLITERAL_LONG,
      STRINGLITERAL_CHAR
    ];
    const argTypeString = this.tsTypeChecker.typeToString(argType);

    if (this.tsUtils.isNumberLikeType(argType)) {
      this.handleNumericArgument(expr.argumentExpression, argType);
    } else if (!validStringLiteralTypes.includes(argTypeString)) {
      this.incrementCounters(expr.argumentExpression, FaultID.ArrayIndexExprType);
    }
  }

  private handleNumericArgument(argExpr: ts.Expression, argType: ts.Type): void {
    const isNumericLiteral = ts.isNumericLiteral(argExpr);
    const argText = argExpr.getText();
    const argValue = Number(argText);

    if (isNumericLiteral) {
      const isInteger = Number.isInteger(argValue);
      const containsDot = argText.includes('.');

      if (!isInteger || containsDot) {
        const autofix = this.autofixer?.fixArrayIndexExprType(argExpr);
        this.incrementCounters(argExpr, FaultID.ArrayIndexExprType, autofix);
      }
    } else if (this.tsTypeChecker.typeToString(argType) === 'number') {
      if (this.isArrayIndexValidNumber(argExpr)) {
        return;
      }
      const autofix = this.autofixer?.fixArrayIndexExprType(argExpr);
      this.incrementCounters(argExpr, FaultID.ArrayIndexExprType, autofix);
    } else {
      this.checkNumericArgumentDeclaration(argExpr);
    }

    if (ts.isConditionalExpression(argExpr)) {
      this.incrementCounters(argExpr, FaultID.ArrayIndexExprType);
    }
  }

  private checkNumericArgumentDeclaration(argExpr: ts.Expression): void {
    const symbol = this.tsTypeChecker.getSymbolAtLocation(argExpr);

    if (!symbol) {
      return;
    }

    const declarations = symbol.getDeclarations();
    if (!declarations || declarations.length === 0) {
      return;
    }

    const firstDeclaration = declarations[0] as ts.VariableDeclaration;
    const initializer = firstDeclaration.initializer;
    const initializerText = initializer ? initializer.getText() : 'undefined';
    const isNumericInitializer = initializer && ts.isNumericLiteral(initializer);
    const initializerNumber = isNumericInitializer ? Number(initializerText) : NaN;
    const isUnsafeNumber = isNumericInitializer && !Number.isInteger(initializerNumber);

    if (isUnsafeNumber) {
      const autofix = this.autofixer?.fixArrayIndexExprType(argExpr);
      this.incrementCounters(argExpr, FaultID.ArrayIndexExprType, autofix);
    }

    if (initializerText === 'undefined') {
      this.handleUndefinedInitializer(argExpr, firstDeclaration);
    }
  }

  private evaluateValueFromDeclaration(argExpr: ts.Expression): number | null | 'skip' {
    const declaration = this.tsUtils.getDeclarationNode(argExpr);
    if (!declaration) {
      return null;
    }

    if (!ts.isVariableDeclaration(declaration)) {
      return null;
    }

    if (declaration.type !== undefined) {
      return 'skip';
    }

    const initializer = declaration.initializer;
    if (!initializer) {
      return null;
    }

    if (!ts.isNumericLiteral(initializer)) {
      return null;
    }

    const numericValue = Number(initializer.text);
    if (!Number.isInteger(numericValue)) {
      return null;
    }

    return numericValue;
  }

  private isArrayIndexValidNumber(argExpr: ts.Expression): boolean {
    let evaluatedValue: number | null = null;
    if (ts.isParenthesizedExpression(argExpr)) {
      return this.isArrayIndexValidNumber(argExpr.expression);
    }

    if (ts.isBinaryExpression(argExpr)) {
      evaluatedValue = this.evaluateNumericValueFromBinaryExpression(argExpr);
    } else {
      const evalResult = this.evaluateValueFromDeclaration(argExpr);
      if (evalResult === 'skip') {
        return false;
      }
      evaluatedValue = evalResult;
    }

    if (evaluatedValue === null) {
      return false;
    }

    const valueString = String(evaluatedValue);

    if (!Number.isInteger(evaluatedValue)) {
      return false;
    }
    if (valueString.includes('.')) {
      return false;
    }

    return evaluatedValue >= 0;
  }

  private handleUndefinedInitializer(argExpr: ts.Expression, declaration: ts.VariableDeclaration): void {
    if (ts.isParameter(declaration)) {
      const autofix = this.autofixer?.fixArrayIndexExprType(argExpr);
      this.incrementCounters(argExpr, FaultID.ArrayIndexExprType, autofix);
    } else {
      this.incrementCounters(argExpr, FaultID.ArrayIndexExprType);
    }
  }

  private handleEnumMember(node: ts.Node): void {
    const tsEnumMember = node as ts.EnumMember;
    const tsEnumMemberType = this.tsTypeChecker.getTypeAtLocation(tsEnumMember);
    const constVal = this.tsTypeChecker.getConstantValue(tsEnumMember);
    const tsEnumMemberName = tsEnumMember.name;
    if (this.options.arkts2) {
      this.handleInvalidIdentifier(tsEnumMember);
      if (ts.isStringLiteral(tsEnumMemberName)) {
        this.handleStringLiteralEnumMember(tsEnumMember, tsEnumMemberName, node);
      }
    }

    if (tsEnumMember.initializer && !this.tsUtils.isValidEnumMemberInit(tsEnumMember.initializer)) {
      this.incrementCounters(node, FaultID.EnumMemberNonConstInit);
    }
    // check for type - all members should be of same type
    const enumDecl = tsEnumMember.parent;
    const firstEnumMember = enumDecl.members[0];
    const firstEnumMemberType = this.tsTypeChecker.getTypeAtLocation(firstEnumMember);
    const firstElewmVal = this.tsTypeChecker.getConstantValue(firstEnumMember);
    this.handleEnumNotSupportFloat(tsEnumMember);

    /*
     * each string enum member has its own type
     * so check that value type is string
     */
    if (
      constVal !== undefined &&
      typeof constVal === STRINGLITERAL_STRING &&
      firstElewmVal !== undefined &&
      typeof firstElewmVal === STRINGLITERAL_STRING
    ) {
      return;
    }
    if (
      constVal !== undefined &&
      typeof constVal === STRINGLITERAL_NUMBER &&
      firstElewmVal !== undefined &&
      typeof firstElewmVal === STRINGLITERAL_NUMBER
    ) {
      return;
    }
    if (firstEnumMemberType !== tsEnumMemberType) {
      this.incrementCounters(node, FaultID.EnumMemberNonConstInit);
    }
  }

  private handleStringLiteralEnumMember(
    tsEnumMember: ts.EnumMember,
    tsEnumMemberName: ts.StringLiteral,
    node: ts.Node
  ): void {
    const autofix = this.autofixer?.fixLiteralAsPropertyNamePropertyName(tsEnumMemberName, tsEnumMember);
    this.incrementCounters(node, FaultID.LiteralAsPropertyName, autofix);
  }

  private handleEnumNotSupportFloat(enumMember: ts.EnumMember): void {
    if (!this.options.arkts2) {
      return;
    }
    const initializer = enumMember.initializer;
    if (!initializer) {
      return;
    }
    let value;
    if (ts.isNumericLiteral(initializer)) {
      value = parseFloat(initializer.text);
    } else if (ts.isPrefixUnaryExpression(initializer)) {
      const operand = initializer.operand;
      value = ts.isNumericLiteral(operand) ? parseFloat(operand.text) : value;
    } else {
      return;
    }

    if (!Number.isInteger(value)) {
      this.incrementCounters(enumMember, FaultID.EnumMemberNonConstInit);
    }
  }

  private handleExportAssignment(node: ts.Node): void {
    const exportAssignment = node as ts.ExportAssignment;
    if (exportAssignment.isExportEquals) {
      this.incrementCounters(node, FaultID.ExportAssignment);
    }

    if (!TypeScriptLinter.inSharedModule(node)) {
      return;
    }

    if (!this.tsUtils.isShareableEntity(exportAssignment.expression)) {
      this.incrementCounters(exportAssignment.expression, FaultID.SharedModuleExports);
    }
  }

  private processCalleeSym(calleeSym: ts.Symbol, tsCallExpr: ts.CallExpression): void {
    const name = calleeSym.getName();
    const parName = this.tsUtils.getParentSymbolName(calleeSym);
    if (!this.options.arkts2) {
      this.handleStdlibAPICall(tsCallExpr, calleeSym, name, parName);
      this.handleFunctionApplyBindPropCall(tsCallExpr, calleeSym);
    } else {
      this.handleBuiltinThisArgs(tsCallExpr, calleeSym, name, parName);
    }
    if (TsUtils.symbolHasEsObjectType(calleeSym)) {
      const faultId = this.options.arkts2 ? FaultID.EsValueTypeError : FaultID.EsValueType;
      this.incrementCounters(tsCallExpr, faultId);
    }
    // Need to process Symbol call separately in order to not report two times when using Symbol API
    if (this.options.arkts2 && this.tsUtils.isStdSymbol(calleeSym)) {
      this.incrementCounters(tsCallExpr, FaultID.SymbolType);
    }

    if (this.options.arkts2 && calleeSym.getEscapedName() === 'pow' && isStdLibrarySymbol(calleeSym)) {
      this.incrementCounters(tsCallExpr, FaultID.MathPow);
    }

    if (this.options.arkts2 && calleeSym.getEscapedName() === 'RegExp' && isStdLibrarySymbol(calleeSym)) {
      const autofix = this.autofixer?.fixRegularExpressionLiteral(tsCallExpr);
      this.incrementCounters(tsCallExpr, FaultID.RegularExpressionLiteral, autofix);
    }
  }

  private handleSdkPropertyAccessByIndex(tsCallExpr: ts.CallExpression): void {
    const propertyAccessNode = tsCallExpr.expression as ts.PropertyAccessExpression;
    if (!ts.isPropertyAccessExpression(propertyAccessNode)) {
      return;
    }

    const funcName = propertyAccessNode.name;
    const indexedTypeSdkInfos = Array.from(TypeScriptLinter.indexedTypeSet);
    const isCallInDeprecatedApi = indexedTypeSdkInfos.some((indexedTypeSdkInfo) => {
      const isApiNameMismatch = funcName?.getText() !== indexedTypeSdkInfo.api_info.api_name;
      if (isApiNameMismatch) {
        return false;
      }

      const funcDecls = this.tsTypeChecker.getTypeAtLocation(funcName).symbol?.declarations;
      return funcDecls?.some((declaration) => {
        const interfaceDecl = declaration.parent as ts.InterfaceDeclaration;
        if (!(ts.isMethodSignature(declaration) && ts.isInterfaceDeclaration(interfaceDecl))) {
          return false;
        }
        const declFileFromJson = path.normalize(interfaceDecl.getSourceFile().fileName);
        const declFileFromSdk = path.normalize(indexedTypeSdkInfo.file_path);
        const isSameSdkFilePath = declFileFromJson.endsWith(declFileFromSdk);
        const interfaceNameData = indexedTypeSdkInfo.api_info.parent_api[0].api_name;
        const isSameInterfaceName = interfaceDecl.name.getText() === interfaceNameData;
        return isSameSdkFilePath && isSameInterfaceName;
      });
    });
    if (isCallInDeprecatedApi) {
      this.incrementCounters(tsCallExpr.expression, FaultID.PropertyAccessByIndexFromSdk);
    }
  }

  private handleBuiltinCtorCallSignature(tsCallExpr: ts.CallExpression | ts.TypeReferenceNode): void {
    if (!this.options.arkts2) {
      return;
    }
    const node = ts.isCallExpression(tsCallExpr) ? tsCallExpr.expression : tsCallExpr.typeName;
    const constructorType = this.tsTypeChecker.getTypeAtLocation(node);
    const callSignatures = constructorType.getCallSignatures();
    if (callSignatures.length === 0 || BUILTIN_DISABLE_CALLSIGNATURE.includes(node.getText())) {
      return;
    }
    const isSameApi = callSignatures.some((callSignature) => {
      const callSignatureDecl = callSignature.getDeclaration();
      if (!ts.isCallSignatureDeclaration(callSignatureDecl)) {
        return false;
      }
      const parentDecl = callSignatureDecl.parent;
      const parentName = ts.isInterfaceDeclaration(parentDecl) ? parentDecl.name.getText() : '';
      const BultinNoCtorFuncApiInfoSet = TypeScriptLinter.globalApiInfo.get(BuiltinProblem.BuiltinNoCtorFunc);
      if (!BultinNoCtorFuncApiInfoSet) {
        return false;
      }
      const isSameApi = [...BultinNoCtorFuncApiInfoSet].some((apiInfoItem) => {
        if (apiInfoItem.api_info.parent_api?.length <= 0) {
          return false;
        }
        const apiInfoParentName = apiInfoItem.api_info.parent_api[0].api_name;
        return apiInfoParentName === parentName;
      });
      return isSameApi;
    });
    if (isSameApi) {
      this.incrementCounters(node, FaultID.BuiltinNoCtorFunc);
    }
  }

  private handleCallExpression(node: ts.Node): void {
    const tsCallExpr = node as ts.CallExpression;
    this.handleStateStyles(tsCallExpr);
    this.handleBuiltinCtorCallSignature(tsCallExpr);
    this.handleSdkConstructorIfaceForCallExpression(tsCallExpr);
    if (this.options.arkts2 && tsCallExpr.typeArguments !== undefined) {
      this.handleSdkPropertyAccessByIndex(tsCallExpr);
    }
    const calleeSym = this.tsUtils.trueSymbolAtLocation(tsCallExpr.expression);
    const callSignature = this.tsTypeChecker.getResolvedSignature(tsCallExpr);
    this.handleImportCall(tsCallExpr);
    this.handleRequireCall(tsCallExpr);
    if (calleeSym !== undefined) {
      this.processCalleeSym(calleeSym, tsCallExpr);
    }
    if (callSignature !== undefined) {
      if (!this.tsUtils.isLibrarySymbol(calleeSym)) {
        this.handleStructIdentAndUndefinedInArgs(tsCallExpr, callSignature);
        this.handleGenericCallWithNoTypeArgs(tsCallExpr, callSignature);
      } else if (this.options.arkts2) {
        this.handleGenericCallWithNoTypeArgs(tsCallExpr, callSignature);
      }
      this.handleNotsLikeSmartTypeOnCallExpression(tsCallExpr, callSignature);
    }
    this.handleInteropForCallExpression(tsCallExpr);
    this.handleLibraryTypeCall(tsCallExpr);
    if (
      ts.isPropertyAccessExpression(tsCallExpr.expression) &&
      this.tsUtils.hasEsObjectType(tsCallExpr.expression.expression)
    ) {
      const faultId = this.options.arkts2 ? FaultID.EsValueTypeError : FaultID.EsValueType;
      this.incrementCounters(node, faultId);
    }
    if (
      !ts.isExpressionStatement(tsCallExpr.parent) &&
      !ts.isVoidExpression(tsCallExpr.parent) &&
      !ts.isArrowFunction(tsCallExpr.parent) &&
      !(ts.isConditionalExpression(tsCallExpr.parent) && ts.isExpressionStatement(tsCallExpr.parent.parent))
    ) {
      this.handleLimitedVoidWithCall(tsCallExpr);
    }
    this.handleAppStorageCallExpression(tsCallExpr);
    this.fixJsImportCallExpression(tsCallExpr);
    this.handleInteropForCallJSExpression(tsCallExpr, calleeSym, callSignature);
    this.handleNoTsLikeFunctionCall(tsCallExpr);
    this.handleObjectLiteralInFunctionArgs(tsCallExpr);
    this.handleSdkGlobalApi(tsCallExpr);
    this.handleObjectLiteralAssignmentToClass(tsCallExpr);
    this.checkRestrictedAPICall(tsCallExpr);
  }

  handleNoTsLikeFunctionCall(callExpr: ts.CallExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    const expression = callExpr.expression;
    const type = this.tsTypeChecker.getTypeAtLocation(expression);
    const typeText = this.tsTypeChecker.typeToString(type);
    if (typeText === LIKE_FUNCTION) {
      const autofix = this.autofixer?.fixNoTsLikeFunctionCall(callExpr);
      this.incrementCounters(expression, FaultID.ExplicitFunctionType, autofix);
    }
  }

  private handleAppStorageCallExpression(tsCallExpr: ts.CallExpression): void {
    if (!this.options.arkts2 || !tsCallExpr) {
      return;
    }

    if (!TsUtils.isAppStorageAccess(tsCallExpr)) {
      return;
    }

    let varDecl: ts.VariableDeclaration | undefined;
    let parent = tsCallExpr.parent;

    while (parent) {
      if (ts.isVariableDeclaration(parent)) {
        varDecl = parent;
        break;
      }
      parent = parent.parent;
    }

    if (!varDecl || varDecl.type) {
      return;
    }
    const callReturnType = this.tsTypeChecker.getTypeAtLocation(tsCallExpr);
    const isNumberReturnType = callReturnType.flags & ts.TypeFlags.Number;
    const isNumberGeneric = ((): boolean => {
      if (tsCallExpr.typeArguments?.length !== 1) {
        return false;
      }
      const callText = tsCallExpr.getText();
      if (callText.startsWith('Array<') || callText.startsWith('Set<') || callText.startsWith('Map<')) {
        return false;
      }
      const typeArg = tsCallExpr.typeArguments[0];
      if (typeArg.kind === ts.SyntaxKind.NumberKeyword) {
        return true;
      }

      if (ts.isTypeReferenceNode(typeArg)) {
        return ts.isIdentifier(typeArg.typeName) && typeArg.typeName.text === STRINGLITERAL_NUMBER;
      }
      return typeArg.getText().trim() === STRINGLITERAL_NUMBER;
    })();

    if (isNumberGeneric && !isNumberReturnType) {
      const autofix = this.autofixer?.fixAppStorageCallExpression(tsCallExpr);
      this.incrementCounters(tsCallExpr, FaultID.NumericSemantics, autofix);
    }
  }

  handleInteropForCallJSExpression(
    tsCallExpr: ts.CallExpression,
    sym: ts.Symbol | undefined,
    callSignature: ts.Signature | undefined
  ): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }
    if (ts.isAwaitExpression(tsCallExpr.parent) || ts.isTypeOfExpression(tsCallExpr.parent)) {
      return;
    }

    if (!callSignature || this.isDeclaredInArkTs2(callSignature)) {
      return;
    }

    if (!sym?.declarations?.[0]?.getSourceFile().fileName.endsWith(EXTNAME_JS)) {
      return;
    }

    const autofix = this.autofixer?.fixInteropInvokeExpression(tsCallExpr);

    this.incrementCounters(
      tsCallExpr,
      ts.isPropertyAccessExpression(tsCallExpr.expression) ? FaultID.InteropCallObjectMethods : FaultID.CallJSFunction,
      autofix
    );
  }

  private handleInteropForCallExpression(tsCallExpr: ts.CallExpression): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    const callSignature = this.tsTypeChecker.getResolvedSignature(tsCallExpr);
    if (!callSignature) {
      return;
    }

    if (!this.isDeclaredInArkTs2(callSignature)) {
      return;
    }

    this.checkForForbiddenAPIs(callSignature, tsCallExpr);
  }

  private isExportedEntityDeclaredInJs(exportDecl: ts.ExportDeclaration): boolean {
    if (!this.options.arkts2 || !this.useStatic) {
      return false;
    }

    // For named exports with braces { ... }
    if (exportDecl.exportClause && ts.isNamedExports(exportDecl.exportClause)) {
      for (const exportSpecifier of exportDecl.exportClause.elements) {
        const identifier = exportSpecifier.name;
        if (this.tsUtils.isImportedFromJS(identifier)) {
          return true;
        }
      }
    }

    // For namespace exports (export * as namespace from ...)
    if (exportDecl.exportClause && ts.isNamespaceExport(exportDecl.exportClause)) {
      const namespaceIdentifier = exportDecl.exportClause.name;
      if (this.tsUtils.isImportedFromJS(namespaceIdentifier)) {
        return true;
      }
    }

    return false;
  }

  private isDeclaredInArkTs2(callSignature: ts.Signature): boolean | undefined {
    const declarationSourceFile = callSignature?.declaration?.getSourceFile();
    if (!declarationSourceFile) {
      return undefined;
    }
    if (!declarationSourceFile.statements) {
      return undefined;
    }

    if (this.tsUtils.isArkts12File(declarationSourceFile)) {
      return true;
    }
    return false;
  }

  private checkRestrictedAPICall(node: ts.CallExpression): void {
    if (TypeScriptLinter.isReflectAPICall(node)) {
      this.incrementCounters(node.parent, FaultID.InteropCallReflect);
    }
  }

  static isReflectAPICall(callExpr: ts.CallExpression): boolean {
    if (ts.isPropertyAccessExpression(callExpr.expression)) {
      const expr = callExpr.expression.expression;
      if (ts.isIdentifier(expr) && expr.text === REFLECT_LITERAL) {
        return true;
      }
    }

    if (ts.isElementAccessExpression(callExpr.expression)) {
      const expr = callExpr.expression.expression;
      if (ts.isIdentifier(expr) && expr.text === REFLECT_LITERAL) {
        return true;
      }
    }
    return false;
  }

  private shouldCheckForForbiddenAPI(declaration: ts.SignatureDeclaration | ts.JSDocSignature): boolean {
    for (const parameter of declaration.parameters) {
      if (ts.isJSDocParameterTag(parameter)) {
        continue;
      }
      const parameterType = this.tsTypeChecker.getTypeAtLocation(parameter);
      const parameterTypeString = this.tsTypeChecker.typeToString(parameterType);

      if (parameterTypeString === OBJECT_LITERAL) {
        return true;
      }
    }

    return false;
  }

  private checkForForbiddenAPIs(callSignature: ts.Signature, tsCallExpr: ts.CallExpression): void {
    if (!callSignature.declaration) {
      return;
    }

    if (!this.shouldCheckForForbiddenAPI(callSignature.declaration)) {
      return;
    }

    const functionSymbol = this.getFunctionSymbol(callSignature.declaration);
    const functionDeclaration = functionSymbol?.valueDeclaration;
    if (!functionDeclaration) {
      return;
    }

    if (!TypeScriptLinter.isFunctionLike(functionDeclaration)) {
      return;
    }

    switch (TypeScriptLinter.containsForbiddenAPI(functionDeclaration)) {
      case REFLECT_LITERAL:
        this.incrementCounters(tsCallExpr.parent, FaultID.InteropCallReflect);
        break;
      case OBJECT_LITERAL:
        this.incrementCounters(tsCallExpr.parent, FaultID.InteropCallObjectParam);
        break;
      default:
        break;
    }
  }

  private handleEtsComponentExpression(node: ts.Node): void {
    // for all the checks we make EtsComponentExpression is compatible with the CallExpression
    const etsComponentExpression = node as ts.CallExpression;
    this.handleLibraryTypeCall(etsComponentExpression);
  }

  private handleImportCall(tsCallExpr: ts.CallExpression): void {
    if (tsCallExpr.expression.kind !== ts.SyntaxKind.ImportKeyword) {
      return;
    } else if (this.options.arkts2) {
      this.incrementCounters(tsCallExpr, FaultID.DynamicImport);
    }

    // relax rule#133 "arkts-no-runtime-import"
    const tsArgs = tsCallExpr.arguments;
    if (tsArgs.length <= 1 || !ts.isObjectLiteralExpression(tsArgs[1])) {
      return;
    }

    for (const tsProp of tsArgs[1].properties) {
      if (
        (ts.isPropertyAssignment(tsProp) || ts.isShorthandPropertyAssignment(tsProp)) &&
        tsProp.name.getText() === 'assert'
      ) {
        this.incrementCounters(tsProp, FaultID.ImportAssertion);
        break;
      }
    }
  }

  private handleRequireCall(tsCallExpr: ts.CallExpression): void {
    if (
      ts.isIdentifier(tsCallExpr.expression) &&
      tsCallExpr.expression.text === 'require' &&
      ts.isVariableDeclaration(tsCallExpr.parent)
    ) {
      const tsType = this.tsTypeChecker.getTypeAtLocation(tsCallExpr.expression);
      if (TsUtils.isInterfaceType(tsType) && tsType.symbol.name === 'NodeRequire') {
        this.incrementCounters(tsCallExpr.parent, FaultID.ImportAssignment);
      }
    }
  }

  private handleGenericCallWithNoTypeArgs(
    callLikeExpr: ts.CallExpression | ts.NewExpression,
    callSignature: ts.Signature
  ): void {

    /*
     * Note: The PR!716 has led to a significant performance degradation.
     * Since initial problem was fixed in a more general way, this change
     * became redundant. Therefore, it was reverted. See #13721 comments
     * for a detailed analysis.
     */
    if (this.options.arkts2 && TypeScriptLinter.isInvalidBuiltinGenericConstructorCall(callLikeExpr)) {
      const autofix = this.autofixer?.fixGenericCallNoTypeArgs(callLikeExpr as ts.NewExpression);
      this.incrementCounters(callLikeExpr, FaultID.GenericCallNoTypeArgs, autofix);
      return;
    }
    this.checkTypeArgumentsForGenericCallWithNoTypeArgs(callLikeExpr, callSignature);
  }

  private static isInvalidBuiltinGenericConstructorCall(newExpression: ts.CallExpression | ts.NewExpression): boolean {
    if (!ts.isNewExpression(newExpression)) {
      return false;
    }
    const isBuiltin = BUILTIN_GENERIC_CONSTRUCTORS.has(newExpression.expression.getText().replace(/Constructor$/, ''));
    return isBuiltin && (!newExpression.typeArguments || newExpression.typeArguments.length === 0);
  }

  private checkTypeArgumentsForGenericCallWithNoTypeArgs(
    callLikeExpr: ts.CallExpression | ts.NewExpression,
    callSignature: ts.Signature
  ): void {
    if (ts.isNewExpression(callLikeExpr) && this.isNonGenericClass(callLikeExpr)) {
      return;
    }
    const tsSyntaxKind = ts.isNewExpression(callLikeExpr) ?
      ts.SyntaxKind.Constructor :
      ts.SyntaxKind.FunctionDeclaration;
    const signFlags = ts.NodeBuilderFlags.WriteTypeArgumentsOfSignature | ts.NodeBuilderFlags.IgnoreErrors;
    const signDecl = this.tsTypeChecker.signatureToSignatureDeclaration(
      callSignature,
      tsSyntaxKind,
      undefined,
      signFlags
    );
    if (!signDecl?.typeArguments) {
      return;
    }
    const resolvedTypeArgs = signDecl.typeArguments;
    const providedTypeArgs = callLikeExpr.typeArguments;
    const startTypeArg = providedTypeArgs?.length ?? 0;
    let shouldReportError = startTypeArg !== resolvedTypeArgs.length;
    if (this.options.arkts2 && callLikeExpr.kind === ts.SyntaxKind.NewExpression) {
      shouldReportError = this.shouldReportGenericTypeArgsError(
        callLikeExpr,
        resolvedTypeArgs,
        providedTypeArgs,
        startTypeArg,
        shouldReportError
      );
      if (shouldReportError) {
        const autofix = this.autofixer?.fixGenericCallNoTypeArgs(callLikeExpr);
        this.incrementCounters(callLikeExpr, FaultID.GenericCallNoTypeArgs, autofix);
      }
    } else {
      this.checkForUnknownTypeInNonArkTS2(callLikeExpr, resolvedTypeArgs, startTypeArg);
    }
  }

  private checkForUnknownTypeInNonArkTS2(
    callLikeExpr: ts.CallExpression | ts.NewExpression,
    resolvedTypeArgs: ts.NodeArray<ts.TypeNode>,
    startTypeArg: number
  ): void {
    for (let i = startTypeArg; i < resolvedTypeArgs.length; ++i) {
      const typeNode = resolvedTypeArgs[i];

      /*
       * if compiler infers 'unknown' type there are 2 possible cases:
       *   1. Compiler unable to infer type from arguments and use 'unknown'
       *   2. Compiler infer 'unknown' from arguments
       * We report error in both cases. It is ok because we cannot use 'unknown'
       * in ArkTS and already have separate check for it.
       */
      if (typeNode.kind === ts.SyntaxKind.UnknownKeyword) {
        this.incrementCounters(callLikeExpr, FaultID.GenericCallNoTypeArgs);
        break;
      }
    }
  }

  private shouldReportGenericTypeArgsError(
    callLikeExpr: ts.CallExpression | ts.NewExpression,
    resolvedTypeArgs: ts.NodeArray<ts.TypeNode>,
    providedTypeArgs: ts.NodeArray<ts.TypeNode> | undefined,
    startTypeArg: number,
    initialErrorState: boolean
  ): boolean {
    const typeParameters = this.getOriginalTypeParameters(callLikeExpr);
    if (!typeParameters || typeParameters.length === 0) {
      return initialErrorState;
    }
    const optionalParamsCount = typeParameters.filter((param, index) => {
      return param.default && (!providedTypeArgs || index >= providedTypeArgs.length);
    }).length;
    if (optionalParamsCount === 0) {
      return initialErrorState;
    }
    return startTypeArg + optionalParamsCount !== resolvedTypeArgs.length;
  }

  private getOriginalTypeParameters(
    callLikeExpr: ts.CallExpression | ts.NewExpression
  ): ts.TypeParameterDeclaration[] | undefined {
    const typeChecker = this.tsTypeChecker;
    const expressionType = typeChecker.getTypeAtLocation(callLikeExpr.expression);
    const declarations = expressionType.symbol?.declarations;
    if (!declarations || declarations.length === 0) {
      return undefined;
    }
    for (const decl of declarations) {
      if (ts.isFunctionDeclaration(decl) && decl.typeParameters) {
        return [...decl.typeParameters];
      }
      if (ts.isMethodDeclaration(decl) && decl.typeParameters) {
        return [...decl.typeParameters];
      }
      if (ts.isClassDeclaration(decl) && decl.typeParameters) {
        return [...decl.typeParameters];
      }
      if (ts.isInterfaceDeclaration(decl) && decl.typeParameters) {
        return [...decl.typeParameters];
      }
    }
    return undefined;
  }

  private isNonGenericClass(expression: ts.NewExpression): boolean {
    const declaration = this.tsUtils.getDeclarationNode(expression.expression);
    return !!declaration && ts.isClassDeclaration(declaration) && !declaration.typeParameters;
  }

  static isArrayFromCall(callLikeExpr: ts.CallExpression | ts.NewExpression): boolean {
    return (
      ts.isCallExpression(callLikeExpr) &&
      ts.isPropertyAccessExpression(callLikeExpr.expression) &&
      callLikeExpr.expression.name.text === STRINGLITERAL_FROM &&
      ts.isIdentifier(callLikeExpr.expression.expression) &&
      callLikeExpr.expression.expression.text === STRINGLITERAL_ARRAY
    );
  }

  private static readonly listFunctionApplyCallApis = [
    'Function.apply',
    'Function.call',
    'CallableFunction.apply',
    'CallableFunction.call'
  ];

  private static readonly listFunctionBindApis = ['Function.bind', 'CallableFunction.bind'];

  private handleFunctionApplyBindPropCall(tsCallExpr: ts.CallExpression, calleeSym: ts.Symbol): void {
    const exprName = this.tsTypeChecker.getFullyQualifiedName(calleeSym);
    if (TypeScriptLinter.listFunctionApplyCallApis.includes(exprName)) {
      this.incrementCounters(tsCallExpr, FaultID.FunctionApplyCall);
    }
    if (TypeScriptLinter.listFunctionBindApis.includes(exprName)) {
      const faultId = this.options.arkts2 ? FaultID.FunctionBindError : FaultID.FunctionBind;
      this.incrementCounters(tsCallExpr, faultId);
    }
  }

  private handleStructIdentAndUndefinedInArgs(
    tsCallOrNewExpr: ts.CallExpression | ts.NewExpression,
    callSignature: ts.Signature
  ): void {
    if (!tsCallOrNewExpr.arguments) {
      return;
    }
    for (let argIndex = 0; argIndex < tsCallOrNewExpr.arguments.length; ++argIndex) {
      const tsArg = tsCallOrNewExpr.arguments[argIndex];
      const tsArgType = this.tsTypeChecker.getTypeAtLocation(tsArg);
      if (!tsArgType) {
        continue;
      }
      const paramIndex = argIndex < callSignature.parameters.length ? argIndex : callSignature.parameters.length - 1;
      const tsParamSym = callSignature.parameters[paramIndex];
      if (!tsParamSym) {
        continue;
      }
      const tsParamDecl = tsParamSym.valueDeclaration;
      if (tsParamDecl && ts.isParameter(tsParamDecl)) {
        let tsParamType = this.tsTypeChecker.getTypeOfSymbolAtLocation(tsParamSym, tsParamDecl);
        if (tsParamDecl.dotDotDotToken && this.tsUtils.isGenericArrayType(tsParamType) && tsParamType.typeArguments) {
          tsParamType = tsParamType.typeArguments[0];
        }
        if (!tsParamType) {
          continue;
        }
        this.checkAssignmentMatching(tsArg, tsParamType, tsArg);
      }
    }
  }

  private static readonly LimitedApis = new Map<string, { arr: Array<string> | null; fault: FaultID }>([
    ['global', { arr: LIMITED_STD_GLOBAL_API, fault: FaultID.LimitedStdLibApi }],
    ['Object', { arr: LIMITED_STD_OBJECT_API, fault: FaultID.LimitedStdLibApi }],
    ['ObjectConstructor', { arr: LIMITED_STD_OBJECT_API, fault: FaultID.LimitedStdLibApi }],
    ['Reflect', { arr: LIMITED_STD_REFLECT_API, fault: FaultID.LimitedStdLibApi }],
    ['ProxyHandler', { arr: LIMITED_STD_PROXYHANDLER_API, fault: FaultID.LimitedStdLibApi }],
    [SYMBOL, { arr: null, fault: FaultID.SymbolType }],
    [SYMBOL_CONSTRUCTOR, { arr: null, fault: FaultID.SymbolType }]
  ]);

  private handleStdlibAPICall(
    callExpr: ts.CallExpression,
    calleeSym: ts.Symbol,
    name: string,
    parName: string | undefined
  ): void {
    if (parName === undefined) {
      if (LIMITED_STD_GLOBAL_API.includes(name)) {
        this.incrementCounters(callExpr, FaultID.LimitedStdLibApi);
        return;
      }
      const escapedName = calleeSym.escapedName;
      if (escapedName === 'Symbol' || escapedName === 'SymbolConstructor') {
        this.incrementCounters(callExpr, FaultID.SymbolType);
      }
      return;
    }
    const lookup = TypeScriptLinter.LimitedApis.get(parName);
    if (
      lookup !== undefined &&
      (lookup.arr === null || lookup.arr.includes(name)) &&
      (!this.options.useRelaxedRules || !this.supportedStdCallApiChecker.isSupportedStdCallAPI(callExpr, parName, name))
    ) {
      this.incrementCounters(callExpr, lookup.fault);
    }
  }

  private handleBuiltinThisArgs(
    callExpr: ts.CallExpression,
    calleeSym: ts.Symbol,
    name: string,
    parName: string | undefined
  ): void {
    if (parName === undefined) {
      return;
    }

    const builtinThisArgsInfos = TypeScriptLinter.funcMap.get(name);
    if (!builtinThisArgsInfos) {
      return;
    }

    const sourceFile = calleeSym?.declarations?.[0]?.getSourceFile();
    if (!sourceFile) {
      return;
    }

    const fileName = path.basename(sourceFile.fileName);
    const builtinInfos = builtinThisArgsInfos.get(fileName);
    if (!(builtinInfos && builtinInfos.size > 0)) {
      return;
    }
    for (const info of builtinInfos) {
      const needReport =
        info.parent_api.length > 0 &&
        info.parent_api[0].api_name === parName &&
        info?.api_func_args?.length === callExpr.arguments.length;
      if (needReport) {
        this.incrementCounters(callExpr, FaultID.BuiltinThisArgs);
        return;
      }
    }
  }

  private checkLimitedStdlibApi(node: ts.Identifier, symbol: ts.Symbol): void {
    const parName = this.tsUtils.getParentSymbolName(symbol);
    const entries = LIMITED_STD_API.get(parName);
    if (!entries) {
      return;
    }
    for (const entry of entries) {
      if (
        entry.api.includes(symbol.name) &&
        !this.supportedStdCallApiChecker.isSupportedStdCallAPI(node, parName, symbol.name)
      ) {
        this.incrementCounters(node, entry.faultId);
        return;
      }
    }
  }

  private handleLibraryTypeCall(expr: ts.CallExpression | ts.NewExpression): void {
    if (!expr.arguments || !this.tscStrictDiagnostics || !this.sourceFile) {
      return;
    }

    const file = path.normalize(this.sourceFile.fileName);
    const tscDiagnostics: readonly ts.Diagnostic[] | undefined = this.tscStrictDiagnostics.get(file);
    if (!tscDiagnostics?.length) {
      return;
    }

    const isOhModulesEts = TsUtils.isOhModulesEtsSymbol(this.tsUtils.trueSymbolAtLocation(expr.expression));
    const deleteDiagnostics: Set<ts.Diagnostic> = new Set();
    LibraryTypeCallDiagnosticChecker.instance.filterDiagnostics(
      tscDiagnostics,
      expr,
      this.tsUtils.isLibraryType(this.tsTypeChecker.getTypeAtLocation(expr.expression)),
      (diagnostic, errorType) => {

        /*
         * When a diagnostic meets the filter criteria, If it happens in an ets file in the 'oh_modules' directory.
         * the diagnostic is downgraded to warning. For other files, downgraded to nothing.
         */
        if (isOhModulesEts && errorType !== DiagnosticCheckerErrorType.UNKNOW) {
          diagnostic.category = ts.DiagnosticCategory.Warning;
          return false;
        }
        deleteDiagnostics.add(diagnostic);
        return true;
      }
    );

    if (!deleteDiagnostics.size) {
      return;
    }

    this.tscStrictDiagnostics.set(
      file,
      tscDiagnostics.filter((item) => {
        return !deleteDiagnostics.has(item);
      })
    );
  }

  private checkConstrutorAccess(propertyAccessExpr: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    if (propertyAccessExpr.name.text === 'constructor') {
      this.incrementCounters(propertyAccessExpr, FaultID.NoConstructorOnClass);
    }
  }

  private checkForInterfaceInitialization(newExpression: ts.NewExpression): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }
    const calleeExpr = newExpression.expression;
    if (!ts.isIdentifier(calleeExpr)) {
      return;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(calleeExpr);
    const typeDeclaration = TsUtils.getDeclaration(type.symbol);
    if (typeDeclaration && ts.isInterfaceDeclaration(typeDeclaration) && type.symbol) {
      const filePath = typeDeclaration.getSourceFile().fileName;
      this.checkIsConstructorIface(calleeExpr, type.symbol.name, path.basename(filePath));
    }
  }

  private checkIsConstructorIface(node: ts.Node, symbol: string, filePath: string): void {
    const constructorIfaceSetInfos = Array.from(TypeScriptLinter.ConstructorIfaceSet);
    constructorIfaceSetInfos.some((constructorFuncsInfo) => {
      const api_name = constructorFuncsInfo.api_info.parent_api[0].api_name;
      if (
        symbol === api_name &&
        (constructorFuncsInfo.file_path.includes(filePath) || constructorFuncsInfo.import_path.includes(filePath))
      ) {
        this.incrementCounters(node, FaultID.ConstructorIfaceFromSdk);
        return true;
      }
      return false;
    });
  }

  private handleNewExpression(node: ts.Node): void {
    const tsNewExpr = node as ts.NewExpression;

    this.checkForInterfaceInitialization(tsNewExpr);
    this.handleSharedArrayBuffer(tsNewExpr);
    this.handleSdkGlobalApi(tsNewExpr);
    this.checkCreatingPrimitiveTypes(tsNewExpr);

    if (this.options.advancedClassChecks || this.options.arkts2) {
      const calleeExpr = tsNewExpr.expression;
      const calleeType = this.tsTypeChecker.getTypeAtLocation(calleeExpr);
      if (
        !this.tsUtils.isClassTypeExpression(calleeExpr) &&
        !isStdLibraryType(calleeType) &&
        !this.tsUtils.isLibraryType(calleeType) &&
        !this.tsUtils.hasEsObjectType(calleeExpr)
      ) {
        // missing exact rule
        const faultId = this.options.arkts2 ? FaultID.DynamicCtorCall : FaultID.ClassAsObject;
        this.incrementCounters(calleeExpr, faultId);
      }
    }
    const sym = this.tsUtils.trueSymbolAtLocation(tsNewExpr.expression);
    const callSignature = this.tsTypeChecker.getResolvedSignature(tsNewExpr);
    if (callSignature !== undefined) {
      if (!this.tsUtils.isLibrarySymbol(sym)) {
        this.handleStructIdentAndUndefinedInArgs(tsNewExpr, callSignature);
        this.handleGenericCallWithNoTypeArgs(tsNewExpr, callSignature);
      } else if (this.options.arkts2) {
        this.handleGenericCallWithNoTypeArgs(tsNewExpr, callSignature);
      }
    }
    this.handleSendableGenericTypes(tsNewExpr);
    this.handleInstantiatedJsObject(tsNewExpr, sym);
    this.handlePromiseNeedVoidResolve(tsNewExpr);
  }

  handlePromiseNeedVoidResolve(newExpr: ts.NewExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!ts.isIdentifier(newExpr.expression) || newExpr.expression.text !== 'Promise') {
      return;
    }

    const typeArg = newExpr.typeArguments?.[0];
    if (!typeArg) {
      return;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(typeArg);
    if (!(type.getFlags() & ts.TypeFlags.Void)) {
      return;
    }

    const executor = newExpr.arguments?.[0];
    if (!executor || !ts.isFunctionLike(executor)) {
      return;
    }

    const resolveParam = executor.parameters[0];
    if (resolveParam?.type) {
      if (ts.isFunctionTypeNode(resolveParam.type) &&
        resolveParam.type.parameters.length === 0) {
        this.incrementCounters(resolveParam.type,FaultID.PromiseVoidNeedResolveArg);
      }
    }
    if (executor.body) {
      ts.forEachChild(executor.body, node => {
        if (ts.isCallExpression(node) &&
          ts.isIdentifier(node.expression) &&
          node.expression.text === 'resolve' &&
          node.arguments.length === 0) {
          this.incrementCounters(node,FaultID.PromiseVoidNeedResolveArg);
        }
      });
    }
  }

  private checkCreatingPrimitiveTypes(tsNewExpr: ts.NewExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    const typeStr = this.tsTypeChecker.typeToString(this.tsTypeChecker.getTypeAtLocation(tsNewExpr));
    const primitiveTypes = ['Number', 'String', 'Boolean'];
    if (primitiveTypes.includes(typeStr)) {
      this.incrementCounters(tsNewExpr, FaultID.CreatingPrimitiveTypes);
    }
  }

  handleInstantiatedJsObject(tsNewExpr: ts.NewExpression, sym: ts.Symbol | undefined): void {
    if (this.useStatic && this.options.arkts2) {
      if (sym?.declarations?.[0]?.getSourceFile().fileName.endsWith(EXTNAME_JS)) {
        const args = tsNewExpr.arguments;
        const autofix = this.autofixer?.fixInteropInstantiateExpression(tsNewExpr, args);
        this.incrementCounters(tsNewExpr, FaultID.InstantiatedJsOjbect, autofix);
      }
    }
  }

  private handleSendableGenericTypes(node: ts.NewExpression): void {
    const type = this.tsTypeChecker.getTypeAtLocation(node);
    if (!this.tsUtils.isSendableClassOrInterface(type)) {
      return;
    }

    const typeArgs = node.typeArguments;
    if (!typeArgs || typeArgs.length === 0) {
      return;
    }

    for (const arg of typeArgs) {
      if (!this.tsUtils.isSendableTypeNode(arg)) {
        this.incrementCounters(arg, FaultID.SendableGenericTypes);
      }
    }
  }

  private handleAsExpression(node: ts.Node): void {
    const tsAsExpr = node as ts.AsExpression;
    if (tsAsExpr.type.getText() === 'const') {
      this.incrementCounters(node, FaultID.ConstAssertion);
    }
    const targetType = this.tsTypeChecker.getTypeAtLocation(tsAsExpr.type).getNonNullableType();
    const exprType = this.tsTypeChecker.getTypeAtLocation(tsAsExpr.expression).getNonNullableType();
    // check for rule#65:   'number as Number' and 'boolean as Boolean' are disabled
    if (
      this.tsUtils.isNumberLikeType(exprType) && this.tsUtils.isStdNumberType(targetType) ||
      TsUtils.isBooleanLikeType(exprType) && this.tsUtils.isStdBooleanType(targetType)
    ) {
      this.incrementCounters(node, FaultID.TypeAssertion);
    }
    if (
      !this.tsUtils.isSendableClassOrInterface(exprType) &&
      !this.tsUtils.isObject(exprType) &&
      !TsUtils.isAnyType(exprType) &&
      this.tsUtils.isSendableClassOrInterface(targetType)
    ) {
      this.incrementCounters(tsAsExpr, FaultID.SendableAsExpr);
    }
    if (this.tsUtils.isWrongSendableFunctionAssignment(targetType, exprType)) {
      this.incrementCounters(tsAsExpr, FaultID.SendableFunctionAsExpr);
    }
    if (
      this.options.arkts2 &&
      this.tsUtils.needToDeduceStructuralIdentity(targetType, exprType, tsAsExpr.expression, true)
    ) {
      if (this.isExemptedAsExpression(tsAsExpr)) {
        return;
      }
      if (!this.tsUtils.isObject(exprType)) {
        this.incrementCounters(node, FaultID.StructuralIdentity);
      }
    }
    this.handleAsExpressionImport(tsAsExpr);
    this.handleNoTuplesArrays(node, targetType, exprType);
    this.handleObjectLiteralAssignmentToClass(tsAsExpr);
    this.handleArrayTypeImmutable(tsAsExpr, exprType, targetType);
    this.handleNotsLikeSmartTypeOnAsExpression(tsAsExpr);
  }

  private isExemptedAsExpression(node: ts.AsExpression): boolean {
    if (!ts.isElementAccessExpression(node.expression)) {
      return false;
    }

    const sourceType = this.tsTypeChecker.getTypeAtLocation(node.expression);
    const targetType = this.tsTypeChecker.getTypeAtLocation(node.type);
    const isRecordIndexAccess = (): boolean => {
      const exprType = this.tsTypeChecker.getTypeAtLocation(node.expression);
      const hasNumberIndex = !!exprType.getNumberIndexType();
      const hasStringIndex = !!exprType.getStringIndexType();
      const hasBooleanIndex = !!exprType.getProperty('true') || !!exprType.getProperty('false');

      return hasNumberIndex || hasStringIndex || hasBooleanIndex;
    };

    if (isRecordIndexAccess()) {
      const targetSymbol = targetType.getSymbol();
      if (targetSymbol && targetSymbol.getName() === 'Array') {
        return true;
      }
    }
    const primitiveFlags = ts.TypeFlags.Number | ts.TypeFlags.String | ts.TypeFlags.Boolean;
    const objectFlag = ts.TypeFlags.Object;
    return (
      sourceType.isUnion() &&
      sourceType.types.some((t) => {
        return t.flags & primitiveFlags;
      }) &&
      sourceType.types.some((t) => {
        return t.flags & objectFlag;
      })
    );
  }

  private handleAsExpressionImport(tsAsExpr: ts.AsExpression): void {
    if (!this.useStatic || !this.options.arkts2) {
      return;
    }

    const type = tsAsExpr.type;
    const expression = tsAsExpr.expression;
    const restrictedPrimitiveTypes = [
      ts.SyntaxKind.NumberKeyword,
      ts.SyntaxKind.BooleanKeyword,
      ts.SyntaxKind.StringKeyword,
      ts.SyntaxKind.BigIntKeyword,
      ts.SyntaxKind.UndefinedKeyword
    ];
    this.handleAsExpressionImportNull(tsAsExpr);
    const isRestrictedPrimitive = restrictedPrimitiveTypes.includes(type.kind);
    const isRestrictedArrayType =
      type.kind === ts.SyntaxKind.ArrayType ||
      ts.isTypeReferenceNode(type) && ts.isIdentifier(type.typeName) && type.typeName.text === 'Array';

    if (!isRestrictedPrimitive && !isRestrictedArrayType) {
      return;
    }

    let identifier: ts.Identifier | undefined;
    if (ts.isIdentifier(expression)) {
      identifier = expression;
    } else if (ts.isPropertyAccessExpression(expression)) {
      identifier = ts.isIdentifier(expression.expression) ? expression.expression : undefined;
    }

    if (identifier) {
      const sym = this.tsUtils.trueSymbolAtLocation(identifier);
      const decl = TsUtils.getDeclaration(sym);
      if (decl?.getSourceFile().fileName.endsWith(EXTNAME_JS)) {
        const autofix = this.autofixer?.fixInteropAsExpression(tsAsExpr);
        this.incrementCounters(tsAsExpr, FaultID.InterOpConvertImport, autofix);
      }
    }
  }

  private handleAsExpressionImportNull(tsAsExpr: ts.AsExpression): void {
    const type = tsAsExpr.type;
    const isNullAssertion =
      type.kind === ts.SyntaxKind.NullKeyword ||
      ts.isLiteralTypeNode(type) && type.literal.kind === ts.SyntaxKind.NullKeyword ||
      type.getText() === 'null';
    if (isNullAssertion) {
      this.incrementCounters(tsAsExpr, FaultID.InterOpConvertImport);
    }
  }

  private handleSdkConstructorIface(typeRef: ts.TypeReferenceNode): void {
    if (!this.options.arkts2 && typeRef?.typeName === undefined && !ts.isQualifiedName(typeRef.typeName)) {
      return;
    }
    const qualifiedName = typeRef.typeName as ts.QualifiedName;
    // tsc version diff
    const topName = qualifiedName.left?.getText();
    const sdkInfos = this.interfaceMap.get(topName);
    if (!sdkInfos) {
      return;
    }
    for (const sdkInfo of sdkInfos) {
      if (sdkInfo.api_type !== 'ConstructSignature') {
        continue;
      }
      // sdk api from json has 3 overload. need consider these case.
      if (sdkInfo.parent_api[0].api_name === qualifiedName.right.getText()) {
        this.incrementCounters(typeRef, FaultID.ConstructorIfaceFromSdk);
        break;
      }
    }
  }

  private handleSdkConstructorIfaceForCallExpression(callExpr: ts.CallExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    let type: ts.Type | undefined;
    if (!callExpr.arguments || callExpr.arguments.length === 0) {
      if (ts.isPropertyAccessExpression(callExpr.expression)) {
        type = this.tsTypeChecker.getTypeAtLocation(callExpr.expression.expression);
      }
    }
    callExpr.arguments.some((args) => {
      if (ts.isIdentifier(args)) {
        type = this.tsTypeChecker.getTypeAtLocation(args);
      }
    });
    if (!type) {
      return;
    }
    const decl = TsUtils.getDeclaration(type?.symbol);
    if (!decl) {
      return;
    }
    const filePath = TypeScriptLinter.getFileName(decl);
    this.checkIsConstructorIface(callExpr, type.symbol.name, filePath);
  }

  private static getFileName(decl: ts.Declaration): string {
    let filePath = '';
    if (
      ts.isImportSpecifier(decl) &&
      ts.isImportDeclaration(decl.parent.parent.parent) &&
      ts.isStringLiteral(decl.parent.parent.parent.moduleSpecifier)
    ) {
      filePath = decl.parent.parent.parent.moduleSpecifier.text;
    } else if (
      ts.isImportClause(decl) &&
      ts.isImportDeclaration(decl.parent) &&
      ts.isStringLiteral(decl.parent.moduleSpecifier)
    ) {
      filePath = decl.parent.moduleSpecifier.text;
    } else {
      filePath = decl.getSourceFile().fileName;
    }
    return path.basename(filePath);
  }

  private handleSharedArrayBuffer(
    node: ts.TypeReferenceNode | ts.NewExpression | ts.ExpressionWithTypeArguments
  ): void {
    if (!this.options.arkts2) {
      return;
    }

    const typeNameIdentifier = ts.isTypeReferenceNode(node) ? node.typeName : node.expression;
    if (!ts.isIdentifier(typeNameIdentifier) || typeNameIdentifier.getText() !== ESLIB_SHAREDARRAYBUFFER) {
      return;
    }
    const symbol = this.tsUtils.trueSymbolAtLocation(typeNameIdentifier);
    if (!symbol) {
      return;
    }

    const isImported = this.sourceFile.statements.some((stmt) => {
      if (!ts.isImportDeclaration(stmt)) {
        return false;
      }
      const importClause = stmt.importClause;
      if (!importClause?.namedBindings || !ts.isNamedImports(importClause.namedBindings)) {
        return false;
      }

      const elements = importClause.namedBindings.elements.some((element) => {
        return element.name.text === ESLIB_SHAREDARRAYBUFFER;
      });
      return elements;
    });
    if (isImported) {
      return;
    }
    const decls = symbol.getDeclarations();
    const isSharedMemoryEsLib = decls?.some((decl) => {
      const srcFileName = decl.getSourceFile().fileName;
      return srcFileName.endsWith(ESLIB_SHAREDMEMORY_FILENAME);
    });

    if (!isSharedMemoryEsLib || this.hasLocalSharedArrayBufferClass()) {
      return;
    }

    const autofix = this.autofixer?.replaceNode(typeNameIdentifier, 'ArrayBuffer');
    this.incrementCounters(typeNameIdentifier, FaultID.SharedArrayBufferDeprecated, autofix);
  }

  private hasLocalSharedArrayBufferClass(): boolean {
    return this.sourceFile.statements.some((stmt) => {
      return ts.isClassDeclaration(stmt) && stmt.name?.text === ESLIB_SHAREDARRAYBUFFER;
    });
  }

  private handleTypeReference(node: ts.Node): void {
    const typeRef = node as ts.TypeReferenceNode;

    this.handleBuiltinCtorCallSignature(typeRef);
    this.handleSharedArrayBuffer(typeRef);
    this.handleSdkGlobalApi(typeRef);

    this.handleSdkConstructorIface(typeRef);

    const isESValue = TsUtils.isEsValueType(typeRef);
    const isPossiblyValidContext = TsUtils.isEsValuePossiblyAllowed(typeRef);
    if (isESValue && !isPossiblyValidContext) {
      const faultId = this.options.arkts2 ? FaultID.EsValueTypeError : FaultID.EsValueType;
      this.incrementCounters(node, faultId);
      return;
    }

    const typeName = this.tsUtils.entityNameToString(typeRef.typeName);
    const isStdUtilityType = LIMITED_STANDARD_UTILITY_TYPES.includes(typeName);
    if (isStdUtilityType) {
      this.incrementCounters(node, FaultID.UtilityType);
      return;
    }

    this.checkPartialType(node);

    const typeNameType = this.tsTypeChecker.getTypeAtLocation(typeRef.typeName);
    if (this.options.arkts2 && (typeNameType.flags & ts.TypeFlags.Void) !== 0) {
      this.incrementCounters(typeRef, FaultID.LimitedVoidType);
    }
    if (this.tsUtils.isSendableClassOrInterface(typeNameType)) {
      this.checkSendableTypeArguments(typeRef);
    }

    this.checkNoEnumProp(typeRef);
    if (ts.isQualifiedName(typeRef.typeName)) {
      this.handleSdkForConstructorFuncs(typeRef.typeName);
    }
  }

  private checkNoEnumProp(typeRef: ts.TypeReferenceNode): void {
    if (!this.options.arkts2) {
      return;
    }
    if (ts.isQualifiedName(typeRef.typeName)) {
      const symbol = this.tsTypeChecker.getSymbolAtLocation(typeRef.typeName.right);

      if (!symbol) {
        return;
      }

      const declarations = symbol.getDeclarations();
      if (!declarations || declarations.length === 0) {
        return;
      }

      if (ts.isEnumMember(declarations[0])) {
        this.incrementCounters(typeRef, FaultID.NoEnumPropAsType);
      }
    }
  }

  private checkPartialType(node: ts.Node): void {
    const typeRef = node as ts.TypeReferenceNode;
    // Using Partial<T> type is allowed only when its argument type is either Class or Interface.
    const isStdPartial = this.tsUtils.entityNameToString(typeRef.typeName) === 'Partial';
    if (!isStdPartial) {
      return;
    }

    const hasSingleTypeArgument = !!typeRef.typeArguments && typeRef.typeArguments.length === 1;
    let argType;
    if (!this.options.useRtLogic) {
      const firstTypeArg = !!typeRef.typeArguments && hasSingleTypeArgument && typeRef.typeArguments[0];
      argType = firstTypeArg && this.tsTypeChecker.getTypeFromTypeNode(firstTypeArg);
    } else {
      argType = hasSingleTypeArgument && this.tsTypeChecker.getTypeFromTypeNode(typeRef.typeArguments[0]);
    }

    if (argType && !argType.isClassOrInterface()) {
      this.incrementCounters(node, FaultID.UtilityType);
    }
  }

  private checkSendableTypeArguments(typeRef: ts.TypeReferenceNode): void {
    if (typeRef.typeArguments) {
      for (const typeArg of typeRef.typeArguments) {
        if (!this.tsUtils.isSendableTypeNode(typeArg)) {
          this.incrementCounters(typeArg, FaultID.SendableGenericTypes);
        }
      }
    }
  }

  private handleMetaProperty(node: ts.Node): void {
    const tsMetaProperty = node as ts.MetaProperty;
    if (tsMetaProperty.name.text === 'target') {
      this.incrementCounters(node, FaultID.NewTarget);
    }
  }

  private handleSpreadOp(node: ts.Node): void {

    /*
     * spread assignment is disabled
     * spread element is allowed only for arrays as rest parameter
     */
    if (ts.isSpreadElement(node)) {
      const spreadExprType = this.tsUtils.getTypeOrTypeConstraintAtLocation(node.expression);
      if (
        spreadExprType &&
        (this.options.useRtLogic || ts.isCallLikeExpression(node.parent) || ts.isArrayLiteralExpression(node.parent)) &&
        (this.tsUtils.isOrDerivedFrom(spreadExprType, this.tsUtils.isArray) ||
          this.tsUtils.isOrDerivedFrom(spreadExprType, this.tsUtils.isCollectionArrayType))
      ) {
        return;
      }
    }
    this.incrementCounters(node, FaultID.SpreadOperator);
  }

  private handleConstructSignature(node: ts.Node): void {
    switch (node.parent.kind) {
      case ts.SyntaxKind.TypeLiteral:
        this.incrementCounters(node, FaultID.ConstructorType);
        break;
      case ts.SyntaxKind.InterfaceDeclaration:
        this.incrementCounters(node, FaultID.ConstructorIface);
        break;
      default:
    }
  }

  private handleExpressionWithTypeArguments(node: ts.Node): void {
    const tsTypeExpr = node as ts.ExpressionWithTypeArguments;
    const symbol = this.tsUtils.trueSymbolAtLocation(tsTypeExpr.expression);

    if (!!symbol && TsUtils.isEsObjectSymbol(symbol)) {
      const faultId = this.options.arkts2 ? FaultID.EsValueTypeError : FaultID.EsValueType;
      this.incrementCounters(tsTypeExpr, faultId);
    }
    this.handleSdkGlobalApi(tsTypeExpr);
  }

  private handleComputedPropertyName(node: ts.Node): void {
    const computedProperty = node as ts.ComputedPropertyName;
    if (this.isSendableCompPropName(computedProperty)) {
      // cancel the '[Symbol.iterface]' restriction of 'sendable class/interface' in the 'collections.d.ts' file
      if (this.tsUtils.isSymbolIteratorExpression(computedProperty.expression)) {
        const declNode = computedProperty.parent?.parent;
        if (declNode && TsUtils.isArkTSCollectionsClassOrInterfaceDeclaration(declNode)) {
          return;
        }
      }
      this.incrementCounters(node, FaultID.SendableComputedPropName);
    } else if (!this.tsUtils.isValidComputedPropertyName(computedProperty, false)) {
      this.incrementCounters(node, FaultID.ComputedPropertyName);
    }
  }

  private isSendableCompPropName(compProp: ts.ComputedPropertyName): boolean {
    const declNode = compProp.parent?.parent;
    if (declNode && ts.isClassDeclaration(declNode) && TsUtils.hasSendableDecorator(declNode)) {
      return true;
    } else if (declNode && ts.isInterfaceDeclaration(declNode)) {
      const declNodeType = this.tsTypeChecker.getTypeAtLocation(declNode);
      if (this.tsUtils.isSendableClassOrInterface(declNodeType)) {
        return true;
      }
    }
    return false;
  }

  private handleGetAccessor(node: ts.GetAccessorDeclaration): void {
    TsUtils.getDecoratorsIfInSendableClass(node)?.forEach((decorator) => {
      this.incrementCounters(decorator, FaultID.SendableClassDecorator);
    });
  }

  private handleSetAccessor(node: ts.SetAccessorDeclaration): void {
    TsUtils.getDecoratorsIfInSendableClass(node)?.forEach((decorator) => {
      this.incrementCounters(decorator, FaultID.SendableClassDecorator);
    });
  }

  /*
   * issue 13987:
   * When variable have no type annotation and no initial value, and 'noImplicitAny'
   * option is enabled, compiler attempts to infer type from variable references:
   * see https://github.com/microsoft/TypeScript/pull/11263.
   * In this case, we still want to report the error, since ArkTS doesn't allow
   * to omit both type annotation and initializer.
   */
  private proceedVarPropDeclaration(
    decl: ts.VariableDeclaration | ts.PropertyDeclaration | ts.ParameterDeclaration
  ): boolean | undefined {
    if (
      (ts.isVariableDeclaration(decl) && ts.isVariableStatement(decl.parent.parent) ||
        ts.isPropertyDeclaration(decl)) &&
      !decl.initializer
    ) {
      if (
        ts.isPropertyDeclaration(decl) &&
        this.tsUtils.skipPropertyInferredTypeCheck(decl, this.sourceFile, this.options.isEtsFileCb)
      ) {
        return true;
      }

      this.incrementCounters(decl, FaultID.AnyType);
      return true;
    }
    return undefined;
  }

  private handleDeclarationInferredType(
    decl: ts.VariableDeclaration | ts.PropertyDeclaration | ts.ParameterDeclaration
  ): void {
    // The type is explicitly specified, no need to check inferred type.
    if (decl.type) {
      return;
    }

    /*
     * issue 13161:
     * In TypeScript, the catch clause variable must be 'any' or 'unknown' type. Since
     * ArkTS doesn't support these types, the type for such variable is simply omitted,
     * and we don't report it as an error. See TypeScriptLinter.handleCatchClause()
     * for reference.
     */
    if (ts.isCatchClause(decl.parent)) {
      return;
    }
    // Destructuring declarations are not supported, do not process them.
    if (ts.isArrayBindingPattern(decl.name) || ts.isObjectBindingPattern(decl.name)) {
      return;
    }

    if (this.proceedVarPropDeclaration(decl)) {
      return;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(decl);
    if (type) {
      this.validateDeclInferredType(type, decl);
    }
  }

  private handleDefiniteAssignmentAssertion(decl: ts.VariableDeclaration | ts.PropertyDeclaration): void {
    if (decl.exclamationToken === undefined) {
      return;
    }

    if (decl.kind === ts.SyntaxKind.PropertyDeclaration) {
      const parentDecl = decl.parent;
      if (parentDecl.kind === ts.SyntaxKind.ClassDeclaration && TsUtils.hasSendableDecorator(parentDecl)) {
        this.incrementCounters(decl, FaultID.SendableDefiniteAssignment);
        return;
      }
    }
    const faultId = this.options.arkts2 ? FaultID.DefiniteAssignmentError : FaultID.DefiniteAssignment;
    this.incrementCounters(decl, faultId);
  }

  private readonly validatedTypesSet = new Set<ts.Type>();

  private checkAnyOrUnknownChildNode(node: ts.Node): boolean {
    if (node.kind === ts.SyntaxKind.AnyKeyword || node.kind === ts.SyntaxKind.UnknownKeyword) {
      return true;
    }
    for (const child of node.getChildren()) {
      if (this.checkAnyOrUnknownChildNode(child)) {
        return true;
      }
    }
    return false;
  }

  private handleInferredObjectreference(
    type: ts.Type,
    decl: ts.VariableDeclaration | ts.PropertyDeclaration | ts.ParameterDeclaration
  ): void {
    const typeArgs = this.tsTypeChecker.getTypeArguments(type as ts.TypeReference);
    if (typeArgs) {
      const haveAnyOrUnknownNodes = this.checkAnyOrUnknownChildNode(decl);
      if (!haveAnyOrUnknownNodes) {
        for (const typeArg of typeArgs) {
          this.validateDeclInferredType(typeArg, decl);
        }
      }
    }
  }

  private validateDeclInferredType(
    type: ts.Type,
    decl: ts.VariableDeclaration | ts.PropertyDeclaration | ts.ParameterDeclaration
  ): void {
    if (type.aliasSymbol !== undefined) {
      return;
    }
    if (TsUtils.isObjectType(type) && !!(type.objectFlags & ts.ObjectFlags.Reference)) {
      this.handleInferredObjectreference(type, decl);
      return;
    }
    if (this.validatedTypesSet.has(type)) {
      return;
    }
    if (type.isUnion()) {
      this.validatedTypesSet.add(type);
      for (const unionElem of type.types) {
        this.validateDeclInferredType(unionElem, decl);
      }
    }
    if (TsUtils.isAnyType(type)) {
      this.incrementCounters(decl, FaultID.AnyType);
    } else if (TsUtils.isUnknownType(type)) {
      this.incrementCounters(decl, FaultID.UnknownType);
    }
  }

  private handleCommentDirectives(sourceFile: ts.SourceFile): void {

    /*
     * We use a dirty hack to retrieve list of parsed comment directives by accessing
     * internal properties of SourceFile node.
     */
    /* CC-OFFNXT(no_explicit_any) std lib */
    // Handle comment directive '@ts-nocheck'
    const pragmas = (sourceFile as any).pragmas;
    if (pragmas && pragmas instanceof Map) {
      const noCheckPragma = pragmas.get('ts-nocheck');
      if (noCheckPragma) {

        /*
         * The value is either a single entry or an array of entries.
         * Wrap up single entry with array to simplify processing.
         */
        /* CC-OFFNXT(no_explicit_any) std lib */
        const noCheckEntries: any[] = Array.isArray(noCheckPragma) ? noCheckPragma : [noCheckPragma];
        for (const entry of noCheckEntries) {
          this.processNoCheckEntry(entry);
        }
      }
    }

    /* CC-OFFNXT(no_explicit_any) std lib */
    // Handle comment directives '@ts-ignore' and '@ts-expect-error'
    const commentDirectives = (sourceFile as any).commentDirectives;
    if (commentDirectives && Array.isArray(commentDirectives)) {
      for (const directive of commentDirectives) {
        if (directive.range?.pos === undefined || directive.range?.end === undefined) {
          continue;
        }

        const range = directive.range as ts.TextRange;
        const kind: ts.SyntaxKind =
          sourceFile.text.slice(range.pos, range.pos + 2) === '/*' ?
            ts.SyntaxKind.MultiLineCommentTrivia :
            ts.SyntaxKind.SingleLineCommentTrivia;
        const commentRange: ts.CommentRange = {
          pos: range.pos,
          end: range.end,
          kind
        };

        this.incrementCounters(commentRange, FaultID.ErrorSuppression);
      }
    }
  }

  /* CC-OFFNXT(no_explicit_any) std lib */
  private processNoCheckEntry(entry: any): void {
    if (entry.range?.kind === undefined || entry.range?.pos === undefined || entry.range?.end === undefined) {
      return;
    }

    this.incrementCounters(entry.range as ts.CommentRange, FaultID.ErrorSuppression);
  }

  private reportThisKeywordsInScope(scope: ts.Block | ts.Expression): void {
    const callback = (node: ts.Node): void => {
      if (node.kind === ts.SyntaxKind.ThisKeyword) {
        this.incrementCounters(node, FaultID.FunctionContainsThis);
      }
    };
    const stopCondition = (node: ts.Node): boolean => {
      const isClassLike = ts.isClassDeclaration(node) || ts.isClassExpression(node);
      const isFunctionLike = ts.isFunctionDeclaration(node) || ts.isFunctionExpression(node);
      const isModuleDecl = ts.isModuleDeclaration(node);
      return isClassLike || isFunctionLike || isModuleDecl;
    };
    forEachNodeInSubtree(scope, callback, stopCondition);
  }

  private handleConstructorDeclaration(node: ts.Node): void {
    const ctorDecl = node as ts.ConstructorDeclaration;
    this.checkDefaultParamBeforeRequired(ctorDecl);
    this.handleTSOverload(ctorDecl);
    const paramProperties = ctorDecl.parameters.filter((x) => {
      return this.tsUtils.hasAccessModifier(x);
    });
    if (paramProperties.length === 0) {
      return;
    }
    let paramTypes: ts.TypeNode[] | undefined;
    if (ctorDecl.body) {
      paramTypes = this.collectCtorParamTypes(ctorDecl);
    }
    const autofix = this.autofixer?.fixCtorParameterProperties(ctorDecl, paramTypes);
    for (const param of paramProperties) {
      this.incrementCounters(param, FaultID.ParameterProperties, autofix);
    }
  }

  private collectCtorParamTypes(ctorDecl: ts.ConstructorDeclaration): ts.TypeNode[] | undefined {
    const paramTypes: ts.TypeNode[] = [];

    for (const param of ctorDecl.parameters) {
      let paramTypeNode = param.type;
      if (!paramTypeNode) {
        const paramType = this.tsTypeChecker.getTypeAtLocation(param);
        paramTypeNode = this.tsTypeChecker.typeToTypeNode(paramType, param, ts.NodeBuilderFlags.None);
      }
      if (!paramTypeNode || !this.tsUtils.isSupportedType(paramTypeNode)) {
        return undefined;
      }
      paramTypes.push(paramTypeNode);
    }

    return paramTypes;
  }

  private handlePrivateIdentifier(node: ts.Node): void {
    const ident = node as ts.PrivateIdentifier;
    const autofix = this.autofixer?.fixPrivateIdentifier(ident);
    this.incrementCounters(node, FaultID.PrivateIdentifier, autofix);
  }

  private handleIndexSignature(node: ts.Node): void {
    if (!this.tsUtils.isAllowedIndexSignature(node as ts.IndexSignatureDeclaration)) {
      this.incrementCounters(node, FaultID.IndexMember);
    }
  }

  private handleTypeLiteral(node: ts.Node): void {
    const typeLiteral = node as ts.TypeLiteralNode;
    const autofix = this.autofixer?.fixTypeliteral(typeLiteral);
    this.incrementCounters(node, FaultID.ObjectTypeLiteral, autofix);
  }

  private scanCapturedVarsInSendableScope(startNode: ts.Node, scope: ts.Node, faultId: FaultID): void {
    const callback = (node: ts.Node): void => {
      // Namespace import will introduce closure in the es2abc compiler stage
      if (!ts.isIdentifier(node) || this.checkNamespaceImportVar(node)) {
        return;
      }

      // The "b" of "A.b" should not be checked since it's load from object "A"
      const parent: ts.Node = node.parent;
      if (ts.isPropertyAccessExpression(parent) && parent.name === node) {
        return;
      }
      // When overloading function, will misreport
      if (ts.isFunctionDeclaration(startNode) && startNode.name === node) {
        return;
      }

      this.checkLocalDecl(node, scope, faultId);
    };
    // Type nodes should not checked because no closure will be introduced
    const stopCondition = (node: ts.Node): boolean => {
      // already existed 'arkts-sendable-class-decoratos' error
      if (ts.isDecorator(node) && node.parent === startNode) {
        return true;
      }
      return ts.isTypeReferenceNode(node);
    };
    forEachNodeInSubtree(startNode, callback, stopCondition);
  }

  private checkLocalDecl(node: ts.Identifier, scope: ts.Node, faultId: FaultID): void {
    const trueSym = this.tsUtils.trueSymbolAtLocation(node);
    // Sendable decorator should be used in method of Sendable classes
    if (trueSym === undefined) {
      return;
    }

    // Const enum member will be replaced by the exact value of it, no closure will be introduced
    if (TsUtils.isConstEnum(trueSym)) {
      return;
    }

    const declarations = trueSym.getDeclarations();
    if (declarations?.length) {
      this.checkLocalDeclWithSendableClosure(node, scope, declarations[0], faultId);
    }
  }

  private checkLocalDeclWithSendableClosure(
    node: ts.Identifier,
    scope: ts.Node,
    decl: ts.Declaration,
    faultId: FaultID
  ): void {
    const declPosition = decl.getStart();
    if (
      decl.getSourceFile().fileName !== node.getSourceFile().fileName ||
      declPosition !== undefined && declPosition >= scope.getStart() && declPosition < scope.getEnd()
    ) {
      return;
    }

    if (this.isFileExportDecl(decl)) {
      return;
    }

    if (this.isTopSendableClosure(decl)) {
      return;
    }

    /**
     * The cases in condition will introduce closure if defined in the same file as the Sendable class. The following
     * cases are excluded because they are not allowed in ArkTS:
     * 1. ImportEqualDecalration
     * 2. BindingElement
     */
    if (
      ts.isVariableDeclaration(decl) ||
      ts.isFunctionDeclaration(decl) ||
      ts.isClassDeclaration(decl) ||
      ts.isInterfaceDeclaration(decl) ||
      ts.isEnumDeclaration(decl) ||
      ts.isModuleDeclaration(decl) ||
      ts.isParameter(decl)
    ) {
      this.incrementCounters(node, faultId);
    }
  }

  private isTopSendableClosure(decl: ts.Declaration): boolean {
    if (!ts.isSourceFile(decl.parent)) {
      return false;
    }
    if (
      ts.isClassDeclaration(decl) &&
      this.tsUtils.isSendableClassOrInterface(this.tsTypeChecker.getTypeAtLocation(decl))
    ) {
      return true;
    }
    if (ts.isFunctionDeclaration(decl) && TsUtils.hasSendableDecoratorFunctionOverload(decl)) {
      return true;
    }
    return false;
  }

  private checkNamespaceImportVar(node: ts.Node): boolean {
    // Namespace import cannot be determined by the true symbol
    const sym = this.tsTypeChecker.getSymbolAtLocation(node);
    const decls = sym?.getDeclarations();
    if (decls?.length) {
      if (ts.isNamespaceImport(decls[0])) {
        this.incrementCounters(node, FaultID.SendableCapturedVars);
        return true;
      }
    }
    return false;
  }

  private isFileExportDecl(decl: ts.Declaration): boolean {
    const sourceFile = decl.getSourceFile();
    if (!this.fileExportDeclCaches) {
      this.fileExportDeclCaches = this.tsUtils.searchFileExportDecl(sourceFile);
    }
    return this.fileExportDeclCaches.has(decl);
  }

  private handleExportKeyword(node: ts.Node): void {
    const parentNode = node.parent;
    if (!TypeScriptLinter.inSharedModule(node) || ts.isModuleBlock(parentNode.parent)) {
      return;
    }

    switch (parentNode.kind) {
      case ts.SyntaxKind.EnumDeclaration:
      case ts.SyntaxKind.InterfaceDeclaration:
      case ts.SyntaxKind.FunctionDeclaration:
      case ts.SyntaxKind.ClassDeclaration:
        if (!this.tsUtils.isShareableType(this.tsTypeChecker.getTypeAtLocation(parentNode))) {
          this.incrementCounters((parentNode as ts.NamedDeclaration).name ?? parentNode, FaultID.SharedModuleExports);
        }
        return;
      case ts.SyntaxKind.VariableStatement:
        for (const variableDeclaration of (parentNode as ts.VariableStatement).declarationList.declarations) {
          if (!this.tsUtils.isShareableEntity(variableDeclaration.name)) {
            this.incrementCounters(variableDeclaration.name, FaultID.SharedModuleExports);
          }
        }
        return;
      case ts.SyntaxKind.TypeAliasDeclaration:
        if (!this.tsUtils.isShareableEntity(parentNode)) {
          this.incrementCounters(parentNode, FaultID.SharedModuleExportsWarning);
        }
        return;
      default:
        this.incrementCounters(parentNode, FaultID.SharedModuleExports);
    }
  }

  private handleExportDeclaration(node: ts.Node): void {
    const exportDecl = node as ts.ExportDeclaration;

    if (this.isExportedEntityDeclaredInJs(exportDecl)) {
      this.incrementCounters(node, FaultID.InteropJsObjectExport);
      return;
    }

    if (!TypeScriptLinter.inSharedModule(node) || ts.isModuleBlock(node.parent)) {
      return;
    }

    if (exportDecl.exportClause === undefined) {
      this.incrementCounters(exportDecl, FaultID.SharedModuleNoWildcardExport);
      return;
    }

    if (ts.isNamespaceExport(exportDecl.exportClause)) {
      if (!this.tsUtils.isShareableType(this.tsTypeChecker.getTypeAtLocation(exportDecl.exportClause.name))) {
        this.incrementCounters(exportDecl.exportClause.name, FaultID.SharedModuleExports);
      }
      return;
    }

    for (const exportSpecifier of exportDecl.exportClause.elements) {
      if (!this.tsUtils.isShareableEntity(exportSpecifier.name)) {
        this.incrementCounters(exportSpecifier.name, FaultID.SharedModuleExports);
      }
    }
  }

  private handleReturnStatement(node: ts.Node): void {
    // The return value must match the return type of the 'function'
    const returnStat = node as ts.ReturnStatement;
    const expr = returnStat.expression;
    if (!expr) {
      return;
    }
    const lhsType = this.tsTypeChecker.getContextualType(expr);
    if (!lhsType) {
      return;
    }
    this.checkAssignmentMatching(node, lhsType, expr, true);
    this.handleObjectLiteralInReturn(returnStat);
    this.handleObjectLiteralAssignmentToClass(returnStat);
  }

  /**
   * 'arkts-no-structural-typing' check was missing in some scenarios,
   * in order not to cause incompatibility,
   * only need to strictly match the type of filling the check again
   */
  private checkAssignmentMatching(
    field: ts.Node,
    lhsType: ts.Type,
    rhsExpr: ts.Expression,
    isNewStructuralCheck: boolean = false
  ): void {
    const rhsType = this.tsTypeChecker.getTypeAtLocation(rhsExpr);
    this.handleNoTuplesArrays(field, lhsType, rhsType);
    this.handleArrayTypeImmutable(field, lhsType, rhsType, rhsExpr);
    // check that 'sendable typeAlias' is assigned correctly
    if (this.tsUtils.isWrongSendableFunctionAssignment(lhsType, rhsType)) {
      this.incrementCounters(field, FaultID.SendableFunctionAssignment);
    }
    const isStrict = this.tsUtils.needStrictMatchType(lhsType, rhsType);
    // 'isNewStructuralCheck' means that this assignment scenario was previously omitted, so only strict matches are checked now
    if (isNewStructuralCheck && !isStrict) {
      return;
    }
    if (this.tsUtils.needToDeduceStructuralIdentity(lhsType, rhsType, rhsExpr, isStrict)) {
      if (ts.isNewExpression(rhsExpr) && ts.isIdentifier(rhsExpr.expression) && rhsExpr.expression.text === 'Promise') {
        const isReturnStatement = ts.isReturnStatement(rhsExpr.parent);
        const enclosingFunction = ts.findAncestor(rhsExpr, ts.isFunctionLike);
        const isAsyncFunction =
          enclosingFunction &&
          (enclosingFunction.modifiers?.some((m) => {
            return m.kind === ts.SyntaxKind.AsyncKeyword;
          }) ||
            false);
        if (isReturnStatement && isAsyncFunction) {
          return;
        }
      }
      this.incrementCounters(field, FaultID.StructuralIdentity);
    }
  }

  private handleDecorator(node: ts.Node): void {
    this.handleExtendDecorator(node);
    this.handleEntryDecorator(node);
    this.handleProvideDecorator(node);
    this.handleLocalBuilderDecorator(node);

    const decorator: ts.Decorator = node as ts.Decorator;
    this.checkSendableAndConcurrentDecorator(decorator);
    this.handleStylesDecorator(decorator);
    if (TsUtils.getDecoratorName(decorator) === SENDABLE_DECORATOR) {
      const parent: ts.Node = decorator.parent;
      if (!parent || !SENDABLE_DECORATOR_NODES.includes(parent.kind)) {
        const autofix = this.autofixer?.removeNode(decorator);
        this.incrementCounters(decorator, FaultID.SendableDecoratorLimited, autofix);
      }
    }
    this.handleNotSupportCustomDecorators(decorator);
  }

  private handleProvideDecorator(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!ts.isDecorator(node)) {
      return;
    }

    if (ts.isCallExpression(node.expression) && ts.isIdentifier(node.expression.expression)) {
      if (node.expression.expression.text !== PROVIDE_DECORATOR_NAME || node.expression.arguments.length !== 1) {
        return;
      }
      const arg = node.expression.arguments[0];
      if (!ts.isStringLiteral(arg) && !ts.isObjectLiteralExpression(arg)) {
        return;
      }
      if (ts.isObjectLiteralExpression(arg)) {
        const properties = arg.properties;
        if (properties.length !== 1) {
          return;
        }
        const property = properties[0] as ts.PropertyAssignment;
        if (!ts.isIdentifier(property.name) || !ts.isStringLiteral(property.initializer)) {
          return;
        }
        if (property.name.escapedText !== PROVIDE_ALLOW_OVERRIDE_PROPERTY_NAME) {
          return;
        }
      }
      const autofix = this.autofixer?.fixProvideDecorator(node);
      this.incrementCounters(node.parent, FaultID.ProvideAnnotation, autofix);
    }
  }

  private isSendableDecoratorValid(decl: ts.FunctionDeclaration | ts.TypeAliasDeclaration): boolean {
    if (
      this.compatibleSdkVersion > SENDBALE_FUNCTION_START_VERSION ||
      this.compatibleSdkVersion === SENDBALE_FUNCTION_START_VERSION &&
        !SENDABLE_FUNCTION_UNSUPPORTED_STAGES_IN_API12.includes(this.compatibleSdkVersionStage)
    ) {
      return true;
    }
    const curDecorator = TsUtils.getSendableDecorator(decl);
    if (curDecorator) {
      this.incrementCounters(curDecorator, FaultID.SendableBetaCompatible);
    }
    return false;
  }

  private handleImportType(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }
    this.incrementCounters(node, FaultID.ImportType);
    this.incrementCounters(node, FaultID.DynamicImport);
  }

  private handleVoidExpression(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }
    const autofix = this.autofixer?.fixVoidOperator(node as ts.VoidExpression);
    this.incrementCounters(node, FaultID.VoidOperator, autofix);
  }

  private handleRegularExpressionLiteral(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }
    const autofix = this.autofixer?.fixRegularExpressionLiteral(node as ts.RegularExpressionLiteral);
    this.incrementCounters(node, FaultID.RegularExpressionLiteral, autofix);
  }

  private handleLimitedVoidType(node: ts.VariableDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const typeNode = node.type;
    if (typeNode && TsUtils.typeContainsVoid(typeNode)) {
      const autofix = this.autofixer?.fixLimitedVoidType(node);
      this.incrementCounters(typeNode, FaultID.LimitedVoidType, autofix);
    }
  }

  private handleLimitedVoidWithCall(node: ts.CallExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    if (ts.isPropertyAccessExpression(node.parent)) {
      return;
    }

    const signature = this.tsTypeChecker.getResolvedSignature(node);
    if (!signature) {
      return;
    }

    const returnType = this.tsTypeChecker.getReturnTypeOfSignature(signature);
    if (this.tsTypeChecker.typeToString(returnType) !== 'void') {
      return;
    }

    if (ts.isReturnStatement(node.parent)) {
      const functionLike = TypeScriptLinter.findContainingFunction(node);
      if (functionLike && TypeScriptLinter.isRecursiveCall(node, functionLike)) {
        this.incrementCounters(node, FaultID.LimitedVoidType);
      }
      return;
    }

    this.incrementCounters(node, FaultID.LimitedVoidType);
  }

  private static findContainingFunction(node: ts.Node): ts.FunctionLikeDeclaration | undefined {
    while (node) {
      if (ts.isFunctionDeclaration(node) || ts.isFunctionExpression(node) || ts.isArrowFunction(node)) {
        return node;
      }
      node = node.parent;
    }
    return undefined;
  }

  // Helper function to check if a call is recursive
  private static isRecursiveCall(callExpr: ts.CallExpression, fn: ts.FunctionLikeDeclaration): boolean {
    return (
      ts.isIdentifier(callExpr.expression) &&
      ts.isFunctionDeclaration(fn) &&
      !!fn.name &&
      fn.name.text === callExpr.expression.text
    );
  }

  private handleArrayType(arrayType: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!arrayType || !ts.isArrayTypeNode(arrayType)) {
      return;
    }

    if (arrayType.elementType.kind === ts.SyntaxKind.VoidKeyword) {
      this.incrementCounters(arrayType.elementType, FaultID.LimitedVoidType);
    }
  }

  private handleUnionType(unionType: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!unionType || !ts.isUnionTypeNode(unionType)) {
      return;
    }

    const types = unionType.types;
    for (const type of types) {
      if (type.kind === ts.SyntaxKind.VoidKeyword) {
        this.incrementCounters(type, FaultID.LimitedVoidType);
      }
    }
  }

  private handleDebuggerStatement(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    this.incrementCounters(node, FaultID.DebuggerStatement);
  }

  private handleTSOverload(decl: ts.FunctionDeclaration | ts.MethodDeclaration | ts.ConstructorDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }
    if (decl.name) {
      const symbol = this.tsTypeChecker.getSymbolAtLocation(decl.name);
      if (!symbol) {
        return;
      }
      const declarations = symbol.getDeclarations();
      if (!declarations) {
        return;
      }
      const filterDecl = declarations.filter((name) => {
        return ts.isFunctionDeclaration(name) || ts.isMethodDeclaration(name);
      });
      const isInternalFunction = decl.name && ts.isIdentifier(decl.name) && interanlFunction.includes(decl.name.text);
      if (isInternalFunction && filterDecl.length > 2 || !isInternalFunction && filterDecl.length > 1) {
        this.incrementCounters(decl, FaultID.TsOverload);
      }
    } else if (ts.isConstructorDeclaration(decl) && decl.getText()) {
      this.handleTSOverloadUnderConstructorDeclaration(decl);
    }
  }

  private handleTSOverloadUnderConstructorDeclaration(decl: ts.ConstructorDeclaration): void {
    const parent = decl.parent;
    const constructors = parent.members.filter(ts.isConstructorDeclaration);
    const isStruct = decl.getText() && ts.isStructDeclaration(parent);
    if ((isStruct ? --constructors.length : constructors.length) > 1) {
      this.incrementCounters(decl, FaultID.TsOverload);
    }
  }

  private handleSwitchStatement(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }
    const switchStatement = node as ts.SwitchStatement;

    this.validateSwitchExpression(switchStatement);

    const duplicateCases = this.findDuplicateCases(switchStatement);
    if (duplicateCases.length > 0) {
      for (const duplicateCase of duplicateCases) {
        this.incrementCounters(duplicateCase.expression, FaultID.CaseExpression);
      }
    }
  }

  private validateSwitchExpression(switchStatement: ts.SwitchStatement): void {
    const expr = switchStatement.expression;
    const nodeType = this.tsTypeChecker.getTypeAtLocation(expr);
    const { isLiteralInitialized, isFloatLiteral, hasExplicitTypeAnnotation } = this.getDeclarationInfo(expr);

    const isUnionType = (nodeType.flags & ts.TypeFlags.Union) !== 0;

    const isTypeAllowed = (t: ts.Type): boolean => {
      const typeText = this.tsTypeChecker.typeToString(t);
      return Boolean(
        t.flags & ts.TypeFlags.StringLike ||
          typeText === 'String' ||
          t.flags & ts.TypeFlags.NumberLike && (/^\d+$/).test(typeText) ||
          isLiteralInitialized && !hasExplicitTypeAnnotation && !isFloatLiteral ||
          t.flags & ts.TypeFlags.EnumLike
      );
    };

    let isAllowed = !isUnionType && isTypeAllowed(nodeType);

    if (isUnionType) {
      const unionType = nodeType as ts.UnionType;
      isAllowed = unionType.types.every(isTypeAllowed);
    }

    if (!isAllowed) {
      this.incrementCounters(expr, FaultID.SwitchExpression);
    }
  }

  private getDeclarationInfo(expression: ts.Expression): {
    isLiteralInitialized: boolean;
    isFloatLiteral: boolean;
    hasExplicitTypeAnnotation: boolean;
  } {
    const symbol = this.tsTypeChecker.getSymbolAtLocation(expression);
    const declaration = symbol?.valueDeclaration;

    if (!declaration || !ts.isVariableDeclaration(declaration)) {
      return { isLiteralInitialized: false, isFloatLiteral: false, hasExplicitTypeAnnotation: false };
    }

    const hasExplicitTypeAnnotation = !!declaration.type;
    const initializerInfo = TypeScriptLinter.getInitializerInfo(declaration.initializer);

    return {
      isLiteralInitialized: initializerInfo.isLiteralInitialized,
      isFloatLiteral: initializerInfo.isFloatLiteral,
      hasExplicitTypeAnnotation
    };
  }

  private static getInitializerInfo(initializer?: ts.Expression): {
    isLiteralInitialized: boolean;
    isFloatLiteral: boolean;
  } {
    if (!initializer) {
      return { isLiteralInitialized: false, isFloatLiteral: false };
    }

    const isLiteralInitialized = ts.isNumericLiteral(initializer) || ts.isStringLiteral(initializer);

    let isFloatLiteral = false;
    if (ts.isNumericLiteral(initializer)) {
      const literalText = initializer.getText();
      if (!(/^0[xX]/).test(literalText)) {
        isFloatLiteral = (/\.|e[-+]|\dE[-+]/i).test(literalText);
      }
    }

    return { isLiteralInitialized, isFloatLiteral };
  }

  private findDuplicateCases(switchStatement: ts.SwitchStatement): ts.CaseClause[] {
    const seenValues = new Map<string | number | boolean, ts.CaseClause>();
    const duplicates: ts.CaseClause[] = [];

    for (const clause of switchStatement.caseBlock.clauses) {
      if (ts.isCaseClause(clause) && clause.expression) {
        const value = this.getConstantValue(clause.expression);
        const key = value !== undefined ? value : clause.expression.getText();
        if (seenValues.has(key)) {
          duplicates.push(clause);
        } else {
          seenValues.set(key, clause);
        }
      }
    }
    return duplicates;
  }

  private getConstantValue(expression: ts.Expression): string | number | boolean | undefined {
    if (ts.isLiteralExpression(expression)) {
      return ts.isNumericLiteral(expression) ? Number(expression.text) : expression.text;
    }

    switch (expression.kind) {
      case ts.SyntaxKind.TrueKeyword:
        return true;
      case ts.SyntaxKind.FalseKeyword:
        return false;
      default:
        if (ts.isElementAccessExpression(expression) || ts.isPropertyAccessExpression(expression)) {
          const constantValue = this.tsTypeChecker.getConstantValue(expression);
          if (constantValue !== undefined) {
            return constantValue;
          }
        }
        return undefined;
    }
  }

  private handleLimitedLiteralType(literalTypeNode: ts.LiteralTypeNode): void {
    if (!this.options.arkts2 || !literalTypeNode) {
      return;
    }
    const literal = literalTypeNode.literal;
    if (
      !(
        literal.kind === ts.SyntaxKind.StringLiteral ||
        literal.kind === ts.SyntaxKind.NullKeyword ||
        literal.kind === ts.SyntaxKind.UndefinedKeyword
      )
    ) {
      this.incrementCounters(literalTypeNode, FaultID.LimitedLiteralType);
    }
  }

  private findVariableInitializationValue(identifier: ts.Identifier): number | null {
    const symbol = this.tsTypeChecker.getSymbolAtLocation(identifier);
    if (!symbol) {
      return null;
    }
    if (this.constVariableInitCache.has(symbol)) {
      return this.constVariableInitCache.get(symbol)!;
    }
    const declarations = symbol.getDeclarations();
    if (declarations && declarations.length > 0) {
      const declaration = declarations[0];

      const isConditionOnEnumMember = ts.isEnumMember(declaration) && declaration.initializer;
      const isConditionOnVariableDecl =
        ts.isVariableDeclaration(declaration) &&
        declaration.initializer &&
        (declaration.parent as ts.VariableDeclarationList).flags & ts.NodeFlags.Const;
      if (isConditionOnEnumMember || isConditionOnVariableDecl) {
        const res = this.evaluateNumericValue(declaration.initializer);
        this.constVariableInitCache.set(symbol, res);
        return res;
      }
    }

    return null;
  }

  private evaluateNumericValueFromPrefixUnaryExpression(node: ts.PrefixUnaryExpression): number | null {
    if (node.operator === ts.SyntaxKind.MinusToken) {
      if (ts.isNumericLiteral(node.operand) || ts.isIdentifier(node.operand) && node.operand.text === 'Infinity') {
        return node.operand.text === 'Infinity' ? Number.NEGATIVE_INFINITY : -Number(node.operand.text);
      }
      const operandValue = this.evaluateNumericValue(node.operand);
      if (operandValue !== null) {
        return -operandValue;
      }
    }
    return null;
  }

  private evaluateNumericValueFromAsExpression(node: ts.AsExpression): number | null {
    const typeNode = node.type;
    if (
      typeNode.kind === ts.SyntaxKind.NumberKeyword ||
      ts.isTypeReferenceNode(typeNode) && typeNode.typeName.getText() === 'Number'
    ) {
      return this.evaluateNumericValue(node.expression);
    }
    return null;
  }

  private evaluateNumericValue(node: ts.Expression): number | null {
    let result: number | null = null;
    if (ts.isNumericLiteral(node)) {
      result = Number(node.text);
    } else if (ts.isPrefixUnaryExpression(node)) {
      result = this.evaluateNumericValueFromPrefixUnaryExpression(node);
    } else if (ts.isBinaryExpression(node)) {
      result = this.evaluateNumericValueFromBinaryExpression(node);
    } else if (ts.isPropertyAccessExpression(node)) {
      result = this.evaluateNumericValueFromPropertyAccess(node);
    } else if (ts.isParenthesizedExpression(node)) {
      result = this.evaluateNumericValue(node.expression);
    } else if (ts.isAsExpression(node)) {
      result = this.evaluateNumericValueFromAsExpression(node);
    } else if (ts.isIdentifier(node)) {
      if (node.text === 'Infinity') {
        return Number.POSITIVE_INFINITY;
      } else if (node.text === 'NaN') {
        return Number.NaN;
      }
      const symbol = this.tsTypeChecker.getSymbolAtLocation(node);
      return symbol ? this.constVariableInitCache.get(symbol) || null : null;
    }
    return result;
  }

  private evaluateNumericValueFromBinaryExpression(node: ts.BinaryExpression): number | null {
    const leftValue = this.evaluateNumericValue(node.left);
    const rightValue = this.evaluateNumericValue(node.right);
    if (leftValue !== null && rightValue !== null) {
      switch (node.operatorToken.kind) {
        case ts.SyntaxKind.PlusToken:
          return leftValue + rightValue;
        case ts.SyntaxKind.MinusToken:
          return leftValue - rightValue;
        case ts.SyntaxKind.AsteriskToken:
          return leftValue * rightValue;
        case ts.SyntaxKind.SlashToken:
          return leftValue / rightValue;
        case ts.SyntaxKind.PercentToken:
          return leftValue % rightValue;
        case ts.SyntaxKind.AsteriskAsteriskToken:
          return Math.pow(leftValue, rightValue);
        default:
          return null;
      }
    }
    return null;
  }

  private evaluateNumericValueFromPropertyAccess(node: ts.PropertyAccessExpression): number | null {
    const numberProperties = ['MIN_SAFE_INTEGER', 'MAX_SAFE_INTEGER', 'NaN', 'NEGATIVE_INFINITY', 'POSITIVE_INFINITY'];
    if (
      ts.isIdentifier(node.expression) &&
      node.expression.text === 'Number' &&
      numberProperties.includes(node.name.text)
    ) {
      switch (node.name.text) {
        case 'MIN_SAFE_INTEGER':
          return Number.MIN_SAFE_INTEGER;
        case 'MAX_SAFE_INTEGER':
          return Number.MAX_SAFE_INTEGER;
        case 'NaN':
          return Number.NaN;
        case 'NEGATIVE_INFINITY':
          return Number.NEGATIVE_INFINITY;
        case 'POSITIVE_INFINITY':
          return Number.POSITIVE_INFINITY;
        default:
          return null;
      }
    }
    return this.evaluateNumericValue(node.name);
  }

  private collectVariableNamesAndCache(node: ts.Node): void {
    if (ts.isIdentifier(node)) {
      const value = this.findVariableInitializationValue(node);
      const symbol = this.tsTypeChecker.getSymbolAtLocation(node);
      if (value && symbol) {
        this.constVariableInitCache.set(symbol, value);
      }
    }
    ts.forEachChild(node, this.collectVariableNamesAndCache.bind(this));
  }

  private handleIndexNegative(node: ts.Node): void {
    if (!this.options.arkts2 || !ts.isElementAccessExpression(node)) {
      return;
    }
    const indexNode = node.argumentExpression;
    if (indexNode) {
      this.collectVariableNamesAndCache(indexNode);
      const indexValue = this.evaluateNumericValue(indexNode);

      if (indexValue !== null && (indexValue < 0 || isNaN(indexValue))) {
        this.incrementCounters(node, FaultID.IndexNegative);
      }
    }
  }

  private handleNoTuplesArrays(node: ts.Node, lhsType: ts.Type, rhsType: ts.Type): void {
    if (!this.options.arkts2) {
      return;
    }
    if (
      this.tsUtils.isOrDerivedFrom(lhsType, this.tsUtils.isArray) &&
        this.tsUtils.isOrDerivedFrom(rhsType, TsUtils.isTuple) ||
      this.tsUtils.isOrDerivedFrom(rhsType, this.tsUtils.isArray) &&
        this.tsUtils.isOrDerivedFrom(lhsType, TsUtils.isTuple)
    ) {
      this.incrementCounters(node, FaultID.NoTuplesArrays);
    }
  }

  private handleNoTuplesArraysForPropertyAccessExpression(node: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    const lhsType = this.tsTypeChecker.getTypeAtLocation(node.expression);
    if (this.tsUtils.isOrDerivedFrom(lhsType, TsUtils.isTuple)) {
      if (ARRAY_API_LIST.includes(node.name.text)) {
        this.incrementCounters(node, FaultID.NoTuplesArrays);
      }
    }
  }

  private handleArrayTypeImmutable(node: ts.Node, lhsType: ts.Type, rhsType: ts.Type, rhsExpr?: ts.Expression): void {
    if (!this.options.arkts2) {
      return;
    }
    const isArray =
      this.tsUtils.isOrDerivedFrom(lhsType, this.tsUtils.isArray) &&
      this.tsUtils.isOrDerivedFrom(rhsType, this.tsUtils.isArray);
    const isTuple =
      this.tsUtils.isOrDerivedFrom(lhsType, TsUtils.isTuple) && this.tsUtils.isOrDerivedFrom(rhsType, TsUtils.isTuple);
    if (!((isArray || isTuple) && lhsType !== rhsType)) {
      return;
    }

    const rhsTypeStr = this.tsTypeChecker.typeToString(rhsType);
    let lhsTypeStr = this.tsTypeChecker.typeToString(lhsType);
    if (rhsExpr && (this.isNullOrEmptyArray(rhsExpr) || ts.isArrayLiteralExpression(rhsExpr))) {
      return;
    }

    if (ts.isAsExpression(node) && ts.isArrayLiteralExpression(node.expression)) {
      node.expression.elements.forEach((elem) => {
        if (elem.kind === ts.SyntaxKind.FalseKeyword || elem.kind === ts.SyntaxKind.TrueKeyword) {
          lhsTypeStr = rhsTypeStr.replace(elem.getText(), 'boolean');
        }
      });
    }
    if (lhsTypeStr !== rhsTypeStr) {
      this.incrementCounters(node, FaultID.ArrayTypeImmutable);
    }
  }

  private isNullOrEmptyArray(expr: ts.Expression): boolean {
    if (ts.isNewExpression(expr)) {
      const constructorSym = this.tsTypeChecker.getSymbolAtLocation(expr.expression);
      if (constructorSym?.name === 'Array') {
        if (!expr.arguments || expr.arguments.length === 0) {
          return true;
        }
        if (expr.arguments.length === 1) {
          const argType = this.tsTypeChecker.getTypeAtLocation(expr.arguments[0]);
          return !!(argType.flags & ts.TypeFlags.NumberLike);
        }
      }
    }

    return false;
  }

  private handleExponentOperation(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }
    const autofix = this.autofixer?.fixExponent(node.parent);
    this.incrementCounters(node, FaultID.ExponentOp, autofix);
  }

  private handleNonNullExpression(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    if (
      !ts.isNonNullExpression(node) ||
      !ts.isNonNullExpression(node.expression) ||
      ts.isNonNullExpression(node.parent) ||
      ts.isPropertyAccessExpression(node.parent) ||
      ts.isNonNullExpression(node.expression.expression)
    ) {
      return;
    }

    const statement = ts.findAncestor(node, ts.isExpressionStatement);
    if (statement && this.isCustomComponent(statement)) {
      this.handleCustomBidirectionalBinding(node, node.expression);
    } else {
      const autofix = this.autofixer?.fixNativeBidirectionalBinding(node, this.interfacesNeedToImport);
      this.incrementCounters(node, FaultID.DoubleExclaBindingNotSupported, autofix);
    }
  }

  private isCustomComponent(statement: ts.ExpressionStatement): boolean {
    const callExpr = statement.expression;
    if (!ts.isCallExpression(callExpr)) {
      return false;
    }

    const identifier = callExpr.expression;
    if (!ts.isIdentifier(identifier)) {
      return false;
    }

    const symbol = this.tsTypeChecker.getSymbolAtLocation(identifier);
    if (symbol) {
      const decl = this.tsUtils.getDeclarationNode(identifier);
      if (decl?.getSourceFile() === statement.getSourceFile()) {
        return true;
      }
    }

    return this.interfacesAlreadyImported.has(callExpr.expression.getText());
  }

  private handleCustomBidirectionalBinding(firstExpr: ts.NonNullExpression, secondExpr: ts.NonNullExpression): void {
    let currentParam: ts.Identifier | undefined;
    if (ts.isPropertyAccessExpression(secondExpr.expression)) {
      currentParam = secondExpr.expression.name as ts.Identifier;
    }

    let customParam: ts.Identifier | undefined;
    if (ts.isPropertyAssignment(firstExpr.parent)) {
      customParam = firstExpr.parent.name as ts.Identifier;
    }

    if (!currentParam || !customParam) {
      return;
    }

    const originalExpr = firstExpr.parent.parent;
    if (!ts.isObjectLiteralExpression(originalExpr)) {
      return;
    }

    const autofix = this.autofixer?.fixCustomBidirectionalBinding(originalExpr, currentParam, customParam);
    this.incrementCounters(firstExpr, FaultID.DoubleExclaBindingNotSupported, autofix);
  }

  private handleDoubleDollar(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    if (
      ts.isPropertyAccessExpression(node) &&
      ts.isIdentifier(node.expression) &&
      node.expression.escapedText === DOUBLE_DOLLAR_IDENTIFIER + THIS_IDENTIFIER
    ) {
      const autofix = this.autofixer?.fixDoubleDollar(node, this.interfacesNeedToImport);
      this.incrementCounters(node, FaultID.DoubleDollarBindingNotSupported, autofix);
    }
  }

  private handleDollarBind(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!ts.isPropertyAssignment(node) || !ts.isIdentifier(node.initializer)) {
      return;
    }

    const text = node.initializer.getText();
    if (!(/^\$.+$/).test(text)) {
      return;
    }

    const autofix = this.autofixer?.fixDollarBind(node);
    this.incrementCounters(node, FaultID.DollarBindingNotSupported, autofix);
  }

  private handleExtendDecorator(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!ts.isFunctionDeclaration(node.parent) || !ts.isDecorator(node)) {
      return;
    }

    if (ts.isCallExpression(node.expression) && ts.isIdentifier(node.expression.expression)) {
      if (node.expression.expression.text === CustomDecoratorName.Extend) {
        const autofix = this.autofixer?.fixExtendDecorator(node, false, this.interfacesNeedToImport);
        this.incrementCounters(node.parent, FaultID.ExtendDecoratorNotSupported, autofix);
      } else if (node.expression.expression.text === CustomDecoratorName.AnimatableExtend) {
        const autofix = this.autofixer?.fixExtendDecorator(node, true, this.interfacesNeedToImport);
        this.incrementCounters(node.parent, FaultID.AnimatableExtendDecoratorTransform, autofix);
      }
    }
  }

  private handleEntryDecorator(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!ts.isDecorator(node)) {
      return;
    }

    if (ts.isCallExpression(node.expression) && ts.isIdentifier(node.expression.expression)) {
      if (node.expression.expression.escapedText !== ENTRY_DECORATOR_NAME || node.expression.arguments.length !== 1) {
        return;
      }
      const arg = node.expression.arguments[0];
      if (ts.isObjectLiteralExpression(arg)) {
        const properties = arg.properties;
        if (properties.length !== 1) {
          return;
        }
        if (!ts.isPropertyAssignment(properties[0])) {
          return;
        }
        const property = properties[0];
        if (ts.isStringLiteral(property.initializer)) {
          return;
        }
      }
      const autofix = this.autofixer?.fixEntryDecorator(node);
      this.incrementCounters(node, FaultID.EntryAnnotation, autofix);
    }
  }

  private handleStructPropertyDecl(propDecl: ts.PropertyDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }
    const isStatic = TsUtils.hasModifier(propDecl.modifiers, ts.SyntaxKind.StaticKeyword);
    const hasNoInitializer = !propDecl.initializer;
    const isOptional = !!propDecl.questionToken;

    const defaultSkipTypeCheck = (typeNode: ts.TypeNode | undefined): boolean => {
      if (!typeNode) {
        return false;
      }

      const typeText = typeNode.getText();
      if (ts.isLiteralTypeNode(typeNode) || ['boolean', 'number', 'null', 'undefined'].includes(typeText)) {
        return true;
      }

      if (ts.isUnionTypeNode(typeNode)) {
        return typeNode.types.some((t) => {
          const tText = t.getText();
          return tText === 'undefined';
        });
      }

      return false;
    };

    const shouldSkipCheck = isOptional || defaultSkipTypeCheck(propDecl.type);

    if (isStatic && hasNoInitializer && !shouldSkipCheck) {
      this.incrementCounters(propDecl, FaultID.ClassstaticInitialization);
    }
  }

  private handleTaggedTemplatesExpression(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }
    this.incrementCounters(node, FaultID.TaggedTemplates);
  }

  private checkFunctionTypeCompatible(lhsTypeNode: ts.TypeNode | undefined, rhsExpr: ts.Expression): void {
    if (this.options.arkts2 && lhsTypeNode && this.tsUtils.isIncompatibleFunctionals(lhsTypeNode, rhsExpr)) {
      this.incrementCounters(rhsExpr, FaultID.IncompationbleFunctionType);
    }
  }

  private handleInvalidIdentifier(
    decl:
      | ts.TypeAliasDeclaration
      | ts.StructDeclaration
      | ts.VariableDeclaration
      | ts.FunctionDeclaration
      | ts.MethodSignature
      | ts.ClassDeclaration
      | ts.PropertyDeclaration
      | ts.MethodDeclaration
      | ts.ParameterDeclaration
      | ts.PropertySignature
      | ts.ImportDeclaration
      | ts.EnumDeclaration
      | ts.EnumMember
      | ts.ModuleDeclaration
      | ts.InterfaceDeclaration
  ): void {
    if (!this.options.arkts2) {
      return;
    }

    const checkIdentifier = (identifier: ts.Identifier | undefined): void => {
      const text = identifier && ts.isIdentifier(identifier) ? identifier.text : '';
      if (identifier && text && INVALID_IDENTIFIER_KEYWORDS.includes(text)) {
        this.incrementCounters(identifier, FaultID.InvalidIdentifier);
      }
    };

    if (ts.isImportDeclaration(decl)) {
      const importClause = decl.importClause;
      if (importClause?.namedBindings && ts.isNamedImports(importClause?.namedBindings)) {
        importClause.namedBindings.elements.forEach((importSpecifier) => {
          checkIdentifier(importSpecifier.name);
        });
      }
      checkIdentifier(importClause?.name);
    } else if (isStructDeclaration(decl)) {
      checkIdentifier((decl as ts.StructDeclaration).name);
    } else {
      checkIdentifier(decl.name as ts.Identifier);
    }
  }

  private handleHeritageClause(node: ts.HeritageClause): void {
    this.checkEWTArgumentsForSdkDuplicateDeclName(node);
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }
    if (node.token === ts.SyntaxKind.ExtendsKeyword || node.token === ts.SyntaxKind.ImplementsKeyword) {
      node.types.forEach((type) => {
        const expr = type.expression;
        if (ts.isCallExpression(expr)) {
          this.incrementCounters(expr, FaultID.ExtendsExpression);
          return;
        }
        if (
          ts.isIdentifier(expr) &&
          this.isVariableReference(expr) &&
          this.tsUtils.isBuiltinClassHeritageClause(node)
        ) {
          this.incrementCounters(expr, FaultID.ExtendsExpression);
        } else if (ts.isIdentifier(expr)) {
          this.fixJsImportExtendsClass(node.parent, expr);
        }
      });

      this.handleMissingSuperCallInExtendedClass(node);
    }
  }

  private isVariableReference(identifier: ts.Identifier): boolean {
    const symbol = this.tsTypeChecker.getSymbolAtLocation(identifier);
    return !!symbol && (symbol.flags & ts.SymbolFlags.Variable) !== 0;
  }

  private checkSendableAndConcurrentDecorator(decorator: ts.Decorator): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }
    const decoratorName = TsUtils.getDecoratorName(decorator);
    const autofix = this.autofixer?.removeNode(decorator);
    if (decoratorName === SENDABLE_DECORATOR) {
      this.incrementCounters(decorator, FaultID.LimitedStdLibNoSendableDecorator, autofix);
    }

    if (decoratorName === CONCURRENT_DECORATOR) {
      this.incrementCounters(decorator, FaultID.LimitedStdLibNoDoncurrentDecorator, autofix);
    }
  }

  private checkAsonSymbol(node: ts.Identifier): void {
    if (!this.options.arkts2) {
      return;
    }

    if (node.text !== ASON_TEXT) {
      return;
    }

    const parent = node.parent;
    switch (parent.kind) {
      case ts.SyntaxKind.QualifiedName:
        if (!ts.isQualifiedName(parent)) {
          return;
        }
        if (parent.right.text !== node.text) {
          return;
        }
        if (ts.isQualifiedName(parent.parent) && ASON_WHITE_SET.has(parent.parent.right.text)) {
          this.checkAsonUsage(parent.left, true);
        } else {
          this.checkAsonUsage(parent.left, false);
        }
        break;
      case ts.SyntaxKind.PropertyAccessExpression:
        if (!ts.isPropertyAccessExpression(parent)) {
          return;
        }
        if (parent.name.text !== node.text) {
          return;
        }
        if (ts.isPropertyAccessExpression(parent.parent) && ASON_WHITE_SET.has(parent.parent.name.text)) {
          this.checkAsonUsage(parent.expression, true);
        } else {
          this.checkAsonUsage(parent.expression, false);
        }
        break;
      default:
    }
  }

  private checkAsonUsage(nodeToCheck: ts.Node, needAutofix: boolean): void {
    if (!ts.isIdentifier(nodeToCheck)) {
      return;
    }

    const declaration = this.tsUtils.getDeclarationNode(nodeToCheck);
    if (!declaration && nodeToCheck.text === ARKTS_UTILS_TEXT) {
      const autofix =
        needAutofix && this.autofixer ? this.autofixer.replaceNode(nodeToCheck.parent, JSON_TEXT) : undefined;
      this.incrementCounters(nodeToCheck, FaultID.LimitedStdLibNoASON, autofix);
      return;
    }

    if (!declaration) {
      return;
    }

    const sourceFile = declaration.getSourceFile();
    const fileName = path.basename(sourceFile.fileName);

    if (
      ASON_MODULES.some((moduleName) => {
        return fileName.startsWith(moduleName);
      })
    ) {
      const autofix =
        needAutofix && this.autofixer ? this.autofixer.replaceNode(nodeToCheck.parent, JSON_TEXT) : undefined;
      this.incrementCounters(nodeToCheck, FaultID.LimitedStdLibNoASON, autofix);
    }
  }

  private checkCollectionsSymbol(symbol: ts.Symbol, node: ts.Node): void {
    const cb = (): void => {
      const parent = node.parent;
      if (!parent) {
        return;
      }
      if (ts.isPropertyAccessExpression(parent)) {
        const autofix = this.autofixer?.replaceNode(parent, parent.name.text);
        this.incrementCounters(node, FaultID.NoNeedStdLibSendableContainer, autofix);
      }

      if (ts.isQualifiedName(parent)) {
        const autofix = this.autofixer?.replaceNode(parent, parent.right.text);
        this.incrementCounters(node, FaultID.NoNeedStdLibSendableContainer, autofix);
      }

      if (ts.isImportSpecifier(parent) && ts.isIdentifier(node)) {
        const autofix = this.autofixer?.removeImport(node, parent);
        this.incrementCounters(node, FaultID.NoNeedStdLibSendableContainer, autofix);
      }
    };

    this.checkSymbolAndExecute(symbol, COLLECTIONS_TEXT, COLLECTIONS_MODULES, cb);
  }

  private checkWorkerSymbol(symbol: ts.Symbol, node: ts.Node): void {
    const cb = (): void => {
      this.incrementCounters(node, FaultID.NoNeedStdlibWorker);
    };

    this.checkSymbolAndExecute(symbol, WORKER_TEXT, WORKER_MODULES, cb);
  }

  private checkConcurrencySymbol(symbol: ts.Symbol, node: ts.Node): void {
    const cb = (): void => {
      const parent = node.parent;
      if (!ts.isPropertyAccessExpression(parent)) {
        return;
      }
      if (parent.name.text === ARKTSUTILS_LOCKS_MEMBER) {
        this.incrementCounters(node, FaultID.LimitedStdLibNoImportConcurrency);
      }
    };

    this.checkSymbolAndExecute(symbol, ARKTSUTILS_LOCKS_MEMBER, ARKTSUTILS_MODULES, cb);
  }

  private checkSymbolAndExecute(symbol: ts.Symbol, symbolName: string, modules: string[], cb: () => void): void {
    void this;
    if (symbol.name === symbolName) {
      const decl = TsUtils.getDeclaration(symbol);

      if (!decl) {
        return;
      }

      const sourceFile = decl.getSourceFile();
      const fileName = path.basename(sourceFile.fileName);

      if (
        modules.some((moduleName) => {
          return fileName.startsWith(moduleName);
        })
      ) {
        cb();
      }
    }
  }

  interfacesNeedToAlarm: ts.Identifier[] = [];
  interfacesNeedToImport: Set<string> = new Set<string>();
  interfacesAlreadyImported: Set<string> = new Set<string>();

  private handleInterfaceImport(identifier: ts.Identifier): void {
    if (!this.options.arkts2) {
      return;
    }

    if (this.shouldSkipIdentifier(identifier)) {
      return;
    }

    const name = identifier.getText();
    if (!this.interfacesNeedToImport.has(name)) {
      this.interfacesNeedToImport.add(name);
    }

    this.interfacesNeedToAlarm.push(identifier);
  }

  private shouldSkipIdentifier(identifier: ts.Identifier): boolean {
    const name = identifier.getText();
    if (!arkuiImportList.has(name)) {
      return true;
    }

    if (skipImportDecoratorName.has(name)) {
      return true;
    }

    const targetPropertyAccess = TypeScriptLinter.findTargetPropertyAccess(identifier.parent);
    if (targetPropertyAccess) {
      const expr = targetPropertyAccess.expression;
      if (this.isDeclarationInSameFile(expr)) {
        return true;
      }
    }

    const parent = identifier.parent;
    const wrappedSkipComponents = new Set<string>([CustomDecoratorName.AnimatableExtend, CustomDecoratorName.Extend]);
    if (ts.isCallExpression(parent)) {
      const expr = parent.expression;
      if (wrappedSkipComponents.has(expr.getText()) && name !== CustomDecoratorName.AnimatableExtend) {
        return true;
      }
    }

    if (this.isDeclarationInSameFile(identifier)) {
      return true;
    }

    return this.interfacesAlreadyImported.has(name);
  }

  private static findTargetPropertyAccess(node: ts.Node): ts.PropertyAccessExpression | undefined {
    while (ts.isPropertyAccessExpression(node)) {
      const expr = node.expression;
      if (!ts.isPropertyAccessExpression(expr)) {
        return node;
      }
      node = expr;
    }
    return undefined;
  }

  private isDeclarationInSameFile(node: ts.Node): boolean {
    const symbol = this.tsTypeChecker.getSymbolAtLocation(node);
    const decl = TsUtils.getDeclaration(symbol);
    if (decl?.getSourceFile() === node.getSourceFile()) {
      return true;
    }

    return false;
  }

  private processInterfacesToImport(sourceFile: ts.SourceFile): void {
    if (!this.options.arkts2) {
      return;
    }

    const autofix = this.autofixer?.fixInterfaceImport(
      this.interfacesNeedToImport,
      this.interfacesAlreadyImported,
      sourceFile
    );

    this.interfacesNeedToAlarm.forEach((identifier) => {
      const name = identifier.getText();
      const errorMsg = `The ArkUI interface "${name}" should be imported before it is used (arkui-modular-interface)`;
      this.incrementCounters(identifier, FaultID.UIInterfaceImport, autofix, errorMsg);
    });

    this.interfacesNeedToAlarm = [];
    this.interfacesNeedToImport.clear();
    this.interfacesAlreadyImported.clear();
  }

  private extractImportedNames(sourceFile: ts.SourceFile): void {
    if (!this.options.arkts2) {
      return;
    }
    for (const statement of sourceFile.statements) {
      if (!ts.isImportDeclaration(statement)) {
        continue;
      }

      const importClause = statement.importClause;
      if (!importClause) {
        continue;
      }

      const namedBindings = importClause.namedBindings;
      if (!namedBindings || !ts.isNamedImports(namedBindings)) {
        continue;
      }

      for (const specifier of namedBindings.elements) {
        const importedName = specifier.name.getText(sourceFile);
        this.interfacesAlreadyImported.add(importedName);
      }
    }
  }

  private handleStylesDecorator(node: ts.Decorator): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!ts.isFunctionDeclaration(node.parent) && !ts.isMethodDeclaration(node.parent)) {
      return;
    }

    if (!ts.isIdentifier(node.expression) || node.expression.text !== CustomDecoratorName.Styles) {
      return;
    }

    const decl = node.parent;
    const declName = decl.name?.getText();
    if (ts.isFunctionDeclaration(decl)) {
      const functionCalls = TypeScriptLinter.findDeclarationCalls(this.sourceFile, declName as string);
      const autofix = this.autofixer?.fixStylesDecoratorGlobal(decl, functionCalls, this.interfacesNeedToImport);
      this.incrementCounters(decl, FaultID.StylesDecoratorNotSupported, autofix);
    }

    if (ts.isMethodDeclaration(decl)) {
      const methodCalls = TypeScriptLinter.findDeclarationCalls(this.sourceFile, declName as string);
      const autofix = this.autofixer?.fixStylesDecoratorStruct(decl, methodCalls, this.interfacesNeedToImport);
      this.incrementCounters(decl, FaultID.StylesDecoratorNotSupported, autofix);
    }
  }

  private handleStateStyles(node: ts.CallExpression | ts.PropertyAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    let args: ts.Expression[] = [];
    let startNode: ts.Node | undefined;
    if (ts.isCallExpression(node)) {
      if (node.expression.getText() !== STATE_STYLES) {
        return;
      }
      startNode = node.expression;
      args = Array.from(node.arguments);
    }

    if (ts.isPropertyAccessExpression(node)) {
      if (node.name.getText() !== STATE_STYLES) {
        return;
      }
      if (!ts.isCallExpression(node.parent)) {
        return;
      }
      startNode = node.name;
      args = Array.from(node.parent.arguments);
    }

    if (args.length === 0 || !startNode) {
      return;
    }

    const object = args[0];
    if (!object || !ts.isObjectLiteralExpression(object)) {
      return;
    }

    const properties = object.properties;
    if (properties.length === 0) {
      return;
    }

    if (!TypeScriptLinter.hasAnonBlock(properties)) {
      return;
    }

    const autofix = this.autofixer?.fixStateStyles(object, startNode, this.interfacesNeedToImport);
    this.incrementCounters(object, FaultID.StateStylesBlockNeedArrowFunc, autofix);
  }

  private static hasAnonBlock(properties: ts.NodeArray<ts.ObjectLiteralElementLike>): boolean {
    let anonBlockCount = 0;

    properties.forEach((property) => {
      if (ts.isPropertyAssignment(property) && ts.isObjectLiteralExpression(property.initializer)) {
        anonBlockCount++;
      }
    });

    return anonBlockCount !== 0;
  }

  private handleStringLiteral(node: ts.StringLiteral): void {
    if (!this.options.arkts2) {
      return;
    }

    this.checkForConcurrentExpressions(node);
  }

  private checkForConcurrentExpressions(stringLiteral: ts.StringLiteral): void {
    if (!stringLiteral.parent) {
      return;
    }

    if (!ts.isExpressionStatement(stringLiteral.parent)) {
      return;
    }

    const text = stringLiteral.text;
    const autofix = this.autofixer?.removeNode(stringLiteral);

    if (text === USE_CONCURRENT) {
      this.incrementCounters(stringLiteral, FaultID.UseConcurrentDeprecated, autofix);
    }

    if (text === USE_SHARED) {
      this.incrementCounters(stringLiteral, FaultID.UseSharedDeprecated, autofix);
    }
  }

  private static findDeclarationCalls(sourceFile: ts.SourceFile, declName: string): ts.Identifier[] {
    const functionCalls: ts.Identifier[] = [];

    function traverse(node: ts.Node): void {
      const identifier = getIdentifierFromNode(node);
      if (identifier && identifier.getText() === declName) {
        functionCalls.push(identifier);
      }

      ts.forEachChild(node, traverse);
    }

    function getIdentifierFromNode(node: ts.Node): ts.Identifier | undefined {
      if (ts.isCallExpression(node) && ts.isIdentifier(node.expression)) {
        return node.expression;
      }
      if (ts.isPropertyAccessExpression(node) && ts.isIdentifier(node.name)) {
        if (node.expression.getText() === THIS_IDENTIFIER) {
          return undefined;
        }
        return node.name;
      }
      return undefined;
    }

    traverse(sourceFile);
    return functionCalls;
  }

  addObservedDecorator: Set<ts.ClassDeclaration> = new Set<ts.ClassDeclaration>();

  private handleDataObservation(node: ts.PropertyDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const decorators = ts.getDecorators(node);
    if (!decorators || decorators.length === 0) {
      return;
    }
    const decorator = decorators[0];
    let decoratorName = '';
    if (ts.isIdentifier(decorator.expression)) {
      decoratorName = decorator.expression.getText();
    } else if (ts.isCallExpression(decorator.expression)) {
      decoratorName = decorator.expression.expression.getText();
    }
    if (!observedDecoratorName.has(decoratorName)) {
      return;
    }

    let firstClassDecls: ts.ClassDeclaration[] | undefined;
    const expr = node.initializer;
    if (expr && ts.isNewExpression(expr)) {
      firstClassDecls = this.addFromNewExpression(expr);
    }

    let secondClassDecls: ts.ClassDeclaration[] | undefined;
    const type = node.type;
    if (type) {
      secondClassDecls = this.addFromTypeNode(type);
    }

    const classDecls = (firstClassDecls || []).concat(secondClassDecls || []);
    if (classDecls.length === 0) {
      return;
    }

    const filteredClassDecls = classDecls.filter((classDecl) => {
      if (this.addObservedDecorator.has(classDecl)) {
        return false;
      }
      this.addObservedDecorator.add(classDecl);
      return true;
    });
    if (filteredClassDecls.length !== 0) {
      this.interfacesNeedToImport.add(CustomDecoratorName.Observed);
    }
    const autofix = this.autofixer?.fixDataObservation(filteredClassDecls);
    this.incrementCounters(node, FaultID.DataObservation, autofix);
  }

  private addFromNewExpression(expr: ts.NewExpression): ts.ClassDeclaration[] | undefined {
    const identifier = expr.expression;
    if (!ts.isIdentifier(identifier)) {
      return undefined;
    }

    const decl: ts.ClassDeclaration | undefined = this.getClassDeclaration(identifier);
    if (!decl) {
      return undefined;
    }

    const classDecls: ts.ClassDeclaration[] = this.getClassHierarchy(decl);
    const filteredClassDecls = classDecls.filter((classDecl) => {
      if (TypeScriptLinter.hasObservedDecorator(classDecl)) {
        return false;
      }
      return true;
    });
    return filteredClassDecls;
  }

  private addFromTypeNode(type: ts.TypeNode): ts.ClassDeclaration[] | undefined {
    const targets: ts.Node[] = [];
    if (ts.isUnionTypeNode(type)) {
      const types = type.types;
      types.forEach((typeNode) => {
        if (ts.isTypeReferenceNode(typeNode)) {
          targets.push(typeNode.typeName);
        }
      });
    } else if (ts.isTypeReferenceNode(type)) {
      targets.push(type.typeName);
    }

    const classDecls: ts.ClassDeclaration[] = [];
    targets.forEach((target) => {
      const decl: ts.ClassDeclaration | undefined = this.getClassDeclaration(target);
      if (!decl) {
        return;
      }

      const decls: ts.ClassDeclaration[] = this.getClassHierarchy(decl);
      classDecls.push(...decls);
    });
    const filteredClassDecls = classDecls.filter((classDecl) => {
      if (TypeScriptLinter.hasObservedDecorator(classDecl)) {
        return false;
      }
      return true;
    });
    return filteredClassDecls;
  }

  private static hasObservedDecorator(classDecl: ts.ClassDeclaration): boolean {
    return (
      ts.getDecorators(classDecl)?.some((decorator) => {
        return decorator.getText() === '@' + CustomDecoratorName.Observed;
      }) ?? false
    );
  }

  private getClassDeclaration(node: ts.Node): ts.ClassDeclaration | undefined {
    const symbol = this.tsTypeChecker.getSymbolAtLocation(node);
    let decl: ts.Declaration | undefined;
    if (symbol) {
      decl = this.tsUtils.getDeclarationNode(node);
      if (decl?.getSourceFile() !== node.getSourceFile()) {
        return undefined;
      }
    }

    if (!decl || !ts.isClassDeclaration(decl)) {
      return undefined;
    }

    return decl;
  }

  private getClassHierarchy(classDecl: ts.ClassDeclaration): ts.ClassDeclaration[] {
    const hierarchy: ts.ClassDeclaration[] = [];
    let currentClass: ts.ClassDeclaration | undefined = classDecl;

    while (currentClass) {
      hierarchy.push(currentClass);
      const heritageClause = currentClass.heritageClauses?.find((clause) => {
        return clause.token === ts.SyntaxKind.ExtendsKeyword;
      });
      const identifier = heritageClause?.types[0]?.expression as ts.Identifier | undefined;
      if (!identifier) {
        break;
      }
      currentClass = this.getClassDeclaration(identifier);
    }

    return hierarchy;
  }

  private checkArkTSObjectInterop(tsCallExpr: ts.CallExpression): void {
    const callSignature = this.tsTypeChecker.getResolvedSignature(tsCallExpr);
    if (!callSignature?.declaration) {
      return;
    }

    if (!this.isDeclaredInArkTs2(callSignature)) {
      return;
    }

    if (!this.hasObjectParameter(callSignature, tsCallExpr)) {
      return;
    }

    const functionSymbol = this.getFunctionSymbol(callSignature.declaration);
    const functionDeclaration = functionSymbol?.valueDeclaration;
    if (!functionDeclaration) {
      return;
    }

    if (
      TypeScriptLinter.isFunctionLike(functionDeclaration) &&
      TypeScriptLinter.containsForbiddenAPI(functionDeclaration)
    ) {
      this.incrementCounters(tsCallExpr.parent, FaultID.InteropCallReflect);
    }
  }

  private hasObjectParameter(callSignature: ts.Signature, tsCallExpr: ts.CallExpression): boolean {
    for (const [index, param] of callSignature.parameters.entries()) {
      const paramType = this.tsTypeChecker.getTypeOfSymbolAtLocation(param, tsCallExpr);

      if (!this.tsUtils.isObject(paramType)) {
        continue;
      }

      const argument = tsCallExpr.arguments[index];
      if (!argument) {
        continue;
      }

      if (this.tsTypeChecker.getTypeAtLocation(argument).isClass()) {
        return true;
      }
    }

    return false;
  }

  private static containsForbiddenAPI(
    node: ts.FunctionDeclaration | ts.MethodDeclaration | ts.FunctionExpression
  ): ForbidenAPICheckResult {
    if (!node.body) {
      return NONE;
    }
    return TypeScriptLinter.isForbiddenUsed(node.body);
  }

  private static isForbiddenUsed(currentNode: ts.Node): ForbidenAPICheckResult {
    if (!ts.isCallExpression(currentNode)) {
      let found: ForbidenAPICheckResult = NONE;
      ts.forEachChild(currentNode, (child) => {
        if (found === NONE) {
          found = TypeScriptLinter.isForbiddenUsed(child);
        }
      });

      return found;
    }

    const expr = currentNode.expression;
    if (!ts.isPropertyAccessExpression(expr)) {
      return NONE;
    }

    const obj = expr.expression;
    const method = expr.name;
    if (!ts.isIdentifier(obj)) {
      return NONE;
    }

    if (obj.text === REFLECT_LITERAL) {
      if (REFLECT_PROPERTIES.includes(method.text)) {
        return REFLECT_LITERAL;
      }
    }

    if (obj.text === OBJECT_LITERAL) {
      if (OBJECT_PROPERTIES.includes(method.text)) {
        return OBJECT_LITERAL;
      }
    }
    return NONE;
  }

  private getFunctionSymbol(declaration: ts.Declaration): ts.Symbol | undefined {
    if (TypeScriptLinter.isFunctionLike(declaration)) {
      return declaration.name ? this.tsTypeChecker.getSymbolAtLocation(declaration.name) : undefined;
    }
    return undefined;
  }

  private static isFunctionLike(
    node: ts.Node
  ): node is ts.FunctionDeclaration | ts.MethodDeclaration | ts.FunctionExpression {
    return ts.isFunctionDeclaration(node) || ts.isMethodDeclaration(node) || ts.isFunctionExpression(node);
  }

  private static isThirdPartyBySymbol(symbol: ts.Symbol | undefined, apiList: ApiListItem): boolean {
    if (!symbol) {
      return false;
    }
    const declaration = symbol.getDeclarations()?.[0];
    if (declaration && ts.isImportClause(declaration)) {
      const importDecl = declaration.parent;
      const importPath = TsUtils.removeOrReplaceQuotes(importDecl.moduleSpecifier.getText(), false);
      const import_path = TypeScriptLinter.getLocalApiListItemByKey(SdkNameInfo.ImportPath, apiList);
      if (import_path.includes(importPath)) {
        return true;
      }
    }
    return false;
  }

  private static getLocalApiListItemByKey(key: string, apiList: ApiListItem): string | string[] {
    if (!apiList) {
      return '';
    }
    if (SdkNameInfo.ImportPath === key) {
      return apiList.import_path;
    }
    return '';
  }

  private handleSdkForConstructorFuncs(node: ts.PropertyAccessExpression | ts.QualifiedName): void {
    if (!this.options.arkts2) {
      return;
    }
    const rightNode = ts.isPropertyAccessExpression(node) ? node.name : node.right;
    const leftNode = ts.isPropertyAccessExpression(node) ? node.expression : node.left;
    const constructorFuncsInfos = Array.from(TypeScriptLinter.constructorFuncsSet);
    constructorFuncsInfos.some((constructorFuncsInfo) => {
      const api_name = constructorFuncsInfo.api_info.api_name;
      if (api_name !== rightNode.getText()) {
        return;
      }
      const parentSym = this.tsTypeChecker.getSymbolAtLocation(leftNode);
      if (TypeScriptLinter.isThirdPartyBySymbol(parentSym, constructorFuncsInfo)) {
        this.incrementCounters(rightNode, FaultID.ConstructorTypesDeprecated);
      }
    });
  }

  private handleQuotedHyphenPropsDeprecated(node: ts.PropertyAccessExpression | ts.PropertyAssignment): void {
    if (!this.options.arkts2 || !node) {
      return;
    }
    const literalAsPropertyNameInfos = Array.from(TypeScriptLinter.literalAsPropertyNameTypeSet);
    literalAsPropertyNameInfos.some((literalAsPropertyNameInfo) => {
      this.localApiListItem = literalAsPropertyNameInfo;
      const api_name = literalAsPropertyNameInfo.api_info.api_name;
      if (api_name !== (ts.isPropertyAccessExpression(node) ? node.name.text : node.name.getText())) {
        return false;
      }
      const parentSym = this.getFinalSymOnQuotedHyphenPropsDeprecated(
        ts.isPropertyAccessExpression(node) ? node.expression : node
      );
      if (parentSym && this.shouldWarn(parentSym)) {
        this.incrementCounters(node, FaultID.QuotedHyphenPropsDeprecated);
        return true;
      }
      return false;
    });
  }

  private shouldWarn(symbol: ts.Symbol): boolean {
    const parentApiName = this.getLocalApiListItemByKey(SdkNameInfo.ParentApiName);
    return symbol && this.isHeritageClauseisThirdPartyBySymbol(symbol) || symbol.name === parentApiName;
  }

  private getFinalSymOnQuotedHyphenPropsDeprecated(node: ts.Node): ts.Symbol | undefined {
    let currentNode = node;
    while (currentNode) {
      const symbol = this.checkNodeTypeOnQuotedHyphenPropsDeprecated(currentNode);
      if (symbol) {
        return symbol;
      }
      currentNode = currentNode.parent;
    }
    return undefined;
  }

  private checkNodeTypeOnQuotedHyphenPropsDeprecated(node: ts.Node): ts.Symbol | undefined {
    if (ts.isVariableDeclaration(node)) {
      return this.getTypeOfVariable(node);
    }

    if (ts.isPropertySignature(node)) {
      return this.tsTypeChecker.getSymbolAtLocation(node);
    }

    const nodesWithResolvableType = [
      ts.isFunctionDeclaration(node) && node.type,
      ts.isMethodDeclaration(node) && node.type,
      ts.isTypeReferenceNode(node) && node,
      ts.isParameter(node) && node.type
    ].filter(Boolean);

    for (const typeNode of nodesWithResolvableType) {
      return typeNode ? this.resolveTypeNodeSymbol(typeNode) : undefined;
    }

    if (ts.isIdentifier(node)) {
      const symbol = this.tsTypeChecker.getSymbolAtLocation(node);
      const declaration = symbol?.getDeclarations()?.[0];
      if (declaration) {
        return this.getFinalSymOnQuotedHyphenPropsDeprecated(declaration);
      }
    }

    return undefined;
  }

  private getTypeOfVariable(variable: ts.VariableDeclaration): ts.Symbol | undefined {
    if (variable.type) {
      return ts.isArrayTypeNode(variable.type) ?
        this.resolveTypeNodeSymbol(variable.type.elementType) :
        this.resolveTypeNodeSymbol(variable.type);
    }
    return variable.initializer ? this.tsTypeChecker.getTypeAtLocation(variable.initializer).getSymbol() : undefined;
  }

  private resolveTypeNodeSymbol(typeNode: ts.TypeNode): ts.Symbol | undefined {
    if (!ts.isTypeReferenceNode(typeNode)) {
      return undefined;
    }
    return this.resolveTypeNoSymbol(typeNode);
  }

  private resolveTypeNoSymbol(typeNode: ts.TypeReferenceNode): ts.Symbol | undefined {
    if (!typeNode.typeName) {
      return undefined;
    }

    if (ts.isQualifiedName(typeNode.typeName)) {
      return this.tsTypeChecker.getSymbolAtLocation(typeNode.typeName.right);
    }

    const symbol = this.tsUtils.trueSymbolAtLocation(typeNode.typeName);
    if (symbol?.declarations && symbol.declarations.length > 0) {
      const globalDeclaration = symbol.declarations[0];
      if (ts.isTypeAliasDeclaration(globalDeclaration)) {
        return this.resolveTypeNodeSymbol(globalDeclaration.type);
      } else if (ts.isInterfaceDeclaration(globalDeclaration)) {
        return this.processQuotedHyphenPropsDeprecatedOnInterfaceDeclaration(globalDeclaration);
      }
    }
    return this.tsTypeChecker.getTypeAtLocation(typeNode).getSymbol();
  }

  private isHeritageClauseisThirdPartyBySymbol(symbol: ts.Symbol): boolean {
    const declarations = symbol.getDeclarations();
    if (declarations && declarations.length > 0) {
      const firstDeclaration = declarations[0];
      if (ts.isImportSpecifier(firstDeclaration)) {
        const importDecl = firstDeclaration.parent.parent.parent;
        const importPath = importDecl.moduleSpecifier.getText();
        const import_path = this.getLocalApiListItemByKey(SdkNameInfo.ImportPath);
        if (import_path && JSON.stringify(import_path).includes(importPath)) {
          return true;
        }
      }
    }
    return false;
  }

  private getLocalApiListItemByKey(key: string): string | string[] {
    if (!this.localApiListItem) {
      return '';
    }
    if (SdkNameInfo.ParentApiName === key) {
      return this.localApiListItem.api_info.parent_api[0].api_name;
    } else if (SdkNameInfo.ImportPath === key) {
      return this.localApiListItem.import_path;
    }
    return '';
  }

  private processQuotedHyphenPropsDeprecatedOnInterfaceDeclaration(
    node: ts.InterfaceDeclaration
  ): ts.Symbol | undefined {
    const heritageSymbol = this.processHeritageClauses(node);
    if (heritageSymbol) {
      return heritageSymbol;
    }
    return this.processMembers(node);
  }

  private processHeritageClauses(node: ts.InterfaceDeclaration): ts.Symbol | undefined {
    if (!node.heritageClauses) {
      return undefined;
    }
    for (const heritageClause of node.heritageClauses) {
      return this.processHeritageClause(heritageClause);
    }

    return undefined;
  }

  private processHeritageClause(heritageClause: ts.HeritageClause): ts.Symbol | undefined {
    for (const type of heritageClause.types) {
      if (!type.expression) {
        return undefined;
      }
      if (ts.isPropertyAccessExpression(type.expression)) {
        return this.processPropertyAccessExpression(type.expression);
      }
    }
    return undefined;
  }

  private processPropertyAccessExpression(expression: ts.PropertyAccessExpression): ts.Symbol | undefined {
    const heritageSymbol = this.tsTypeChecker.getSymbolAtLocation(expression.expression);
    if (heritageSymbol && expression.name.text === this.getLocalApiListItemByKey(SdkNameInfo.ParentApiName)) {
      return heritageSymbol;
    }
    return undefined;
  }

  private processMembers(node: ts.InterfaceDeclaration): ts.Symbol | undefined {
    if (!node.members) {
      return undefined;
    }
    for (const member of node.members) {
      if (ts.isPropertySignature(member) && member.type) {
        return this.resolveTypeNodeSymbol(member.type);
      }
    }
    return undefined;
  }

  private processApiNodeSdkGlobalApi(apiName: string, errorNode: ts.Node): void {
    for (const [key, value] of globalApiAssociatedInfo) {
      this.doProcessApiNodeSdkGlobalApi(apiName, errorNode, key, value);
    }
  }

  private doProcessApiNodeSdkGlobalApi(apiName: string, errorNode: ts.Node, key: string, faultId: number): void {
    const setApiListItem = TypeScriptLinter.globalApiInfo.get(key);
    if (!setApiListItem) {
      return;
    }
    const apiNamesArr = [...setApiListItem];
    const hasSameApiName = apiNamesArr.some((apilistItem) => {
      return apilistItem.api_info.api_name === errorNode.getText();
    });
    if (!hasSameApiName) {
      return;
    }
    if (ts.isTypeReferenceNode(errorNode)) {
      errorNode = errorNode.typeName;
    }
    const matchedApi = apiNamesArr.some((sdkInfo) => {
      const isSameName = sdkInfo.api_info.api_name === apiName;
      const isGlobal = sdkInfo.is_global;
      return isSameName && isGlobal;
    });
    const checkSymbol = this.isIdentifierFromSDK(errorNode);
    const type = this.tsTypeChecker.getTypeAtLocation(errorNode);
    const typeName = this.tsTypeChecker.typeToString(type);

    if (checkSymbol) {
      if (arkTsBuiltInTypeName.has(typeName)) {
        return;
      }
      if (matchedApi) {
        this.incrementCounters(errorNode, faultId);
      }
    } 
  }

  private isIdentifierFromSDK(node: ts.Node): boolean {
    const symbol = this.tsTypeChecker.getSymbolAtLocation(node);
    if (!symbol) {
      return true;
    }

    // Check if the symbol is from an SDK import
    const declarations = symbol.getDeclarations();
    if (!declarations || declarations.length === 0) {
      return true;
    }

    let isLocal = false;
    for (const declaration of declarations) {
      if (ts.isVariableDeclaration(declaration) ||
          ts.isTypeAliasDeclaration(declaration) ||
          ts.isClassDeclaration(declaration) ||
          ts.isInterfaceDeclaration(declaration) ||
          ts.isFunctionDeclaration(declaration) ||
          ts.isEnumDeclaration(declaration)) {
        isLocal = true;
        break
      }
    }

    if(isLocal) {
      return false;
    }

    return true;
  }

  private handleSdkGlobalApi(
    node:
      | ts.TypeReferenceNode
      | ts.NewExpression
      | ts.VariableDeclaration
      | ts.PropertyDeclaration
      | ts.ParameterDeclaration
      | ts.CallExpression
      | ts.BinaryExpression
      | ts.ExpressionWithTypeArguments
      | ts.Identifier
      | ts.MethodDeclaration
  ): void {
    if (!this.options.arkts2) {
      return;
    }
    switch (node.kind) {
      case ts.SyntaxKind.TypeReference:
        this.checkTypeReferenceForSdkGlobalApi(node);
        break;
      case ts.SyntaxKind.NewExpression:
        this.checkNewExpressionForSdkGlobalApi(node);
        break;
      case ts.SyntaxKind.Identifier:
        this.checkHeritageClauseForSdkGlobalApi(node);
        break;
      case ts.SyntaxKind.VariableDeclaration:
      case ts.SyntaxKind.PropertyDeclaration:
      case ts.SyntaxKind.Parameter:
        this.checkDeclarationForSdkGlobalApi(node);
        break;
      case ts.SyntaxKind.CallExpression:
        this.checkCallExpressionForSdkGlobalApi(node);
        break;
      case ts.SyntaxKind.BinaryExpression:
        this.checkBinaryExpressionForSdkGlobalApi(node);
        break;
      case ts.SyntaxKind.MethodDeclaration:
        this.checkMethodDeclarationForSdkGlobalApi(node);
        break;
      default:
    }
  }

  private checkTypeReferenceForSdkGlobalApi(node: ts.TypeReferenceNode): void {
    const typeName = node.typeName;
    if (ts.isIdentifier(typeName)) {
      this.processApiNodeSdkGlobalApi(typeName.text, node);
    }
  }

  private checkNewExpressionForSdkGlobalApi(node: ts.NewExpression): void {
    const expression = node.expression;
    if (ts.isIdentifier(expression)) {
      this.processApiNodeSdkGlobalApi(expression.text, expression);
    }
  }

  private checkHeritageClauseForSdkGlobalApi(node: ts.Identifier): void {
    if (ts.isIdentifier(node)) {
      this.processApiNodeSdkGlobalApi(node.text, node);
    }
  }

  private checkDeclarationForSdkGlobalApi(
    node: ts.VariableDeclaration | ts.PropertyDeclaration | ts.ParameterDeclaration
  ): void {
    const expression = node.initializer;
    if (expression && ts.isIdentifier(expression)) {
      this.processApiNodeSdkGlobalApi(expression.text, expression);
    }
  }

  private checkCallExpressionForSdkGlobalApi(node: ts.CallExpression): void {
    if (ts.isPropertyAccessExpression(node.expression) && ts.isIdentifier(node.expression.expression)) {
      const expression = node.expression.expression;
      
      this.processApiNodeSdkGlobalApi(expression.text, expression);
    }
  }

  private checkBinaryExpressionForSdkGlobalApi(node: ts.BinaryExpression): void {
    const expression = node.right;
    if (ts.isIdentifier(expression)) {
      this.processApiNodeSdkGlobalApi(expression.text, expression);
    }
  }

  private checkMethodDeclarationForSdkGlobalApi(node: ts.MethodDeclaration): void {
    const expression = node.name;
    if (ts.isIdentifier(expression)) {
      this.processApiNodeSdkGlobalApi(expression.text, expression);
    }
  }

  private checkEWTArgumentsForSdkDuplicateDeclName(node: ts.HeritageClause): void {
    if (!this.options.arkts2) {
      return;
    }
    if (node.token === ts.SyntaxKind.ExtendsKeyword || node.token === ts.SyntaxKind.ImplementsKeyword) {
      node.types.forEach((type) => {
        this.handleSharedArrayBuffer(type);
        const expr = type.expression;
        if (ts.isIdentifier(expr)) {
          this.processApiNodeSdkGlobalApi(expr.text, expr);
        }
      });
    }
  }

  private getOriginalSymbol(node: ts.Node): ts.Symbol | undefined {
    if (ts.isIdentifier(node)) {
      const variableDeclaration = this.findVariableDeclaration(node);
      if (variableDeclaration?.initializer) {
        return this.getOriginalSymbol(variableDeclaration.initializer);
      }
    } else if (ts.isNewExpression(node)) {
      const constructor = node.expression;
      if (ts.isIdentifier(constructor)) {
        return this.tsUtils.trueSymbolAtLocation(constructor);
      }
    } else if (ts.isCallExpression(node)) {
      const callee = node.expression;
      if (ts.isIdentifier(callee)) {
        return this.tsUtils.trueSymbolAtLocation(callee);
      } else if (ts.isPropertyAccessExpression(callee)) {
        return this.getOriginalSymbol(callee.expression);
      }
    } else if (ts.isPropertyAccessExpression(node)) {
      return this.getOriginalSymbol(node.expression);
    }
    return this.tsUtils.trueSymbolAtLocation(node);
  }

  private static isFromJsImport(symbol: ts.Symbol): boolean {
    const declaration = symbol.declarations?.[0];
    if (declaration) {
      const sourceFile = declaration.getSourceFile();
      return sourceFile.fileName.endsWith(EXTNAME_JS);
    }
    return false;
  }

  private hasLocalAssignment(node: ts.Node): boolean {
    if (ts.isIdentifier(node)) {
      const variableDeclaration = this.findVariableDeclaration(node);
      return !!variableDeclaration?.initializer;
    }
    return false;
  }

  private isLocalCall(node: ts.Node): boolean {
    if (ts.isCallExpression(node)) {
      const callee = node.expression;
      if (ts.isIdentifier(callee)) {
        return this.hasLocalAssignment(callee);
      } else if (ts.isPropertyAccessExpression(callee)) {
        const objectNode = callee.expression;
        return this.hasLocalAssignment(objectNode);
      }
    }
    return false;
  }

  private handleInterOpImportJsOnTypeOfNode(typeofExpress: ts.TypeOfExpression): void {
    if (!this.options.arkts2 || !typeofExpress || !this.useStatic) {
      return;
    }
    const targetNode = typeofExpress.expression;
    if (this.hasLocalAssignment(targetNode) || this.isLocalCall(targetNode)) {
      return;
    }
    const targetSymbol = this.getOriginalSymbol(targetNode);
    if (targetSymbol && TypeScriptLinter.isFromJsImport(targetSymbol)) {
      const autofix = this.autofixer?.fixInterOpImportJsOnTypeOf(typeofExpress);
      this.incrementCounters(typeofExpress, FaultID.InterOpImportJsForTypeOf, autofix);
    }
  }

  private handleSdkTypeQuery(decl: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2 || !ts.isPropertyAccessExpression(decl)) {
      return;
    }

    if (this.handleSelfPropertyAccess(decl)) {
      return;
    }

    if (ts.isPropertyAccessExpression(decl)) {
      const deprecatedProperties = [
        'position',
        'subtype',
        'movingPhotoEffectMode',
        'dynamicRangeType',
        'thumbnailVisible'
      ];

      const propertyName = ts.isIdentifier(decl.name) ? decl.name.text : '';
      if (deprecatedProperties.includes(propertyName)) {
        this.incrementCounters(decl.name, FaultID.SdkTypeQuery);
        return;
      }
    }

    this.handleImportApiPropertyAccess(decl);
  }

  private handleSelfPropertyAccess(decl: ts.PropertyAccessExpression): boolean {
    if (!ts.isPropertyAccessExpression(decl.expression)) {
      return false;
    }

    const propertyName = ts.isIdentifier(decl.expression.name) && decl.expression.name.text || '';
    if (propertyName !== 'self') {
      return false;
    }

    this.incrementCounters(decl.name, FaultID.SdkTypeQuery);
    return true;
  }

  private handleImportApiPropertyAccess(decl: ts.PropertyAccessExpression): void {
    if (!ts.isPropertyAccessExpression(decl.expression)) {
      return;
    }

    const importApiName = ts.isIdentifier(decl.expression.expression) && decl.expression.expression.text || '';
    const sdkInfos = importApiName && this.interfaceMap.get(importApiName);
    if (!sdkInfos) {
      return;
    }

    const apiName = ts.isIdentifier(decl.name) && decl.name.text || '';
    const matchedApi = [...sdkInfos].find((sdkInfo) => {
      return sdkInfo.api_name === apiName;
    });

    if (matchedApi) {
      this.incrementCounters(decl.name, FaultID.SdkTypeQuery);
    }
  }

  /**
   * Returns true if the method’s declared return type or body returns Promise<void>.
   */
  private hasPromiseVoidReturn(method: ts.MethodDeclaration): boolean {
    return (
      this.hasAnnotatedPromiseVoidReturn(method) || this.isAsyncMethod(method) || this.hasBodyPromiseReturn(method)
    );
  }

  /**
   * Checks if the method’s declared return type annotation includes Promise<void>.
   */
  private hasAnnotatedPromiseVoidReturn(method: ts.MethodDeclaration): boolean {
    void this;
    if (!method.type) {
      return false;
    }
    const t = method.type;
    // Union type check
    if (ts.isUnionTypeNode(t)) {
      return t.types.some((u) => {
        return this.isSinglePromiseVoid(u);
      });
    }
    // Single Promise<void> check
    return this.isSinglePromiseVoid(t);
  }

  private isSinglePromiseVoid(n: ts.Node): boolean {
    void this;
    return ts.isTypeReferenceNode(n) && n.typeName.getText() === PROMISE && n.typeArguments?.[0]?.getText() === VOID;
  }

  /**
   * Checks if the method is declared async (implying Promise return).
   */
  private isAsyncMethod(method: ts.MethodDeclaration): boolean {
    void this;
    return (
      method.modifiers?.some((m) => {
        return m.kind === ts.SyntaxKind.AsyncKeyword;
      }) ?? false
    );
  }

  /**
   * Scans the method body iteratively for any Promise-returning statements.
   */
  private hasBodyPromiseReturn(method: ts.MethodDeclaration): boolean {
    if (!method.body) {
      return false;
    }

    let found = false;
    const visit = (node: ts.Node): void => {
      if (ts.isReturnStatement(node) && node.expression) {
        const retType = this.tsTypeChecker.getTypeAtLocation(node.expression);
        if (retType.symbol?.getName() === PROMISE) {
          found = true;
          return;
        }
      }
      ts.forEachChild(node, visit);
    };
    ts.forEachChild(method.body, visit);

    return found;
  }

  /**
   * Returns true if this method name is onDestroy/onDisconnect and class extends one of the supported Ability subclasses.
   */
  private isLifecycleMethodOnAbilitySubclass(method: ts.MethodDeclaration): boolean {
    const name = method.name.getText();
    if (name !== ON_DESTROY && name !== ON_DISCONNECT) {
      return false;
    }
    const cls = method.parent;
    if (!ts.isClassDeclaration(cls) || !cls.heritageClauses) {
      return false;
    }
    return cls.heritageClauses.some((h) => {
      return (
        h.token === ts.SyntaxKind.ExtendsKeyword &&
        h.types.some((tn) => {
          return this.isSupportedAbilityBase(method.name.getText(), tn.expression);
        })
      );
    });
  }

  /**
   * Checks that the base class name and its import source or declaration file are supported,
   * and matches the lifecycle method (onDestroy vs onDisconnect).
   */
  private isSupportedAbilityBase(methodName: string, baseExprNode: ts.Expression): boolean {
    const sym = this.tsTypeChecker.getSymbolAtLocation(baseExprNode);
    if (!sym) {
      return false;
    }

    const baseName = sym.getName();
    if (!ASYNC_LIFECYCLE_SDK_LIST.has(baseName)) {
      return false;
    }

    if (methodName === ON_DISCONNECT && baseName !== SERVICE_EXTENSION_ABILITY) {
      return false;
    }
    if (methodName === ON_DESTROY && baseName === SERVICE_EXTENSION_ABILITY) {
      return false;
    }

    const decl = sym.getDeclarations()?.[0];
    if (!decl || !ts.isImportSpecifier(decl)) {
      return false;
    }

    const importDecl = decl.parent.parent.parent;
    const moduleName = (importDecl.moduleSpecifier as ts.StringLiteral).text;
    const srcFile = decl.getSourceFile().fileName;

    return moduleName === ABILITY_KIT || srcFile.endsWith(`${baseName}.${EXTNAME_D_TS}`);
  }

  /**
   * Rule sdk-void-lifecycle-return:
   * Flags onDestroy/onDisconnect methods in Ability subclasses
   * whose return type includes Promise<void>.
   */
  private checkVoidLifecycleReturn(method: ts.MethodDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!this.isLifecycleMethodOnAbilitySubclass(method)) {
      return;
    }

    if (!this.hasPromiseVoidReturn(method)) {
      return;
    }

    this.incrementCounters(method.name, FaultID.SdkAbilityAsynchronousLifecycle);
  }

  private handleGetOwnPropertyNames(decl: ts.PropertyAccessExpression): void {
    if (this.checkPropertyAccessExpression(decl, GET_OWN_PROPERTY_NAMES_TEXT, TypeScriptLinter.missingAttributeSet)) {
      const autofix = this.autofixer?.fixMissingAttribute(decl);
      this.incrementCounters(decl, FaultID.BuiltinGetOwnPropertyNames, autofix);
    }
  }

  private handleSymbolIterator(decl: ts.PropertyAccessExpression): void {
    if (this.checkPropertyAccessExpression(decl, SYMBOL_ITERATOR, TypeScriptLinter.symbotIterSet)) {
      this.incrementCounters(decl, FaultID.BuiltinSymbolIterator);
    }
  }

  private checkPropertyAccessExpression(decl: ts.PropertyAccessExpression, name: string, set: Set<string>): boolean {
    if (set.size === 0 || decl.getText() !== name) {
      return false;
    }

    const symbol = this.tsUtils.trueSymbolAtLocation(decl);
    const sourceFile = symbol?.declarations?.[0]?.getSourceFile();
    if (!sourceFile) {
      return false;
    }

    const fileName = path.basename(sourceFile.fileName);
    return set.has(fileName);
  }

  private fixJsImportCallExpression(callExpr: ts.CallExpression): void {
    if (
      !this.options.arkts2 ||
      !this.useStatic ||
      ts.isAwaitExpression(callExpr.parent) ||
      ts.isTypeOfExpression(callExpr.parent)
    ) {
      return;
    }

    const identifier = this.tsUtils.findIdentifierInExpression(callExpr);
    if (!identifier) {
      return;
    }

    if (!this.tsUtils.isImportedFromJS(identifier)) {
      return;
    }

    callExpr.arguments.forEach((arg) => {
      const type = this.tsTypeChecker.getTypeAtLocation(arg);
      if (ts.isArrowFunction(arg)) {
        this.incrementCounters(arg, FaultID.InteropJsObjectCallStaticFunc);
      } else if (ts.isIdentifier(arg)) {
        const sym = this.tsTypeChecker.getSymbolAtLocation(arg);
        const decl = sym?.declarations?.[0];
        if (
          decl &&
          (ts.isFunctionDeclaration(decl) ||
            ts.isVariableDeclaration(decl) && decl.initializer && ts.isArrowFunction(decl.initializer))
        ) {
          this.incrementCounters(arg, FaultID.InteropJsObjectCallStaticFunc);
        }
        if (type?.isClassOrInterface()) {
          this.incrementCounters(arg, FaultID.InteropJsObjectExpandStaticInstance);
        }
      } else if (ts.isObjectLiteralExpression(arg) || type?.isClassOrInterface()) {
        this.incrementCounters(arg, FaultID.InteropJsObjectExpandStaticInstance);
      }
    });
  }

  private fixJsImportExtendsClass(
    node: ts.ClassLikeDeclaration | ts.InterfaceDeclaration,
    identifier: ts.Identifier
  ): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!this.tsUtils.isImportedFromJS(identifier)) {
      return;
    }

    const className = node.name?.text;
    if (!className) {
      return;
    }
    this.incrementCounters(node, FaultID.InteropJsObjectInheritance);
  }

  private fixJsImportPropertyAccessExpression(node: ts.Node): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    const identifier = this.tsUtils.findIdentifierInExpression(node);
    if (!identifier) {
      return;
    }

    // Try direct check first
    if (!this.tsUtils.isImportedFromJS(identifier)) {
      return;
    }
    const autofix = this.autofixer?.createReplacementForJsImportPropertyAccessExpression(
      node as ts.PropertyAccessExpression
    );
    if (!TsUtils.isInsideIfCondition(node)) {
      return;
    }
    this.incrementCounters(node, FaultID.InteropJsObjectConditionJudgment, autofix);
  }

  private fixJsImportElementAccessExpression(elementAccessExpr: ts.ElementAccessExpression): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }
    if (!TypeScriptLinter.isInForLoopBody(elementAccessExpr)) {
      return;
    }
    const variableDeclaration = ts.isIdentifier(elementAccessExpr.expression) ?
      this.tsUtils.findVariableDeclaration(elementAccessExpr.expression) :
      undefined;
    if (!variableDeclaration?.initializer) {
      return;
    }

    const identifier = ts.isPropertyAccessExpression(variableDeclaration.initializer) ?
      (variableDeclaration.initializer.expression as ts.Identifier) :
      undefined;
    if (!identifier) {
      return;
    }

    if (!this.tsUtils.isImportedFromJS(identifier)) {
      return;
    }

    const autofix = this.autofixer?.fixJsImportElementAccessExpression(elementAccessExpr);

    this.incrementCounters(elementAccessExpr, FaultID.InteropJsObjectTraverseJsInstance, autofix);
  }

  private static isInForLoopBody(node: ts.Node): boolean {
    let current: ts.Node | undefined = node.parent;
    while (current) {
      if (ts.isForStatement(current) || ts.isForInStatement(current) || ts.isForOfStatement(current)) {
        return true;
      }
      current = current.parent;
    }
    return false;
  }

  private handleTaskPoolDeprecatedUsages(propertyAccess: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }
    const objectExpr = ts.isNewExpression(propertyAccess.expression) ?
      propertyAccess.expression.expression :
      propertyAccess.expression;
    // Step 1: Must be either setCloneList or setTransferList
    if (!TypeScriptLinter.isDeprecatedTaskPoolMethodCall(propertyAccess)) {
      return;
    }
    const variableDecl = TsUtils.getDeclaration(this.tsUtils.trueSymbolAtLocation(objectExpr));
    const isNoContinue =
      !variableDecl ||
      !ts.isVariableDeclaration(variableDecl) ||
      !variableDecl?.initializer ||
      !ts.isNewExpression(variableDecl.initializer);
    if (isNoContinue) {
      return;
    }
    const taskpoolExpr = variableDecl.initializer.expression;
    if (!this.isTaskPoolTaskCreation(taskpoolExpr)) {
      return;
    }
    const faultId =
      propertyAccess.name.text === DEPRECATED_TASKPOOL_METHOD_SETCLONELIST ?
        FaultID.SetCloneListDeprecated :
        FaultID.SetTransferListDeprecated;
    this.incrementCounters(propertyAccess.name, faultId);
  }

  private static isDeprecatedTaskPoolMethodCall(propertyAccess: ts.PropertyAccessExpression): boolean {
    const methodName = propertyAccess.name.text;
    return (
      methodName === DEPRECATED_TASKPOOL_METHOD_SETCLONELIST ||
      methodName === DEPRECATED_TASKPOOL_METHOD_SETTRANSFERLIST
    );
  }

  private isTaskPoolTaskCreation(taskpoolExpr: ts.Expression): boolean {
    if (
      ts.isIdentifier(taskpoolExpr) ||
      ts.isPropertyAccessExpression(taskpoolExpr) && taskpoolExpr.name.text === STDLIB_TASK_CLASS_NAME
    ) {
      const objectExpr = ts.isIdentifier(taskpoolExpr) ? taskpoolExpr : taskpoolExpr.expression;
      return this.isTaskPoolReferenceisTaskPoolImportForTaskPoolDeprecatedUsages(objectExpr);
    }
    return false;
  }

  private isTaskPoolReferenceisTaskPoolImportForTaskPoolDeprecatedUsages(expr: ts.Expression): boolean {
    if (ts.isIdentifier(expr)) {
      const sym = this.tsTypeChecker.getSymbolAtLocation(expr);
      const importChild = TsUtils.getDeclaration(sym);
      if (!importChild) {
        return false;
      }
      if (ts.isImportSpecifier(importChild)) {
        return TypeScriptLinter.isTaskPoolImportForTaskPoolDeprecatedUsages(importChild);
      }
      if (ts.isImportClause(importChild) && importChild.name?.text === STDLIB_TASKPOOL_OBJECT_NAME) {
        return TypeScriptLinter.checkModuleSpecifierForTaskPoolDeprecatedUsages(importChild.parent);
      }
    }
    if (ts.isPropertyAccessExpression(expr)) {
      return this.isTaskPoolReferenceOnPropertyAccessExpression(expr);
    }
    return false;
  }

  private static checkModuleSpecifierForTaskPoolDeprecatedUsages(importDecl: ts.ImportDeclaration): boolean {
    if (ts.isImportDeclaration(importDecl) && ts.isStringLiteral(importDecl.moduleSpecifier)) {
      const moduleSpecifier = importDecl.moduleSpecifier;
      return TASKPOOL_MODULES.includes(TsUtils.removeOrReplaceQuotes(moduleSpecifier.getText(), false));
    }
    return false;
  }

  private isTaskPoolReferenceOnPropertyAccessExpression(expr: ts.PropertyAccessExpression): boolean {
    if (expr.name.text !== STDLIB_TASKPOOL_OBJECT_NAME || !ts.isIdentifier(expr.expression)) {
      return false;
    }
    const sym = this.tsTypeChecker.getSymbolAtLocation(expr.expression);
    const importChild = TsUtils.getDeclaration(sym);
    if (importChild && ts.isNamespaceImport(importChild)) {
      return TypeScriptLinter.checkModuleSpecifierForTaskPoolDeprecatedUsages(importChild.parent.parent);
    }
    return false;
  }

  private static isTaskPoolImportForTaskPoolDeprecatedUsages(specifier: ts.ImportSpecifier): boolean {
    const specifierName = specifier.propertyName ? specifier.propertyName : specifier.name;
    if (STDLIB_TASKPOOL_OBJECT_NAME !== specifierName.text) {
      return false;
    }
    const importDeclaration = specifier.parent.parent.parent;
    return TypeScriptLinter.checkModuleSpecifierForTaskPoolDeprecatedUsages(importDeclaration);
  }

  private handleForOfJsArray(node: ts.ForOfStatement): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    const expr = node.expression;
    if (!ts.isIdentifier(expr) || !this.tsUtils.isPossiblyImportedFromJS(expr)) {
      return;
    }

    const exprType = this.tsTypeChecker.getTypeAtLocation(expr);

    if (!this.tsUtils.isArray(exprType)) {
      return;
    }

    this.incrementCounters(node, FaultID.InteropJsObjectTraverseJsInstance);
  }

  private checkStdLibConcurrencyImport(importDeclaration: ts.ImportDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const importClause = importDeclaration.importClause;
    if (!importClause) {
      return;
    }

    const moduleName = (importDeclaration.moduleSpecifier as ts.StringLiteral).text;
    const expectedImports = MODULE_IMPORTS[moduleName];
    if (!expectedImports) {
      return;
    }

    const defaultImport = importClause.name;
    const namedBindings = importClause.namedBindings;

    const namedImports = namedBindings && ts.isNamedImports(namedBindings) ? namedBindings.elements : [];

    const defaultIsForbidden = defaultImport && expectedImports.includes(defaultImport.getText());
    const forbiddenNamed = namedImports.filter((spec) => {
      const name = spec.propertyName ? spec.propertyName.getText() : spec.name.getText();
      return expectedImports.includes(name);
    });

    if (
      TypeScriptLinter.shouldRemoveWholeImport(
        defaultIsForbidden,
        forbiddenNamed.length,
        namedImports.length,
        defaultImport
      )
    ) {
      const autofix = this.autofixer?.removeNode(importDeclaration);
      this.incrementCounters(importDeclaration, FaultID.LimitedStdLibNoImportConcurrency, autofix);
      return;
    }

    if (defaultIsForbidden) {
      const autofix = this.autofixer?.removeDefaultImport(importDeclaration, defaultImport);
      this.incrementCounters(defaultImport, FaultID.LimitedStdLibNoImportConcurrency, autofix);
    }

    for (const spec of forbiddenNamed) {
      const autofix = this.autofixer?.removeImportSpecifier(spec, importDeclaration);
      this.incrementCounters(spec, FaultID.LimitedStdLibNoImportConcurrency, autofix);
    }
  }

  private static shouldRemoveWholeImport(
    defaultIsForbidden: boolean | undefined,
    forbiddenNamedCount: number,
    namedImportsCount: number,
    defaultImport: ts.Identifier | undefined
  ): boolean {
    return (
      defaultIsForbidden && forbiddenNamedCount === namedImportsCount ||
      defaultIsForbidden && namedImportsCount === 0 ||
      !defaultImport && forbiddenNamedCount === namedImportsCount && namedImportsCount > 0
    );
  }

  /**
   * Checks for missing super() call in child classes that extend a parent class
   * with parameterized constructors. If parent class only has parameterized constructors
   * and the child class does not call super() in its constructor, report a fault.
   *
   * This ensures safe and correct subclassing behavior.
   *
   * @param node The HeritageClause node (extends clause) to analyze.
   */
  private handleMissingSuperCallInExtendedClass(node: ts.HeritageClause): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    // We are only interested in 'extends' clauses
    if (node.token !== ts.SyntaxKind.ExtendsKeyword) {
      return;
    }

    // Get the parent class declaration (what the child class extends)
    const parentClass = this.getParentClassDeclaration(node);
    if (!parentClass) {
      return;
    }

    // If parent class has a parameterless constructor (or no constructor at all), child is fine
    if (TypeScriptLinter.parentHasParameterlessConstructor(parentClass)) {
      return;
    }

    // The child class node (the one extending)
    const childClass = node.parent;
    if (!ts.isClassDeclaration(childClass)) {
      return;
    }

    // Look for child class constructor
    const childConstructor = childClass.members.find(ts.isConstructorDeclaration);

    /*
     * If child has no constructor → error (super() cannot be called)
     * If child constructor exists but does not contain super() → error
     */
    if (!childConstructor?.body || !TypeScriptLinter.childHasSuperCall(childConstructor)) {
      this.incrementCounters(node, FaultID.MissingSuperCall);
    }
  }

  /**
   * Retrieves the parent class declaration node from an extends heritage clause.
   */
  private getParentClassDeclaration(node: ts.HeritageClause): ts.ClassDeclaration | undefined {
    const parentExpr = node.types[0]?.expression;
    if (!parentExpr) {
      return undefined;
    }
    const parentSymbol = this.tsUtils.trueSymbolAtLocation(parentExpr);
    return parentSymbol?.declarations?.find(ts.isClassDeclaration);
  }

  /**
   * Determines if a parent class has a parameterless constructor.
   * If it has no constructor at all, that counts as parameterless.
   */
  private static parentHasParameterlessConstructor(parentClass: ts.ClassDeclaration): boolean {
    const constructors = parentClass.members.filter(ts.isConstructorDeclaration);
    return (
      constructors.length === 0 ||
      constructors.some((ctor) => {
        return ctor.parameters.length === 0;
      })
    );
  }

  private static childHasSuperCall(constructor: ts.ConstructorDeclaration): boolean {
    let superCalled = false;

    if (!constructor.body) {
      return false;
    }

    ts.forEachChild(constructor.body, (stmt) => {
      if (
        ts.isExpressionStatement(stmt) &&
        ts.isCallExpression(stmt.expression) &&
        stmt.expression.expression.kind === ts.SyntaxKind.SuperKeyword
      ) {
        superCalled = true;
      }
    });
    return superCalled;
  }

  private handleInterOpImportJs(importDecl: ts.ImportDeclaration): void {
    if (!this.options.arkts2 || !importDecl || !this.useStatic) {
      return;
    }
    const importClause = importDecl.importClause;
    if (!importClause) {
      return;
    }
    const namedBindings = importClause.namedBindings;
    let symbol: ts.Symbol | undefined;
    let defaultSymbol: ts.Symbol | undefined;
    if (importClause.name) {
      defaultSymbol = this.tsUtils.trueSymbolAtLocation(importClause.name);
    }
    if (namedBindings) {
      if (ts.isNamedImports(namedBindings) && namedBindings.elements?.length > 0 && namedBindings.elements[0]?.name) {
        symbol = this.tsUtils.trueSymbolAtLocation(namedBindings.elements[0].name);
      } else if (ts.isNamespaceImport(namedBindings)) {
        symbol = this.tsUtils.trueSymbolAtLocation(namedBindings.name);
      }
    }
    const symbolToUse = defaultSymbol || symbol;
    if (symbolToUse) {
      this.tryAutoFixInterOpImportJs(importDecl, importClause, symbolToUse, defaultSymbol);
    }
  }

  private tryAutoFixInterOpImportJs(
    importDecl: ts.ImportDeclaration,
    importClause: ts.ImportClause,
    symbolToUse: ts.Symbol,
    defaultSymbol?: ts.Symbol
  ): void {
    const declaration = symbolToUse.declarations?.[0];
    if (declaration) {
      const sourceFile = declaration.getSourceFile();
      if (sourceFile.fileName.endsWith(EXTNAME_JS)) {
        const autofix = this.autofixer?.fixInterOpImportJs(
          importDecl,
          importClause,
          TsUtils.removeOrReplaceQuotes(importDecl.moduleSpecifier.getText(this.sourceFile), false),
          defaultSymbol
        );
        this.incrementCounters(importDecl, FaultID.InterOpImportJs, autofix);
      }
    }
  }

  private findVariableDeclaration(identifier: ts.Identifier): ts.VariableDeclaration | undefined {
    const sym = this.tsUtils.trueSymbolAtLocation(identifier);
    const decl = TsUtils.getDeclaration(sym);
    if (
      decl &&
      ts.isVariableDeclaration(decl) &&
      decl.getSourceFile().fileName === identifier.getSourceFile().fileName
    ) {
      return decl;
    }
    return undefined;
  }

  private isFromJSModule(node: ts.Node): boolean {
    const symbol = this.tsUtils.trueSymbolAtLocation(node);
    if (symbol?.declarations?.[0]) {
      const sourceFile = symbol.declarations[0].getSourceFile();
      return sourceFile.fileName.endsWith(EXTNAME_JS);
    }
    return false;
  }

  handleInstanceOfExpression(node: ts.BinaryExpression): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    const left = node.left;
    const right = node.right;
    const getNode = (expr: ts.Expression): ts.Node => {
      return ts.isPropertyAccessExpression(expr) || ts.isCallExpression(expr) ? expr.expression : expr;
    };

    const leftExpr = getNode(left);
    const rightExpr = getNode(right);

    if (!this.tsUtils.isJsImport(leftExpr) && !this.tsUtils.isJsImport(rightExpr)) {
      return;
    }

    const autofix = this.autofixer?.fixInteropJsInstanceOfExpression(node);
    this.incrementCounters(node, FaultID.InteropJsInstanceof, autofix);
  }

  private checkAutoIncrementDecrement(unaryExpr: ts.PostfixUnaryExpression | ts.PrefixUnaryExpression): void {
    if (!this.useStatic || !this.options.arkts2) {
      return;
    }

    if (!ts.isPropertyAccessExpression(unaryExpr.operand)) {
      return;
    }

    const propertyAccess = unaryExpr.operand;
    if (!this.tsUtils.isJsImport(propertyAccess.expression)) {
      return;
    }

    const autofix = this.autofixer?.fixUnaryIncrDecr(unaryExpr, propertyAccess);

    this.incrementCounters(unaryExpr, FaultID.InteropIncrementDecrement, autofix);
  }

  private handleObjectLiteralforUnionTypeInterop(node: ts.VariableDeclaration): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    if (!node.type || !ts.isUnionTypeNode(node.type)) {
      return;
    }

    if (!node.initializer || node.initializer.kind !== ts.SyntaxKind.ObjectLiteralExpression) {
      return;
    }

    const typeNodes = node.type.types;

    const isDefected = typeNodes.some((tNode) => {
      if (!ts.isTypeReferenceNode(tNode)) {
        return false;
      }
      const type = this.tsTypeChecker.getTypeAtLocation(tNode);
      const symbol = type.getSymbol();
      if (!symbol) {
        return false;
      }
      for (const declaration of symbol.declarations ?? []) {
        if (!this.tsUtils.isArkts12File(declaration.getSourceFile())) {
          return true;
        }
      }
      return false;
    });

    if (isDefected) {
      this.incrementCounters(node, FaultID.InteropObjectLiteralAmbiguity);
    }
  }

  private handleObjectLiteralAssignmentToClass(
    node:
      | ts.VariableDeclaration
      | ts.CallExpression
      | ts.ReturnStatement
      | ts.ArrayLiteralExpression
      | ts.PropertyDeclaration
      | ts.AsExpression
      | ts.BinaryExpression
  ): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    switch (node.kind) {
      case ts.SyntaxKind.VariableDeclaration:
        this.checkVariableDeclarationForObjectLiteral(node);
        break;
      case ts.SyntaxKind.CallExpression:
        this.checkCallExpressionForObjectLiteral(node);
        break;
      case ts.SyntaxKind.ReturnStatement:
        this.checkReturnStatementForObjectLiteral(node);
        break;
      case ts.SyntaxKind.ArrayLiteralExpression:
        this.checkArrayLiteralExpressionForObjectLiteral(node);
        break;
      case ts.SyntaxKind.PropertyDeclaration:
        this.checkPropertyDeclarationForObjectLiteral(node);
        break;
      case ts.SyntaxKind.AsExpression:
        this.checkAsExpressionForObjectLiteral(node);
        break;
      case ts.SyntaxKind.BinaryExpression:
        this.checkBinaryExpressionForObjectLiteral(node);
        break;
      default:
    }
  }

  private reportIfAssignedToNonArkts2Class(type: ts.Type, expr: ts.ObjectLiteralExpression): void {
    const symbol = type.getSymbol();
    if (!symbol) {
      return;
    }

    const declarations = symbol.declarations ?? [];
    const isClass = declarations.some(ts.isClassDeclaration);
    if (!isClass) {
      return;
    }

    const isFromArkTs2 = declarations.some((decl) => {
      return this.tsUtils.isArkts12File(decl.getSourceFile());
    });

    if (isFromArkTs2) {
      return;
    }

    const hasConstructor = declarations.some((decl) => {
      return ts.isClassDeclaration(decl) && decl.members.some(ts.isConstructorDeclaration);
    });

    if (hasConstructor) {
      this.incrementCounters(expr, FaultID.InteropObjectLiteralClass);
    }
  }

  private checkVariableDeclarationForObjectLiteral(node: ts.VariableDeclaration): void {
    if (!node.initializer || !node.type) {
      return;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(node.type);

    const checkObjectLiteral = (expr: ts.Expression): void => {
      if (ts.isObjectLiteralExpression(expr)) {
        this.reportIfAssignedToNonArkts2Class(type, expr);
      }
    };

    if (ts.isObjectLiteralExpression(node.initializer)) {
      checkObjectLiteral(node.initializer);
    } else if (ts.isConditionalExpression(node.initializer)) {
      checkObjectLiteral(node.initializer.whenTrue);
      checkObjectLiteral(node.initializer.whenFalse);
    }
  }

  private checkCallExpressionForObjectLiteral(node: ts.CallExpression): void {
    for (const arg of node.arguments) {
      if (ts.isObjectLiteralExpression(arg)) {
        const signature = this.tsTypeChecker.getResolvedSignature(node);
        const params = signature?.getParameters() ?? [];
        const index = node.arguments.indexOf(arg);
        const paramSymbol = params[index];
        if (!paramSymbol) {
          continue;
        }

        const paramDecl = paramSymbol.declarations?.[0];
        if (!paramDecl || !ts.isParameter(paramDecl) || !paramDecl.type) {
          continue;
        }

        const type = this.tsTypeChecker.getTypeAtLocation(paramDecl.type);
        this.reportIfAssignedToNonArkts2Class(type, arg);
      }
    }
  }

  private checkReturnStatementForObjectLiteral(node: ts.ReturnStatement): void {
    if (!node.expression || !ts.isObjectLiteralExpression(node.expression)) {
      return;
    }
    const func = ts.findAncestor(node, ts.isFunctionLike);
    if (!func?.type) {
      return;
    }

    const returnType = this.tsTypeChecker.getTypeAtLocation(func.type);
    this.reportIfAssignedToNonArkts2Class(returnType, node.expression);
  }

  private checkArrayLiteralExpressionForObjectLiteral(node: ts.ArrayLiteralExpression): void {
    for (const element of node.elements) {
      if (ts.isObjectLiteralExpression(element)) {
        const contextualType = this.tsTypeChecker.getContextualType(node);
        if (!contextualType) {
          continue;
        }

        const typeArgs = (contextualType as ts.TypeReference).typeArguments;
        const elementType = typeArgs?.[0];
        if (!elementType) {
          continue;
        }

        this.reportIfAssignedToNonArkts2Class(elementType, element);
      }
    }
  }

  private checkPropertyDeclarationForObjectLiteral(node: ts.PropertyDeclaration): void {
    if (!node.initializer || !ts.isObjectLiteralExpression(node.initializer) || !node.type) {
      return;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(node.type);
    this.reportIfAssignedToNonArkts2Class(type, node.initializer);
  }

  private checkAsExpressionForObjectLiteral(node: ts.AsExpression): void {
    if (!ts.isObjectLiteralExpression(node.expression)) {
      return;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(node.type);
    this.reportIfAssignedToNonArkts2Class(type, node.expression);
  }

  private checkBinaryExpressionForObjectLiteral(node: ts.BinaryExpression): void {
    if (node.operatorToken.kind !== ts.SyntaxKind.EqualsToken) {
      return;
    }
    if (!ts.isObjectLiteralExpression(node.right)) {
      return;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(node.left);
    this.reportIfAssignedToNonArkts2Class(type, node.right);
  }

  private isObjectLiteralAssignedToArkts12Type(node: ts.Expression, expectedTypeNode?: ts.TypeNode): boolean {
    if (node.kind !== ts.SyntaxKind.ObjectLiteralExpression) {
      return false;
    }

    let type: ts.Type;
    if (expectedTypeNode) {
      type = this.tsTypeChecker.getTypeAtLocation(expectedTypeNode);
    } else {
      type = this.tsTypeChecker.getContextualType(node) ?? this.tsTypeChecker.getTypeAtLocation(node);
    }

    if (!type) {
      return false;
    }

    return this.isTypeFromArkts12(type);
  }

  private isTypeFromArkts12(type: ts.Type): boolean {
    const symbol = type?.getSymbol();
    if (!symbol) {
      return false;
    }

    const isFromArkts12 = (symbol.declarations ?? []).some((decl) => {
      return this.tsUtils.isArkts12File(decl.getSourceFile());
    });

    if (isFromArkts12) {
      return true;
    }
    return false;
  }

  private processNestedObjectLiterals(objLiteral: ts.Expression, parentType?: ts.Type): void {
    if (!ts.isObjectLiteralExpression(objLiteral)) {
      return;
    }

    objLiteral.properties.forEach((prop) => {
      if (!ts.isPropertyAssignment(prop) || !ts.isObjectLiteralExpression(prop.initializer)) {
        return;
      }

      if (this.isObjectLiteralAssignedToArkts12Type(prop.initializer)) {
        this.incrementCounters(prop.initializer, FaultID.InteropStaticObjectLiterals);
        return;
      }

      this.checkPropertyTypeFromParent(prop, parentType);
      this.processNestedObjectLiterals(prop.initializer);
    });
  }

  private checkPropertyTypeFromParent(prop: ts.PropertyAssignment, parentType?: ts.Type): void {
    if (!parentType) {
      return;
    }
    if (!ts.isObjectLiteralExpression(prop.initializer)) {
      return;
    }

    const propName = prop.name.getText();
    const property = parentType.getProperty(propName);

    if (!property?.valueDeclaration) {
      return;
    }

    const propType = this.tsTypeChecker.getTypeOfSymbolAtLocation(property, property.valueDeclaration);

    if (this.isTypeFromArkts12(propType)) {
      this.incrementCounters(prop.initializer, FaultID.InteropStaticObjectLiterals);
    }
  }

  private handleObjectLiteralAssignment(node: ts.VariableDeclaration): void {
    if (this.tsUtils.isArkts12File(node.getSourceFile())) {
      return;
    }

    if (!node.initializer) {
      return;
    }

    if (
      ts.isObjectLiteralExpression(node.initializer) &&
      this.isObjectLiteralAssignedToArkts12Type(node.initializer, node.type)
    ) {
      this.incrementCounters(node.initializer, FaultID.InteropStaticObjectLiterals);
      return;
    }

    const parentType = node.type ?
      this.tsTypeChecker.getTypeAtLocation(node.type) :
      this.tsTypeChecker.getTypeAtLocation(node.initializer);

    this.processNestedObjectLiterals(node.initializer, parentType);
  }

  private handleObjectLiteralInFunctionArgs(node: ts.CallExpression): void {
    if (this.tsUtils.isArkts12File(node.getSourceFile())) {
      return;
    }
    const signature = this.tsTypeChecker.getResolvedSignature(node);
    if (!signature) {
      return;
    }

    const params = signature.getParameters();

    node.arguments.forEach((arg, index) => {
      if (!ts.isObjectLiteralExpression(arg)) {
        return;
      }

      if (index < params.length) {
        const param = params[index];
        if (!param.valueDeclaration) {
          return;
        }

        const paramType = this.tsTypeChecker.getTypeOfSymbolAtLocation(param, param.valueDeclaration);

        if (this.isTypeFromArkts12(paramType)) {
          this.incrementCounters(arg, FaultID.InteropStaticObjectLiterals);
        }
      } else if (this.isObjectLiteralAssignedToArkts12Type(arg)) {
        this.incrementCounters(arg, FaultID.InteropStaticObjectLiterals);
      }
    });
  }

  private handleObjectLiteralInReturn(node: ts.ReturnStatement): void {
    if (this.tsUtils.isArkts12File(node.getSourceFile())) {
      return;
    }

    if (!node.expression || !ts.isObjectLiteralExpression(node.expression)) {
      return;
    }

    let current: ts.Node = node;
    let functionNode: ts.FunctionLikeDeclaration | undefined;

    while (current && !functionNode) {
      current = current.parent;
      if (
        current &&
        (ts.isFunctionDeclaration(current) ||
          ts.isMethodDeclaration(current) ||
          ts.isFunctionExpression(current) ||
          ts.isArrowFunction(current))
      ) {
        functionNode = current;
      }
    }

    if (functionNode?.type) {
      const returnType = this.tsTypeChecker.getTypeAtLocation(functionNode.type);
      if (this.isTypeFromArkts12(returnType)) {
        this.incrementCounters(node.expression, FaultID.InteropStaticObjectLiterals);
      }
    } else if (this.isObjectLiteralAssignedToArkts12Type(node.expression)) {
      this.incrementCounters(node.expression, FaultID.InteropStaticObjectLiterals);
    }
  }

  private handleLocalBuilderDecorator(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }
    if (!ts.isDecorator(node) || !ts.isIdentifier(node.expression)) {
      return;
    }
    const decoratorName = node.expression.getText();
    if (decoratorName === CustomDecoratorName.LocalBuilder) {
      const autofix = this.autofixer?.fixBuilderDecorators(node);
      this.incrementCounters(node, FaultID.LocalBuilderDecoratorNotSupported, autofix);
    }
  }

  private checkEnumGetMemberValue(node: ts.ElementAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    const symbol = this.tsUtils.trueSymbolAtLocation(node.expression);
    if (!symbol?.declarations) {
      return;
    }

    for (const decl of symbol.declarations) {
      if (ts.isEnumDeclaration(decl) && this.shouldIncrementCounters(node)) {
        this.incrementCounters(node, FaultID.UnsupportPropNameFromValue);
        return;
      }
    }
  }

  private shouldIncrementCounters(node: ts.ElementAccessExpression): boolean {
    const indexExpr = node.argumentExpression;
    if (!indexExpr) {
      return false;
    }
    if (ts.isStringLiteral(indexExpr) || ts.isNumericLiteral(indexExpr)) {
      return true;
    }
    const type = this.tsTypeChecker.getTypeAtLocation(indexExpr);
    const typeString = this.tsTypeChecker.typeToString(type);
    return typeString === 'number' || typeString === 'string';
  }

  private handleMakeObserved(node: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    const name = node.name;
    if (name.getText() !== MAKE_OBSERVED) {
      return;
    }

    const expr = node.expression;
    const symbol = this.tsTypeChecker.getSymbolAtLocation(expr);
    const importSpecifier = TsUtils.getDeclaration(symbol);
    if (!importSpecifier || !ts.isImportSpecifier(importSpecifier)) {
      return;
    }

    const importDecl = ts.findAncestor(importSpecifier, ts.isImportDeclaration);
    if (!importDecl) {
      return;
    }

    const moduleSpecifier = importDecl.moduleSpecifier;
    if (!ts.isStringLiteral(moduleSpecifier)) {
      return;
    }
    if (moduleSpecifier.text !== ARKUI_PACKAGE_NAME && moduleSpecifier.text !== ARKUI_STATE_MANAGEMENT) {
      return;
    }

    this.incrementCounters(node, FaultID.MakeObservedIsNotSupported);
  }

  private handlePropertyDeclarationForProp(node: ts.PropertyDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const decorators = ts.getDecorators(node);
    if (!decorators || decorators.length === 0) {
      return;
    }

    let decoratorName: string | undefined;
    if (ts.isIdentifier(decorators[0].expression)) {
      decoratorName = decorators[0].expression.getText();
    } else if (ts.isCallExpression(decorators[0].expression) && ts.isIdentifier(decorators[0].expression.expression)) {
      decoratorName = decorators[0].expression.expression.getText();
    }

    if (!decoratorName) {
      return;
    }

    const autofix = this.autofixer?.fixPropDecorator(decorators[0], decoratorName);
    switch (decoratorName) {
      case PropDecoratorName.Prop:
        this.incrementCounters(node, FaultID.PropDecoratorNotSupported, autofix);
        break;
      case PropDecoratorName.StorageProp:
        this.incrementCounters(node, FaultID.StoragePropDecoratorNotSupported, autofix);
        break;
      case PropDecoratorName.LocalStorageProp:
        this.incrementCounters(node, FaultID.LocalStoragePropDecoratorNotSupported, autofix);
        break;
      default:
    }
  }

  private handleVariableDeclarationForProp(node: ts.VariableDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const callExpr = node.initializer;
    if (!callExpr || !ts.isCallExpression(callExpr)) {
      return;
    }

    const propertyAccessExpr = callExpr.expression;
    if (!ts.isPropertyAccessExpression(propertyAccessExpr)) {
      return;
    }

    const storage = propertyAccessExpr.expression;
    if (
      !ts.isIdentifier(storage) ||
      !this.isTargetStorageType(storage, [StorageTypeName.LocalStorage, StorageTypeName.AppStorage])
    ) {
      return;
    }

    const functionName = propertyAccessExpr.name.getText();
    switch (functionName) {
      case PropFunctionName.Prop:
        this.incrementCounters(node, FaultID.PropFunctionNotSupported);
        break;
      case PropFunctionName.SetAndProp:
        this.incrementCounters(node, FaultID.SetAndPropFunctionNotSupported);
        break;
      default:
    }
  }

  private isTargetStorageType(storage: ts.Identifier, targetTypes: string[]): boolean {
    const decl = this.tsUtils.getDeclarationNode(storage);
    if (!decl || decl.getSourceFile() !== storage.getSourceFile()) {
      return targetTypes.includes(storage.getText());
    }

    if (!ts.isVariableDeclaration(decl)) {
      return false;
    }

    let storageType: ts.Node | undefined;
    if (decl.initializer) {
      if (ts.isNewExpression(decl.initializer)) {
        storageType = decl.initializer.expression;
      } else if (ts.isCallExpression(decl.initializer) && ts.isPropertyAccessExpression(decl.initializer.expression)) {
        storageType = decl.initializer.expression.expression;
      }
    }

    if (!storageType || !ts.isIdentifier(storageType)) {
      return false;
    }

    return targetTypes.includes(storageType.getText());
  }

  private handlePropertyAssignmentForProp(node: ts.PropertyAssignment): void {
    if (!this.options.arkts2) {
      return;
    }

    const callExpr = node.parent.parent;
    if (!ts.isCallExpression(callExpr)) {
      return;
    }

    const structDecl = TsUtils.getDeclaration(this.tsTypeChecker.getSymbolAtLocation(callExpr.expression));
    if (!structDecl || !ts.isStructDeclaration(structDecl) || !structDecl.name) {
      return;
    }

    const variable = node.name;
    if (!ts.isIdentifier(variable)) {
      return;
    }

    const targetNode = TypeScriptLinter.findVariableChangeNodeInStruct(variable, structDecl);
    if (!targetNode) {
      return;
    }

    const targetDecl = TsUtils.getDeclaration(this.tsTypeChecker.getSymbolAtLocation(targetNode));
    if (!targetDecl || !ts.isPropertyDeclaration(targetDecl)) {
      return;
    }

    const decorators = ts.getDecorators(targetDecl);
    if (!decorators || decorators.length === 0) {
      return;
    }

    const decorator = decorators[0];
    const decoratorName = TsUtils.getDecoratorName(decorator);
    if (decoratorName === PropDecoratorName.Prop) {
      this.incrementCounters(node, FaultID.PropNeedCallMethodForDeepCopy);
    }
  }

  private static findVariableChangeNodeInStruct(
    variable: ts.Identifier,
    structDecl: ts.StructDeclaration
  ): ts.MemberName | undefined {
    let changeNode: ts.MemberName | undefined;

    function traverse(node: ts.Node): void {
      if (changeNode) {
        return;
      }

      if (ts.isPropertyAccessExpression(node)) {
        if (
          node.expression.kind === ts.SyntaxKind.ThisKeyword &&
          node.name.getText() === variable.getText() &&
          (ts.findAncestor(node, ts.isPostfixUnaryExpression) ||
            ts.findAncestor(node, ts.isPrefixUnaryExpression) ||
            ts.findAncestor(node, ts.isBinaryExpression))
        ) {
          changeNode = node.name;
        }
      }

      ts.forEachChild(node, traverse);
    }

    traverse(structDecl);
    return changeNode;
  }

  private getIdentifierForAwaitExpr(awaitExpr: ts.AwaitExpression): IdentifierAndArguments {
    void this;

    let ident: undefined | ts.Identifier;
    let args: ts.NodeArray<ts.Expression> | undefined;

    const expr = awaitExpr.expression;
    if (ts.isCallExpression(expr)) {
      if (ts.isIdentifier(expr.expression)) {
        ident = expr.expression;
      }

      if (ts.isPropertyAccessExpression(expr.expression)) {
        if (ts.isIdentifier(expr.expression.name)) {
          ident = expr.expression.name;
        }
      }
      args = expr.arguments;
    } else if (ts.isIdentifier(expr)) {
      ident = expr;
    }

    return { ident, args };
  }

  private handleAwaitExpression(awaitExpr: ts.AwaitExpression): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }
    const { ident, args } = this.getIdentifierForAwaitExpr(awaitExpr);
    if (!ident) {
      return;
    }

    if (!this.tsUtils.isJsImport(ident)) {
      return;
    }

    const declaration = this.tsUtils.getDeclarationNode(ident);
    if (!declaration) {
      return;
    }

    if (
      ts.isFunctionDeclaration(declaration) &&
      TsUtils.hasModifier(declaration.modifiers, ts.SyntaxKind.AsyncKeyword)
    ) {
      const autofix = this.autofixer?.fixAwaitJsCallExpression(ident, args);
      this.incrementCounters(awaitExpr, FaultID.NoAwaitJsPromise, autofix);
      return;
    }

    if (ts.isMethodDeclaration(declaration) && TsUtils.hasModifier(declaration.modifiers, ts.SyntaxKind.AsyncKeyword)) {
      const autofix = this.autofixer?.fixAwaitJsMethodCallExpression(ident, args);
      this.incrementCounters(awaitExpr, FaultID.NoAwaitJsPromise, autofix);
      return;
    }

    if (!ts.isVariableDeclaration(declaration)) {
      return;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(declaration);
    const typeString = this.tsTypeChecker.typeToString(type);

    if (typeString.split('<')[0] !== 'Promise') {
      return;
    }

    const autofix = this.autofixer?.fixAwaitJsPromise(ident);
    this.incrementCounters(awaitExpr, FaultID.NoAwaitJsPromise, autofix);
  }

  private handleNotsLikeSmartTypeOnCallExpression(tsCallExpr: ts.CallExpression, callSignature: ts.Signature): void {
    if (!this.options.arkts2) {
      return;
    }
    const isContinue =
      ts.isCallExpression(tsCallExpr) &&
      ts.isIdentifier(tsCallExpr.expression) &&
      !ts.isReturnStatement(tsCallExpr.parent);
    if (!isContinue || !tsCallExpr.arguments) {
      return;
    }
    const declaration = callSignature.getDeclaration();
    if (!declaration || !ts.isFunctionDeclaration(declaration)) {
      return;
    }
    const parameterTypes = declaration.parameters?.map((param) => {
      const paramType = this.tsTypeChecker.getTypeAtLocation(param);
      return this.tsTypeChecker.typeToString(paramType);
    });
    tsCallExpr.arguments.forEach((arg, index) => {
      if (index >= parameterTypes.length) {
        return;
      }
      const expectedType = parameterTypes[index];
      const actualSym = this.tsTypeChecker.getSymbolAtLocation(arg);
      const decl = TsUtils.getDeclaration(actualSym);
      if (decl && ts.isParameter(decl) && decl.type) {
        const actualType = this.tsTypeChecker.getTypeFromTypeNode(decl.type);
        const actualTypeName = this.tsTypeChecker.typeToString(actualType);
        if (actualTypeName !== expectedType) {
          this.incrementCounters(arg, FaultID.NoTsLikeSmartType);
        }
      }
    });
  }

  private handleNotsLikeSmartTypeOnAsExpression(tsAsExpr: ts.AsExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    const asType = this.tsTypeChecker.getTypeAtLocation(tsAsExpr.type);
    const originType = this.tsTypeChecker.getTypeAtLocation(tsAsExpr.expression);
    const originTypeStr = this.tsTypeChecker.typeToString(originType);
    if (originTypeStr === 'never' && this.tsTypeChecker.typeToString(asType) !== originTypeStr) {
      this.incrementCounters(tsAsExpr, FaultID.NoTsLikeSmartType);
    }
  }
  
  private handleAssignmentNotsLikeSmartType(tsBinaryExpr: ts.BinaryExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    if (this.isPriorityInThreadInfo(tsBinaryExpr)) {
      this.incrementCounters(tsBinaryExpr, FaultID.NoTsLikeSmartType);
    }
  }

  private isPriorityInThreadInfo(node: ts.BinaryExpression): boolean {
    if (!ts.isBinaryExpression(node)) {
      return false;
    }

    // Handle both regular assignment and 'as' type assertion
    let right: ts.Expression = ts.isAsExpression(node.right) ? node.right.expression : node.right;
    if (!ts.isPropertyAccessExpression(right)) {
        return false;
    }

    const propertyName = right.name;
    if (!ts.isIdentifier(propertyName)) {
      return false;
    }

    const object = right.expression;
    if (!ts.isIdentifier(object)) {
      return false;
    }

    const symbol = this.tsTypeChecker.getSymbolAtLocation(object);
    if (!symbol) {
      return false;
    }

    const type = this.tsTypeChecker.getTypeOfSymbolAtLocation(symbol, object);
    const typeString = this.tsTypeChecker.typeToString(type);

    for (const [typeName, properties] of Object.entries(ERROR_TASKPOOL_PROP_LIST)) {
      if (typeString === typeName && properties.has(propertyName.text)) {
        return true;
      }
    }

    return false;
  }

  private handleNotsLikeSmartType(classDecl: ts.ClassDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const className = classDecl.name?.getText();
    const { staticProps, instanceProps } = this.collectClassProperties(classDecl);

    classDecl.members.forEach((member) => {
      if (!ts.isMethodDeclaration(member) || !member.body) {
        return;
      }

      const methodReturnType = this.tsTypeChecker.getTypeAtLocation(member);
      this.checkMethodAndReturnStatements(member.body, className, methodReturnType, staticProps, instanceProps);
    });
  }

  private checkMethodAndReturnStatements(
    body: ts.Block,
    className: string | undefined,
    methodReturnType: ts.Type,
    staticProps: Map<string, ts.Type>,
    instanceProps: Map<string, ts.Type>
  ): void {
    body.forEachChild((node) => {
      if (!ts.isReturnStatement(node) || !node.expression) {
        return;
      }

      const isStaticPropertyAccess = (node: ts.Expression, className: string): boolean => {
        return (
          ts.isPropertyAccessExpression(node) && ts.isIdentifier(node.expression) && node.expression.text === className
        );
      };
      const isInstancePropertyAccess = (node: ts.Expression): boolean => {
        return ts.isPropertyAccessExpression(node) && node.expression.kind === ts.SyntaxKind.ThisKeyword;
      };

      if (className && isStaticPropertyAccess(node.expression, className)) {
        this.checkPropertyAccess(node, node.expression as ts.PropertyAccessExpression, staticProps, methodReturnType);
        return;
      }

      if (isInstancePropertyAccess(node.expression)) {
        this.checkPropertyAccess(node, node.expression as ts.PropertyAccessExpression, instanceProps, methodReturnType);
      }
    });
  }

  private checkPropertyAccess(
    returnNode: ts.ReturnStatement,
    propAccess: ts.PropertyAccessExpression,
    propsMap: Map<string, ts.Type>,
    methodReturnType: ts.Type
  ): void {
    const propName = propAccess.name.getText();
    const propType = propsMap.get(propName);

    if (propType && this.isExactlySameType(propType, methodReturnType)) {
      return;
    }

    this.incrementCounters(returnNode, FaultID.NoTsLikeSmartType);
  }

  private collectClassProperties(classDecl: ts.ClassDeclaration): {
    staticProps: Map<string, ts.Type>;
    instanceProps: Map<string, ts.Type>;
  } {
    const result = {
      staticProps: new Map<string, ts.Type>(),
      instanceProps: new Map<string, ts.Type>()
    };

    classDecl.members.forEach((member) => {
      if (!ts.isPropertyDeclaration(member)) {
        return;
      }

      const propName = member.name.getText();
      const propType = this.tsTypeChecker.getTypeAtLocation(member);
      const isStatic = member.modifiers?.some((m) => {
        return m.kind === ts.SyntaxKind.StaticKeyword;
      });

      if (isStatic) {
        result.staticProps.set(propName, propType);
      } else {
        result.instanceProps.set(propName, propType);
      }
    });

    return result;
  }

  private isExactlySameType(type1: ts.Type, type2: ts.Type): boolean {
    if (type2.getCallSignatures().length > 0) {
      const returnType = TsUtils.getFunctionReturnType(type2);
      return returnType ?
        this.tsTypeChecker.typeToString(type1) === this.tsTypeChecker.typeToString(returnType) :
        false;
    }
    return this.tsTypeChecker.typeToString(type1) === this.tsTypeChecker.typeToString(type2);
  }

  private handleNumericBigintCompare(node: ts.BinaryExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    const leftType = this.tsTypeChecker.getTypeAtLocation(node.left);
    const rightType = this.tsTypeChecker.getTypeAtLocation(node.right);

    const isBigInt = (type: ts.Type): boolean => {
      return (type.flags & ts.TypeFlags.BigInt) !== 0 || (type.flags & ts.TypeFlags.BigIntLiteral) !== 0;
    };
    const isNumber = (type: ts.Type): boolean => {
      return (type.flags & ts.TypeFlags.Number) !== 0 || (type.flags & ts.TypeFlags.NumberLiteral) !== 0;
    };

    const isBigIntAndNumberOperand =
      isNumber(leftType) && isBigInt(rightType) || isBigInt(leftType) && isNumber(rightType);
    if (isBigIntAndNumberOperand) {
      this.incrementCounters(node, FaultID.NumericBigintCompare);
    }
  }

  private handleBigIntLiteral(node: ts.BigIntLiteral): void {
    if (!this.options.arkts2) {
      return;
    }
    const literalText = node.getText();

    if ((/^0[box]/i).test(literalText)) {
      this.incrementCounters(node, FaultID.NondecimalBigint);
    }
  }

  private handleStructDeclarationForLayout(node: ts.StructDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!node.name) {
      return;
    }

    let hasTargetFunc = false;

    const members = node.members;
    for (const member of members) {
      if (!ts.isMethodDeclaration(member)) {
        continue;
      }

      if (customLayoutFunctionName.has(member.name.getText())) {
        hasTargetFunc = true;
        break;
      }
    }

    if (!hasTargetFunc) {
      return;
    }

    const decorators = ts.getDecorators(node);
    if (decorators) {
      for (const decorator of decorators) {
        const decoratorName = TsUtils.getDecoratorName(decorator);
        if (decoratorName && decoratorName === CustomDecoratorName.CustomLayout) {
          return;
        }
      }
    }

    const autofix = this.autofixer?.fixCustomLayout(node);
    const name = node.name.getText();
    const errorMsg =
      `The Custom component "${name}" with custom layout capability needs to add the "@CustomLayout" decorator ` +
      '(arkui-custom-layout-need-add-decorator)';
    this.incrementCounters(node.name, FaultID.CustomLayoutNeedAddDecorator, autofix, errorMsg);
  }

  private handleArkTSPropertyAccess(expr: ts.BinaryExpression): void {
    if (!this.useStatic || !this.options.arkts2 || !TypeScriptLinter.isBinaryOperations(expr.operatorToken.kind)) {
      return;
    }

    const processExpression = (expr: ts.Expression): void => {
      const symbol = this.tsUtils.trueSymbolAtLocation(expr);
      if (this.isJsFileSymbol(symbol) || this.isJsFileExpression(expr)) {
        const autofix = this.autofixer?.fixInteropOperators(expr);
        this.incrementCounters(expr, FaultID.BinaryOperations, autofix);
      }
    };

    processExpression(expr.left);
    processExpression(expr.right);
  }

  private static isBinaryOperations(kind: ts.SyntaxKind): boolean {
    const binaryOperators: ts.SyntaxKind[] = [
      ts.SyntaxKind.PlusToken,
      ts.SyntaxKind.MinusToken,
      ts.SyntaxKind.AsteriskToken,
      ts.SyntaxKind.SlashToken,
      ts.SyntaxKind.PercentToken,
      ts.SyntaxKind.AsteriskAsteriskToken
    ];
    return binaryOperators.includes(kind);
  }

  private handleNumericLiteral(node: ts.Node): void {
    if (!this.options.arkts2 || !ts.isNumericLiteral(node)) {
      return;
    }
    const isInElementAccessExpression = (node: ts.NumericLiteral): boolean => {
      for (let parent = node.parent; parent; parent = parent.parent) {
        if (ts.isElementAccessExpression(parent)) {
          return true;
        }
      }
      return false;
    };
    const isStandardFloatFormat = (): boolean => {
      const text = node.getText();
      return (/\.\d*0+$/).test(text);
    };
    const isNoNeedFix =
      isInElementAccessExpression(node) ||
      'name' in node.parent && node.parent.name === node ||
      isStandardFloatFormat();
    if (isNoNeedFix) {
      return;
    }
    const value = parseFloat(node.text);
    const nodeText = node.getText();
    const hasScientificOrRadixNotation = (/[a-zA-Z]/).test(nodeText);
    const isIntegerWithoutZero = Number.isInteger(value) && !nodeText.endsWith('.0');
    if (isIntegerWithoutZero && !hasScientificOrRadixNotation) {
      const autofix = this.autofixer?.fixNumericLiteralIntToNumber(node);
      this.incrementCounters(node, FaultID.NumericSemantics, autofix);
    }
  }

  private checkArrayUsageWithoutBound(accessExpr: ts.ElementAccessExpression): void {
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }

    const arrayAccessInfo = this.getArrayAccessInfo(accessExpr);
    if (!arrayAccessInfo) {
      const accessArgument = accessExpr.argumentExpression;
      if (TypeScriptLinter.isFunctionCall(accessArgument)) {
        this.incrementCounters(accessExpr, FaultID.RuntimeArrayCheck);
      }
      return;
    }

    const { arrayIdent } = arrayAccessInfo;
    const arraySym = this.tsUtils.trueSymbolAtLocation(arrayIdent);
    if (!arraySym) {
      return;
    }

    const arrayDecl = TypeScriptLinter.findArrayDeclaration(arraySym);
    if (arrayDecl && TypeScriptLinter.isArrayCreatedWithOtherArrayLength(arrayDecl)) {
      return;
    }

    const indexExpr = accessExpr.argumentExpression;
    const loopVarName = ts.isIdentifier(indexExpr) ? indexExpr.text : undefined;

    const { isInSafeContext, isValidBoundCheck, isVarModifiedBeforeAccess } = this.analyzeSafeContext(
      accessExpr,
      loopVarName,
      arraySym
    );

    if (isInSafeContext) {
      if (!isValidBoundCheck || isVarModifiedBeforeAccess) {
        this.incrementCounters(arrayIdent.parent, FaultID.RuntimeArrayCheck);
      }
    } else {
      this.incrementCounters(arrayIdent.parent, FaultID.RuntimeArrayCheck);
    }
  }

  private analyzeSafeContext(
    accessExpr: ts.ElementAccessExpression,
    loopVarName: string | undefined,
    arraySym: ts.Symbol
  ): { isInSafeContext: boolean; isValidBoundCheck: boolean; isVarModifiedBeforeAccess: boolean } {
    const context = TypeScriptLinter.findSafeContext(accessExpr);
    if (!context) {
      return { isInSafeContext: false, isValidBoundCheck: false, isVarModifiedBeforeAccess: false };
    }

    return this.analyzeContextSafety(context, accessExpr, loopVarName, arraySym);
  }

  static findSafeContext(
    accessExpr: ts.ElementAccessExpression
  ): { node: ts.ForStatement | ts.WhileStatement | ts.IfStatement } | void {
    let currentNode: ts.Node | undefined = accessExpr;

    while (currentNode) {
      if (ts.isForStatement(currentNode) || ts.isWhileStatement(currentNode) || ts.isIfStatement(currentNode)) {
        return { node: currentNode };
      }
      currentNode = currentNode.parent;
    }

    return undefined;
  }

  private analyzeContextSafety(
    context: { node: ts.ForStatement | ts.WhileStatement | ts.IfStatement },
    accessExpr: ts.ElementAccessExpression,
    loopVarName: string | undefined,
    arraySym: ts.Symbol
  ): { isInSafeContext: boolean; isValidBoundCheck: boolean; isVarModifiedBeforeAccess: boolean } {
    const { node } = context;

    if (!loopVarName) {
      return {
        isInSafeContext: true,
        isValidBoundCheck: false,
        isVarModifiedBeforeAccess: false
      };
    }

    const analysis = this.analyzeStatementType(node, accessExpr, loopVarName, arraySym);

    return {
      isInSafeContext: true,
      isValidBoundCheck: analysis.isValidBoundCheck,
      isVarModifiedBeforeAccess: analysis.isVarModifiedBeforeAccess
    };
  }

  private analyzeStatementType(
    node: ts.ForStatement | ts.WhileStatement | ts.IfStatement,
    accessExpr: ts.ElementAccessExpression,
    loopVarName: string,
    arraySym: ts.Symbol
  ): { isValidBoundCheck: boolean; isVarModifiedBeforeAccess: boolean } {
    switch (node.kind) {
      case ts.SyntaxKind.ForStatement:
        return this.analyzeForStatement(node, accessExpr, loopVarName, arraySym);
      case ts.SyntaxKind.WhileStatement:
        return this.analyzeWhileStatement(node, accessExpr, loopVarName, arraySym);
      case ts.SyntaxKind.IfStatement:
        return this.analyzeIfStatement(node, accessExpr, loopVarName, arraySym);
      default:
        return { isValidBoundCheck: false, isVarModifiedBeforeAccess: false };
    }
  }

  private analyzeForStatement(
    forNode: ts.ForStatement,
    accessExpr: ts.ElementAccessExpression,
    loopVarName: string,
    arraySym: ts.Symbol
  ): { isValidBoundCheck: boolean; isVarModifiedBeforeAccess: boolean } {
    const isValidBoundCheck = forNode.condition ?
      this.checkBoundCondition(forNode.condition, loopVarName, arraySym) :
      false;

    const isVarModifiedBeforeAccess = forNode.statement ?
      TypeScriptLinter.checkVarModifiedBeforeNode(forNode.statement, accessExpr, loopVarName) :
      false;

    return { isValidBoundCheck, isVarModifiedBeforeAccess };
  }

  private analyzeWhileStatement(
    whileNode: ts.WhileStatement,
    accessExpr: ts.ElementAccessExpression,
    loopVarName: string,
    arraySym: ts.Symbol
  ): { isValidBoundCheck: boolean; isVarModifiedBeforeAccess: boolean } {
    const isValidBoundCheck = whileNode.expression ?
      this.checkBoundCondition(whileNode.expression, loopVarName, arraySym) :
      false;

    const isVarModifiedBeforeAccess = whileNode.statement ?
      TypeScriptLinter.checkVarModifiedBeforeNode(whileNode.statement, accessExpr, loopVarName) :
      false;

    return { isValidBoundCheck, isVarModifiedBeforeAccess };
  }

  private analyzeIfStatement(
    ifNode: ts.IfStatement,
    accessExpr: ts.ElementAccessExpression,
    loopVarName: string,
    arraySym: ts.Symbol
  ): { isValidBoundCheck: boolean; isVarModifiedBeforeAccess: boolean } {
    const isValidBoundCheck = ifNode.expression ?
      this.checkBoundCondition(ifNode.expression, loopVarName, arraySym) :
      false;

    let isVarModifiedBeforeAccess = false;
    const statementBlock = ts.isBlock(ifNode.thenStatement) ? ifNode.thenStatement : undefined;
    if (statementBlock) {
      isVarModifiedBeforeAccess = TypeScriptLinter.checkVarModifiedBeforeNode(statementBlock, accessExpr, loopVarName);
    }

    return { isValidBoundCheck, isVarModifiedBeforeAccess };
  }

  private checkBoundCondition(condition: ts.Expression, varName: string, arraySym: ts.Symbol): boolean {
    if (ts.isBinaryExpression(condition)) {
      const { left, right, operatorToken } = condition;

      if (this.checkVarLessThanArrayLength(left, right, operatorToken, varName, arraySym)) {
        return true;
      }

      if (this.checkArrayLengthGreaterThanVar(left, right, operatorToken, varName, arraySym)) {
        return true;
      }

      if (ts.isIdentifier(left) && left.text === varName && ts.isNumericLiteral(right)) {
        const value = parseFloat(right.text);
        if (
          operatorToken.kind === ts.SyntaxKind.GreaterThanEqualsToken && value <= 0 ||
          operatorToken.kind === ts.SyntaxKind.GreaterThanToken && value < 0
        ) {
          return true;
        }
      }

      return this.checkBoundCondition(left, varName, arraySym) || this.checkBoundCondition(right, varName, arraySym);
    }

    return false;
  }

  private checkArrayLengthGreaterThanVar(
    left: ts.Expression,
    right: ts.Expression,
    operatorToken: ts.Token<ts.BinaryOperator>,
    varName: string,
    arraySym: ts.Symbol
  ): boolean {
    if (
      ts.isPropertyAccessExpression(left) &&
      left.name.text === 'length' &&
      ts.isIdentifier(right) &&
      right.text === varName
    ) {
      const leftArraySym = this.tsUtils.trueSymbolAtLocation(left.expression);
      if (leftArraySym === arraySym) {
        return (
          operatorToken.kind === ts.SyntaxKind.GreaterThanToken ||
          operatorToken.kind === ts.SyntaxKind.GreaterThanEqualsToken
        );
      }
    }
    return false;
  }

  private checkVarLessThanArrayLength(
    left: ts.Expression,
    right: ts.Expression,
    operatorToken: ts.Token<ts.BinaryOperator>,
    varName: string,
    arraySym: ts.Symbol
  ): boolean {
    return (
      ts.isIdentifier(left) &&
      left.text === varName &&
      ts.isPropertyAccessExpression(right) &&
      right.name.text === 'length' &&
      (operatorToken.kind === ts.SyntaxKind.LessThanToken ||
        operatorToken.kind === ts.SyntaxKind.LessThanEqualsToken) &&
      this.tsUtils.trueSymbolAtLocation(right.expression) === arraySym
    );
  }

  private static traverseNodesUntilTarget(
    node: ts.Node,
    targetNode: ts.Node,
    varName: string,
    scopeStack: { shadowed: boolean; localVars: Set<string> }[],
    state: { targetFound: boolean; modified: boolean }
  ): void {
    if (node === targetNode) {
      state.targetFound = true;
      return;
    }

    if (state.targetFound) {
      return;
    }

    const newScope = this.handleNewScope(node, scopeStack);

    TypeScriptLinter.getVariablesFromScope(node, varName, scopeStack);

    if (this.isVariableModified(node, varName, scopeStack)) {
      state.modified = true;
    }

    ts.forEachChild(node, (child) => {
      this.traverseNodesUntilTarget(child, targetNode, varName, scopeStack, state);
    });

    if (newScope) {
      scopeStack.pop();
    }
  }

  private static handleNewScope(
    node: ts.Node,
    scopeStack: { shadowed: boolean; localVars: Set<string> }[]
  ): { shadowed: boolean; localVars: Set<string> } | null {
    if (ts.isBlock(node) || ts.isFunctionLike(node) || ts.isCatchClause(node)) {
      const parentScope = scopeStack[scopeStack.length - 1];
      const newScope = {
        shadowed: parentScope.shadowed,
        localVars: new Set<string>()
      };
      scopeStack.push(newScope);
      return newScope;
    }
    return null;
  }

  static getVariablesFromScope(
    node: ts.Node,
    varName: string,
    scopeStack: { shadowed: boolean; localVars: Set<string> }[]
  ): void {
    if (ts.isVariableDeclaration(node) && ts.isIdentifier(node.name) && node.name.text === varName) {
      const parent = node.parent;
      if (
        ts.isVariableDeclarationList(parent) &&
        (parent.flags & ts.NodeFlags.Let || parent.flags & ts.NodeFlags.Const)
      ) {
        scopeStack[scopeStack.length - 1].localVars.add(varName);
      }
    }

    if (ts.isParameter(node) && ts.isIdentifier(node.name) && node.name.text === varName) {
      scopeStack[scopeStack.length - 1].localVars.add(varName);
    }
  }

  private static isVariableModified(
    node: ts.Node,
    varName: string,
    scopeStack: { shadowed: boolean; localVars: Set<string> }[]
  ): boolean {
    if (!ts.isBinaryExpression(node) || node.operatorToken.kind !== ts.SyntaxKind.EqualsToken) {
      return false;
    }

    if (!ts.isIdentifier(node.left) || node.left.text !== varName) {
      return false;
    }

    for (let i = scopeStack.length - 1; i >= 0; i--) {
      if (scopeStack[i].localVars.has(varName)) {
        return false;
      }
    }

    return true;
  }

  static checkVarModifiedBeforeNode(container: ts.Node, targetNode: ts.Node, varName: string): boolean {
    const scopeStack: { shadowed: boolean; localVars: Set<string> }[] = [];
    scopeStack.push({ shadowed: false, localVars: new Set() });

    const state = {
      targetFound: false,
      modified: false
    };

    this.traverseNodesUntilTarget(container, targetNode, varName, scopeStack, state);
    return state.modified;
  }

  static isFunctionCall(node: ts.Node): boolean {
    return ts.isCallExpression(node) || ts.isArrowFunction(node) || ts.isFunctionExpression(node);
  }

  private getArrayAccessInfo(expr: ts.ElementAccessExpression): false | ArrayAccess {
    if (!ts.isIdentifier(expr.expression)) {
      return false;
    }
    const baseType = this.tsTypeChecker.getTypeAtLocation(expr.expression);
    if (!this.tsUtils.isArray(baseType)) {
      return false;
    }
    const accessArgument = expr.argumentExpression;

    TypeScriptLinter.isFunctionCall(accessArgument);

    const checkNumericType = (node: ts.Node): boolean => {
      const argType = this.tsTypeChecker.getTypeAtLocation(node);
      return (
        (argType.flags & ts.TypeFlags.NumberLike) !== 0 ||
        argType.isUnionOrIntersection() &&
          argType.types.some((t) => {
            return t.flags & ts.TypeFlags.NumberLike;
          })
      );
    };

    const isEnumMember = (node: ts.Node): boolean => {
      if (ts.isPropertyAccessExpression(node)) {
        const symbol = this.tsUtils.trueSymbolAtLocation(node);
        return !!symbol && (symbol.flags & ts.SymbolFlags.EnumMember) !== 0;
      }
      return false;
    };

    if (TypeScriptLinter.isFunctionCall(accessArgument)) {
      return false;
    }

    if (checkNumericType(accessArgument) || isEnumMember(accessArgument)) {
      return {
        pos: expr.getEnd(),
        accessingIdentifier: accessArgument,
        arrayIdent: expr.expression
      };
    }

    return false;
  }

  static isArrayCreatedWithOtherArrayLength(decl: ts.VariableDeclaration): boolean {
    if (!decl.initializer || !ts.isNewExpression(decl.initializer)) {
      return false;
    }

    const newExpr = decl.initializer;
    return (
      newExpr.arguments?.some((arg) => {
        return ts.isPropertyAccessExpression(arg) && arg.name.text === 'length';
      }) ?? false
    );
  }

  static findArrayDeclaration(sym: ts.Symbol): ts.VariableDeclaration | undefined {
    const decls = sym.getDeclarations();
    if (!decls) {
      return undefined;
    }

    for (const decl of decls) {
      if (ts.isVariableDeclaration(decl)) {
        return decl;
      }
    }
    return undefined;
  }
}
