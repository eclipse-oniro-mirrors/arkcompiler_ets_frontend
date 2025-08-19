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
import {
  PROMISE_METHODS,
  PROMISE_METHODS_WITH_NO_TUPLE_SUPPORT,
  SYMBOL,
  SYMBOL_CONSTRUCTOR,
  TsUtils
} from './utils/TsUtils';
import { FUNCTION_HAS_NO_RETURN_ERROR_CODE } from './utils/consts/FunctionHasNoReturnErrorCode';
import {
  LIMITED_STANDARD_UTILITY_TYPES,
  LIMITED_STANDARD_UTILITY_TYPES2
} from './utils/consts/LimitedStandardUtilityTypes';
import { LIKE_FUNCTION, LIKE_FUNCTION_CONSTRUCTOR, FORBIDDEN_FUNCTION_BODY } from './utils/consts/LikeFunction';
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
  STRINGLITERAL_ARRAY,
  STRINGLITERAL_INFINITY
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
import { TYPED_ARRAYS } from './utils/consts/TypedArrays';
import {
  BuiltinProblem,
  BuiltinProblemInfos,
  SYMBOL_ITERATOR,
  BUILTIN_CONSTRUCTORS,
  COLLECTION_METHODS,
  COLLECTION_TYPES,
  BUILTIN_TYPE,
  BUILTIN_DISABLE_CALLSIGNATURE,
  GET_OWN_PROPERTY_NAMES_TEXT,
  BUILTIN_CONSTRUCTOR_API_TYPE,
  BUILTIN_CONSTRUCTOR_API_NAME,
  BUILTIN_CALLSIGNATURE_NEWCTOR
} from './utils/consts/BuiltinWhiteList';
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
  ARKTSUTILS_LOCKS_MEMBER,
  OBJECT_PUBLIC_API_METHOD_SIGNATURES,
  ARKTSUTILS_PROCESS_MEMBER,
  PROCESS_DEPRECATED_INTERFACES
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
import type { BitVectorUsage } from './utils/consts/CollectionsAPI';
import { COLLECTIONS_TEXT, COLLECTIONS_MODULES, BIT_VECTOR } from './utils/consts/CollectionsAPI';
import { ASON_TEXT, ASON_MODULES, ARKTS_UTILS_TEXT, JSON_TEXT, ASON_WHITE_SET } from './utils/consts/ArkTSUtilsAPI';
import { interanlFunction } from './utils/consts/InternalFunction';
import { ETS_PART, PATH_SEPARATOR } from './utils/consts/OhmUrl';
import {
  DOUBLE_DOLLAR_IDENTIFIER,
  THIS_IDENTIFIER,
  STATE_STYLES,
  CustomInterfaceName,
  observedDecoratorName,
  skipImportDecoratorName,
  ENTRY_DECORATOR_NAME,
  PROVIDE_DECORATOR_NAME,
  PROVIDE_ALLOW_OVERRIDE_PROPERTY_NAME,
  ARKUI_MODULE,
  MAKE_OBSERVED,
  STATE_MANAGEMENT_MODULE,
  PropDecoratorName,
  PropFunctionName,
  StorageTypeName,
  customLayoutFunctionName,
  VIRTUAL_SCROLL_IDENTIFIER,
  BUILDERNODE_D_TS,
  BuilderNodeFunctionName,
  NESTING_BUILDER_SUPPORTED,
  COMMON_TS_ETS_API_D_TS,
  UI_STATE_MANAGEMENT_D_TS,
  PERSIST_PROP_FUNC_NAME,
  PERSIST_PROPS_FUNC_NAME,
  GLOBAL_CONNECT_FUNC_NAME,
  CONNECT_FUNC_NAME,
  serializationTypeFlags,
  serializationTypeName
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
import { EXTNAME_TS, EXTNAME_D_TS, EXTNAME_JS, EXTNAME_JSON } from './utils/consts/ExtensionName';
import { ARKTS_IGNORE_DIRS_OH_MODULES } from './utils/consts/ArktsIgnorePaths';
import type { ApiInfo, ApiListItem } from './utils/consts/SdkWhitelist';
import { ApiList, SdkProblem, SdkNameInfo } from './utils/consts/SdkWhitelist';
import * as apiWhiteList from './data/SdkWhitelist.json';
import * as builtinWhiteList from './data/BuiltinList.json';
import * as deprecatedApiList from './data/DeprecatedApiList.json';
import * as sdkCommonList from './data/SdkCommonList.json';
import {
  SdkCommonApiProblemInfos,
  SDK_COMMON_TRANSFORMER,
  SDK_COMMON_CONSTRUCTOR,
  SDK_COMMON_VOID,
  SDK_COMMON_SYMBOL_ITERATOR,
  SDK_COMMON_SYMBOL_ITERATOR_APINAME,
  sdkCommonAllDeprecatedFullTypeName,
  sdkCommonAllDeprecatedTypeName,
  SDK_COMMON_INDEX_CLASS,
  SDK_COMMON_BUFFER_API,
  SDK_COMMON_FUNCTIONLIKE,
  SDK_COMMON_PROPERTYLIKE,
  SDK_COMMON_CONSTRUCTORLIKE,
  SDK_COMMON_TYPEKEY,
  SDK_COMMON_TYPE
} from './utils/consts/SdkCommonDeprecateWhiteList';
import {
  DeprecateProblem,
  DEPRECATE_CHECK_KEY,
  DEPRECATE_UNNAMED,
  DEPRECATE_TYPE
} from './utils/consts/DeprecateWhiteList';
import {
  USE_SHARED,
  USE_CONCURRENT,
  ESLIB_SHAREDMEMORY_FILENAME,
  ESLIB_SHAREDARRAYBUFFER,
  TASKPOOL_MODULES,
  SYSTEM_MODULES
} from './utils/consts/ConcurrentAPI';
import {
  DEPRECATED_TASKPOOL_METHOD_SETCLONELIST,
  DEPRECATED_TASKPOOL_METHOD_SETTRANSFERLIST,
  STDLIB_TASK_CLASS_NAME,
  STDLIB_TASKPOOL_OBJECT_NAME
} from './utils/consts/TaskpoolAPI';
import { BaseTypeScriptLinter } from './BaseTypeScriptLinter';
import type { ArrayAccess, UncheckedIdentifier } from './utils/consts/RuntimeCheckAPI';
import { NUMBER_LITERAL, LENGTH_IDENTIFIER } from './utils/consts/RuntimeCheckAPI';
import { globalApiAssociatedInfo } from './utils/consts/AssociatedInfo';
import { ARRAY_API_LIST } from './utils/consts/ArraysAPI';
import {
  ABILITY_KIT,
  ASYNC_LIFECYCLE_SDK_LIST,
  ON_DESTROY,
  ON_DISCONNECT,
  PROMISE,
  SERVICE_EXTENSION_ABILITY,
  VOID,
  ABILITY_LIFECYCLE_SDK
} from './utils/consts/AsyncLifecycleSDK';
import { ERROR_PROP_LIST } from './utils/consts/ErrorProp';
import { D_ETS, D_TS } from './utils/consts/TsSuffix';
import { arkTsBuiltInTypeName } from './utils/consts/ArkuiImportList';
import { ERROR_TASKPOOL_PROP_LIST } from './utils/consts/ErrorProp';
import { COMMON_UNION_MEMBER_ACCESS_WHITELIST } from './utils/consts/ArktsWhiteApiPaths';
import type { BaseClassConstructorInfo, ConstructorParameter, ExtendedIdentifierInfo } from './utils/consts/Types';
import { ExtendedIdentifierType } from './utils/consts/Types';
import { COMPONENT_DECORATOR, SELECT_IDENTIFIER, SELECT_OPTIONS, STRING_ERROR_LITERAL } from './utils/consts/Literals';
import { ES_OBJECT } from './utils/consts/ESObject';
import { cookBookMsg } from './CookBookMsg';
import { getCommonApiInfoMap } from './utils/functions/CommonApiInfo';

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
  static sdkCommonFuncMap: Map<string, Map<string, Set<ApiInfo>>>;
  private interfaceMap: Map<string, Set<ApiInfo>> = new Map<string, Set<ApiInfo>>();
  static pathMap: Map<string, Set<ApiInfo>>;
  static indexedTypeSet: Set<ApiListItem>;
  static globalApiInfo: Map<string, Set<ApiListItem>>;
  static builtApiInfo: Set<ApiListItem>;
  static builtinNewCtorSet: Set<ApiListItem>;
  static builtinFinalClassSet: Set<ApiListItem>;
  static deprecatedApiInfo: Set<ApiListItem>;
  static sdkCommonApiInfo: Set<ApiListItem>;
  static sdkCommonSymbotIterSet: Set<ApiListItem>;
  static sdkCommonAllDeprecatedTypeNameSet: Set<ApiListItem>;
  static sdkCommonIndexClassSet: Map<string, string[]>;
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
    TypeScriptLinter.builtApiInfo = new Set<ApiListItem>();
    TypeScriptLinter.builtinNewCtorSet = new Set<ApiListItem>();
    TypeScriptLinter.builtinFinalClassSet = new Set<ApiListItem>();
    TypeScriptLinter.deprecatedApiInfo = new Set<ApiListItem>();
    TypeScriptLinter.sdkCommonApiInfo = new Set<ApiListItem>();
    TypeScriptLinter.funcMap = new Map<string, Map<string, Set<ApiInfo>>>();
    TypeScriptLinter.sdkCommonFuncMap = new Map<string, Map<string, Set<ApiInfo>>>();
    TypeScriptLinter.symbotIterSet = new Set<string>();
    TypeScriptLinter.sdkCommonSymbotIterSet = new Set<ApiListItem>();
    TypeScriptLinter.sdkCommonAllDeprecatedTypeNameSet = new Set<ApiListItem>();
    TypeScriptLinter.sdkCommonIndexClassSet = new Map<string, string[]>();
    TypeScriptLinter.missingAttributeSet = new Set<string>();
    TypeScriptLinter.initSdkWhitelist();
    TypeScriptLinter.initSdkBuiltinInfo();
    TypeScriptLinter.initBuiltinlist();
    TypeScriptLinter.initDeprecatedApiList();
    TypeScriptLinter.initSdkCommonApilist();
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
          case BuiltinProblem.BuiltinNewCtor:
            TypeScriptLinter.builtinNewCtorSet.add(item);
            break;
          case BuiltinProblem.BuiltinFinalClass:
            TypeScriptLinter.builtinFinalClassSet.add(item);
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
        this.builtApiInfo.add(item);
        TypeScriptLinter.addGlobalApiInfosCollocetionData(item);
      }
    }
  }

  private static initSdkCommonApilist(): void {
    const list: ApiList = new ApiList(sdkCommonList);
    if (list?.api_list?.length > 0) {
      for (const item of list.api_list) {
        const parent_api_name = item.api_info.parent_api[0].api_name;
        if (item.api_info.problem === BuiltinProblem.LimitedThisArg) {
          TypeScriptLinter.initSdkCommonThisArgsWhitelist(item);
        } else if (item.api_info.api_name === SDK_COMMON_SYMBOL_ITERATOR_APINAME) {
          TypeScriptLinter.sdkCommonSymbotIterSet.add(item);
        } else if (sdkCommonAllDeprecatedTypeName.has(parent_api_name)) {
          TypeScriptLinter.sdkCommonAllDeprecatedTypeNameSet.add(item);
        } else {
          this.sdkCommonApiInfo.add(item);
          if (SDK_COMMON_INDEX_CLASS.has(parent_api_name)) {
            const combinedPaths = [...item.import_path, item.file_path];
            TypeScriptLinter.sdkCommonIndexClassSet.set(parent_api_name, combinedPaths);
          }
        }
      }
    }
  }

  static initSdkCommonThisArgsWhitelist(item: ApiListItem): void {
    if (item.file_path === '' || !item.api_info.api_name || item.api_info.parent_api?.length <= 0) {
      return;
    }
    const key = item.api_info.api_name + '_' + item.api_info.parent_api[0].api_name;
    let funcApiInfos: Map<string, Set<ApiInfo>> | undefined = TypeScriptLinter.sdkCommonFuncMap.get(key);
    if (!funcApiInfos) {
      funcApiInfos = new Map<string, Set<ApiInfo>>();
      TypeScriptLinter.sdkCommonFuncMap.set(key, funcApiInfos);
    }
    TypeScriptLinter.addOrUpdateData(funcApiInfos, path.basename(item.file_path), item.api_info);
  }

  private static initDeprecatedApiList(): void {
    const list: ApiList = new ApiList(deprecatedApiList);
    if (list?.api_list?.length > 0) {
      for (const item of list.api_list) {
        this.deprecatedApiInfo.add(item);
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
    [ts.SyntaxKind.TemplateLiteralType, this.handleTemplateType],
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
    if (objectLiteralType && this.options.arkts2) {
      this.isObjectLiteralKeyTypeValid(objectLiteralExpr, objectLiteralType);
    }

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
      if (objectLiteralType) {
        const typeDecl = TsUtils.getDeclaration(objectLiteralType.getSymbol());
        if (typeDecl && ts.isInterfaceDeclaration(typeDecl) && ts.isMethodDeclaration(prop)) {
          continue;
        }
      }
      if (ts.isShorthandPropertyAssignment(prop)) {
        if (this.checkShorthandInObjectLiteral(prop, objectLiteralType)) {
          const autofix = this.autofixer?.fixShorthandPropertyAssignment(prop);
          this.incrementCounters(prop, FaultID.ObjectLiteralProperty, autofix);
        }
      } else {
        this.incrementCounters(prop, FaultID.ObjectLiteralProperty, objLiteralAutofix);
      }
    }
  }

  private checkShorthandInObjectLiteral(prop: ts.ShorthandPropertyAssignment, type: ts.Type | undefined): boolean {
    if (!type) {
      return true;
    }
    const propName = prop.name.text;
    const expectedProp = type.getProperty(propName);
    if (!expectedProp) {
      return false;
    }
    const expectedPropType = this.tsTypeChecker.getTypeOfSymbolAtLocation(expectedProp, prop.name);
    const symbol = this.tsTypeChecker.getSymbolAtLocation(prop.name);
    const varDecl = symbol?.valueDeclaration;
    if (!varDecl) {
      return false;
    }
    const actualType = this.tsTypeChecker.getTypeAtLocation(varDecl);
    if (!this.isTypeAssignable(actualType, expectedPropType)) {
      return true;
    }
    return false;
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

    /*
     * check that array literal consists of inferrable types
     * e.g. there is no element which is untyped object literals
     */
    const isCallExpression = this.checkMethodCallForSparseArray(parent);
    const isTypedArrayOrBuiltInConstructor = TypeScriptLinter.checkTypedArrayOrBuiltInConstructor(parent);
    if (this.options.arkts2 && arrayElementIsEmpty) {
      if (!arrayLitType) {
        this.incrementCounters(node, FaultID.NosparseArray);
      } else if (isCallExpression || isTypedArrayOrBuiltInConstructor) {
        this.incrementCounters(arrayLitNode, FaultID.NosparseArray);
      }
    }

    let emptyContextTypeForArrayLiteral = false;
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

  private static checkTypedArrayOrBuiltInConstructor(parent: ts.Node): boolean {
    if (!ts.isNewExpression(parent)) {
      return false;
    }
    const newExpr = parent;
    const typeName = newExpr.expression.getText();

    return TYPED_ARRAYS.includes(typeName) || BUILTIN_CONSTRUCTORS.includes(typeName);
  }

  private checkMethodCallForSparseArray(parent: ts.Node): boolean {
    if (!ts.isCallExpression(parent)) {
      return false;
    }

    const callExpr = parent;
    const promiseMethodName = TypeScriptLinter.getPromiseMethodName(callExpr.expression);
    if (promiseMethodName && PROMISE_METHODS.has(promiseMethodName)) {
      return true;
    }

    const collectionMethodName = this.getCollectionMethodName(callExpr.expression);
    if (collectionMethodName && COLLECTION_METHODS.has(collectionMethodName)) {
      return true;
    }

    return false;
  }

  private getCollectionMethodName(node: ts.Expression): string | undefined {
    if (!ts.isPropertyAccessExpression(node)) {
      return undefined;
    }

    const expr = node.expression;
    if (ts.isIdentifier(expr) || ts.isPropertyAccessExpression(expr)) {
      const type = this.tsTypeChecker.getTypeAtLocation(expr);
      const typeName = type.symbol?.getName();
      if (typeName && COLLECTION_TYPES.has(typeName)) {
        return node.name.text;
      }
    }

    return undefined;
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
    if (!ts.isArrowFunction(node.parent)) {
      this.handleInvalidIdentifier(tsParam);
    }
    this.handleSdkGlobalApi(tsParam);
    const typeNode = tsParam.type;
    if (this.options.arkts2 && typeNode && TsUtils.typeContainsVoid(typeNode)) {
      this.incrementCounters(typeNode, FaultID.LimitedVoidType);
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
    this.handleLocalDeclarationOfClassAndIface(interfaceNode);
    this.checkObjectPublicApiMethods(interfaceNode);
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
        const autofix = this.autofixer?.fixSideEffectImport(importDeclNode);
        this.incrementCounters(node, FaultID.NoSideEffectImport, autofix);
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
    this.checkImportJsonFile(importDeclNode);
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
    const propertyAccessNode = node as ts.PropertyAccessExpression;
    this.handlePropertyAccessExpressionForUI(propertyAccessNode);
    this.handleQuotedHyphenPropsDeprecated(propertyAccessNode);
    this.handleSdkTypeQuery(propertyAccessNode);
    this.checkUnionTypes(propertyAccessNode);
    this.handleLimitedVoidTypeFromSdkOnPropertyAccessExpression(propertyAccessNode);
    this.checkDepricatedIsConcurrent(propertyAccessNode);
    this.propertyAccessExpressionForBuiltin(propertyAccessNode);
    this.checkConstrutorAccess(propertyAccessNode);
    this.handleTaskPoolDeprecatedUsages(propertyAccessNode);
    this.handleNoTuplesArraysForPropertyAccessExpression(propertyAccessNode);
    this.handleUnsafeOptionalCallComparison(propertyAccessNode);
    this.handleNoDeprecatedApi(node as ts.PropertyAccessExpression);
    if (ts.isCallExpression(propertyAccessNode.parent) && propertyAccessNode === propertyAccessNode.parent.expression) {
      return;
    }
    const exprSym = this.tsUtils.trueSymbolAtLocation(propertyAccessNode);
    const baseExprSym = this.tsUtils.trueSymbolAtLocation(propertyAccessNode.expression);
    const baseExprType = this.tsTypeChecker.getTypeAtLocation(propertyAccessNode.expression);
    this.propertyAccessExpressionForInterop(propertyAccessNode);
    if (this.isPrototypePropertyAccess(propertyAccessNode, exprSym, baseExprSym, baseExprType)) {
      this.incrementCounters(propertyAccessNode.name, FaultID.Prototype);
    }
    this.checkSymbolAPI(propertyAccessNode, exprSym);
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
    this.handleBuiltinIteratorResult(propertyAccessNode);
  }

  private handlePropertyAccessExpressionForUI(node: ts.PropertyAccessExpression): void {
    this.handleMakeObserved(node);
    this.handleStateStyles(node);
    this.handleDoubleDollar(node);
    this.handlePropertyAccessExprForBuilderNode(node);
  }

  private checkSymbolAPI(node: ts.PropertyAccessExpression, exprSym: ts.Symbol | undefined): void {
    if (
      !this.options.arkts2 &&
      !!exprSym &&
      this.tsUtils.isStdSymbolAPI(exprSym) &&
      !ALLOWED_STD_SYMBOL_API.includes(exprSym.getName())
    ) {
      this.incrementCounters(node, FaultID.SymbolType);
    }
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
      typeString === 'symbol' ||
      this.isMixedEnum(baseType) ||
      this.isSpecialType(baseType, node) ||
      this.isStdUtilityTools(node)
    ) {
      this.incrementCounters(node, FaultID.InteropDirectAccessToTSTypes);
    }
  }

  private isSpecialType(baseType: ts.Type, node: ts.Node): boolean {
    const baseTypeStr = this.tsTypeChecker.typeToString(baseType);
    if (TypeScriptLinter.extractKeyofFromString(baseTypeStr)) {
      return true;
    }
    let symbol = baseType.getSymbol();
    if (!symbol) {
      symbol = this.tsUtils.trueSymbolAtLocation(node);
    }
    const decl = TsUtils.getDeclaration(symbol);
    if (!decl) {
      return false;
    }
    if (
      ts.isTypeAliasDeclaration(decl) && this.checkSpecialTypeNode(decl.type, true) ||
      this.checkSpecialTypeNode(decl, true)
    ) {
      return true;
    }

    if (this.isObjectLiteralExpression(decl)) {
      return true;
    }

    if (ts.isFunctionLike(decl)) {
      if (decl.type && this.checkIsTypeLiteral(decl.type)) {
        return true;
      }
      const isObjectLiteralExpression = decl.parameters.some((param) => {
        return param.type && this.checkIsTypeLiteral(param.type);
      });
      if (isObjectLiteralExpression) {
        return true;
      }
      if (TypeScriptLinter.hasObjectLiteralReturn(decl as ts.FunctionLikeDeclaration)) {
        return true;
      }
    }

    return false;
  }

  private isMixedEnum(type: ts.Type): boolean {
    const symbol = type.getSymbol();
    if (!symbol) {
      return false;
    }

    const declarations = symbol.getDeclarations();
    if (!declarations) {
      return false;
    }

    for (const decl of declarations) {
      if (ts.isEnumDeclaration(decl)) {
        const initializerTypes = new Set<string>();

        for (const member of decl.members) {
          if (member.initializer) {
            const memberType = this.tsTypeChecker.getTypeAtLocation(member.initializer);
            const baseTypeStr = this.tsTypeChecker.typeToString(
              this.tsTypeChecker.getBaseTypeOfLiteralType(memberType)
            );
            initializerTypes.add(baseTypeStr);
          }
        }

        if (initializerTypes.size > 1) {
          return true;
        }
      }
    }

    return false;
  }

  private isStdUtilityTools(node: ts.Node): boolean {
    const symbol = this.tsUtils.trueSymbolAtLocation(node);
    const decl = TsUtils.getDeclaration(symbol);
    if (!decl) {
      return false;
    }
    let isStdUtilityType = false;
    const utils = this.tsUtils;
    function traverse(node: ts.Node): void {
      if (isStdUtilityType) {
        return;
      }
      if (ts.isTypeReferenceNode(node) || ts.isExpressionWithTypeArguments(node)) {
        let typeName = '';
        if (ts.isTypeReferenceNode(node)) {
          typeName = utils.entityNameToString(node.typeName);
        } else {
          typeName = node.expression.getText();
        }
        isStdUtilityType = !!(
          LIMITED_STANDARD_UTILITY_TYPES2.includes(typeName) &&
          node.typeArguments &&
          node.typeArguments.length > 0
        );
      }
      node.forEachChild(traverse);
    }
    traverse(decl);
    return isStdUtilityType;
  }

  private checkIsTypeLiteral(node: ts.Node): boolean {
    if (ts.isUnionTypeNode(node) || ts.isIntersectionTypeNode(node)) {
      return node.types.some((typeNode) => {
        return this.checkIsTypeLiteralWithTypeNodes(typeNode);
      });
    }

    return this.checkIsTypeLiteralWithTypeNodes(node);
  }

  private checkIsTypeLiteralWithTypeNodes(node: ts.Node): boolean {
    if (ts.isTypeLiteralNode(node) && node.members.length > 0) {
      return true;
    }

    if (ts.isTypeReferenceNode(node) && node.typeName) {
      const typeDecl = this.tsUtils.getDeclarationNode(node.typeName);
      return (
        typeDecl !== undefined && ts.isTypeAliasDeclaration(typeDecl) && this.checkSpecialTypeNode(typeDecl.type, false)
      );
    }

    return false;
  }

  private checkSpecialTypeNode(typeNode: ts.Node, isNeedCheckIsTypeLiteral: boolean): boolean {
    let specialType =
      ts.isIndexedAccessTypeNode(typeNode) ||
      ts.isConditionalTypeNode(typeNode) ||
      ts.isFunctionTypeNode(typeNode) ||
      ts.isMappedTypeNode(typeNode) ||
      ts.isTemplateLiteralTypeNode(typeNode);
    if (isNeedCheckIsTypeLiteral) {
      specialType ||= this.checkIsTypeLiteral(typeNode);
    }
    return specialType;
  }

  private isObjectLiteralExpression(decl: ts.Node): boolean {
    const isVariableWithInitializer =
      ts.isVariableDeclaration(decl) && decl.initializer && ts.isObjectLiteralExpression(decl.initializer);

    const isVariableWithTypeLiteral = ts.isVariableDeclaration(decl) && decl.type && this.checkIsTypeLiteral(decl.type);
    const isObjectLiteralExpression =
      ts.isObjectLiteralExpression(decl) ||
      this.checkIsTypeLiteral(decl) ||
      isVariableWithInitializer ||
      isVariableWithTypeLiteral;
    return !!isObjectLiteralExpression;
  }

  private static hasObjectLiteralReturn(funcNode: ts.FunctionLikeDeclaration): boolean {
    let found = false;
    function visit(node: ts.Node): void {
      if (found) {
        return;
      }

      if (ts.isReturnStatement(node) && node.expression && ts.isObjectLiteralExpression(node.expression)) {
        found = true;
        return;
      }

      ts.forEachChild(node, visit);
    }
    visit(funcNode);
    return found;
  }

  private static extractKeyofFromString(typeString: string): boolean {
    return (/\bkeyof\b/).test(typeString);
  }

  checkUnionTypes(propertyAccessNode: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    // Safeguard: only process the outermost property access, not nested chains
    if (ts.isPropertyAccessExpression(propertyAccessNode.parent)) {
      return;
    }

    const baseExprType = this.tsTypeChecker.getTypeAtLocation(propertyAccessNode.expression);
    const baseExprSym = baseExprType.aliasSymbol || baseExprType.getSymbol();
    const symbolName = baseExprSym ? baseExprSym.name : this.tsTypeChecker.typeToString(baseExprType);

    if (!baseExprType.isUnion() || COMMON_UNION_MEMBER_ACCESS_WHITELIST.has(symbolName)) {
      return;
    }

    const allTypes = baseExprType.types;
    const propName = propertyAccessNode.name.getText();

    // Only keep union members that have the property
    const typesWithProp = allTypes.filter((type) => {
      return this.tsUtils.findProperty(type, propName) !== undefined;
    });

    if (typesWithProp.length !== allTypes.length) {
      // Not all members have this property, nothing to check
      return;
    }

    // Extract the type of the property for each member
    const propTypes: string[] = [];
    for (const t of typesWithProp) {
      const propSym = this.tsUtils.findProperty(t, propName);
      if (propSym) {
        const propType = this.tsTypeChecker.getTypeOfSymbolAtLocation(propSym, propertyAccessNode);
        propTypes.push(this.tsTypeChecker.typeToString(propType));
      }
    }

    // If there's more than one distinct property type signature, flag it
    const distinctPropTypes = new Set(propTypes);
    if (distinctPropTypes.size > 1) {
      this.incrementCounters(propertyAccessNode, FaultID.AvoidUnionTypes);
    }
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
      this.incrementCounters(node.type, FaultID.LimitedVoidType);
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
    }
    this.handleDeclarationInferredType(node);
    this.handleDefiniteAssignmentAssertion(node);
    this.handleSendableClassProperty(node);
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
    this.handleNoDeprecatedApi(node);
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
    if (!ts.isBlock(arrowFunc.body)) {
      const contextRetType = this.tsTypeChecker.getContextualType(arrowFunc.body);
      if (contextRetType) {
        this.checkAssignmentMatching(arrowFunc.body, contextRetType, arrowFunc.body, true);
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
    if (this.options.arkts2) {
      this.handleParamType(tsFunctionDeclaration);
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
      this.processSendableDecoratorFunctionOverload(tsFunctionDeclaration);
    }
    this.handleTSOverload(tsFunctionDeclaration);
    this.handleInvalidIdentifier(tsFunctionDeclaration);
    this.checkDefaultParamBeforeRequired(tsFunctionDeclaration);
    this.handleLimitedVoidFunction(tsFunctionDeclaration);
  }

  private processSendableDecoratorFunctionOverload(tsFunctionDeclaration: ts.FunctionDeclaration): void {
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

  private handleParamType(decl: ts.FunctionLikeDeclaration): void {
    for (const param of decl.parameters) {
      if (param.type) {
        continue;
      }
      this.incrementCounters(param, FaultID.ParameterType);
    }
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
      this.handleInfinityIdentifier(tsUnaryArithm);
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

  private handleInfinityIdentifier(node: ts.PrefixUnaryExpression): void {
    const identifier = node.operand;
    if (identifier.getText() === STRINGLITERAL_INFINITY && node.operator === ts.SyntaxKind.TildeToken) {
      this.incrementCounters(node, FaultID.PrefixUnaryInfinity);
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
        this.handleUnsignedShiftOnNegative(tsBinaryExpr);
    }
    this.checkInterOpImportJsDataCompare(tsBinaryExpr);
    this.checkInteropEqualityJudgment(tsBinaryExpr);
    this.handleNumericBigintCompare(tsBinaryExpr);
    this.handleArkTSPropertyAccess(tsBinaryExpr);
    this.handleObjectLiteralAssignmentToClass(tsBinaryExpr);
    this.handleAssignmentNotsLikeSmartType(tsBinaryExpr);
    this.checkNumericSemanticsForBinaryExpression(tsBinaryExpr);
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
    if (
      ts.isBinaryExpression(node) &&
      TypeScriptLinter.isNumericInitializer(node.left) &&
      TypeScriptLinter.isNumericInitializer(node.right)
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

  private isObjectLiteralKeyTypeValid(objectLiteral: ts.ObjectLiteralExpression, contextualType: ts.Type): void {
    if (!this.tsUtils.isStdRecordType(contextualType)) {
      return;
    }
    objectLiteral.properties.forEach((prop: ts.ObjectLiteralElementLike): void => {
      if (ts.isPropertyAssignment(prop)) {
        if (!this.tsUtils.isValidRecordObjectLiteralKey(prop.name)) {
          this.incrementCounters(prop, FaultID.ObjectLiteralKeyType);
        }
      }
    });
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
    }
    this.handleEsValueDeclaration(tsVarDecl);
    this.handleDeclarationInferredType(tsVarDecl);
    this.handleDefiniteAssignmentAssertion(tsVarDecl);
    this.handleLimitedVoidType(tsVarDecl);
    this.handleInvalidIdentifier(tsVarDecl);
    this.checkTypeFromSdk(tsVarDecl.type);
    this.handleObjectLiteralforUnionTypeInterop(tsVarDecl);
    this.handleObjectLiteralAssignmentToClass(tsVarDecl);
    this.handleObjectLiteralAssignment(tsVarDecl);
    this.handlePropertyDescriptorInScenarios(tsVarDecl);
    this.handleSdkGlobalApi(tsVarDecl);
    this.handleNoDeprecatedApi(tsVarDecl);
    this.checkNumericSemanticsForVariable(tsVarDecl);
  }

  private checkNumericSemanticsForBinaryExpression(node: ts.BinaryExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    const isInArrayContext = this.isInArrayAssignmentContext(node);

    const isDivision = node.operatorToken.kind === ts.SyntaxKind.SlashToken;
    const isNullishCoalescing = node.operatorToken.kind === ts.SyntaxKind.QuestionQuestionToken;

    if (!isDivision && !isNullishCoalescing) {
      return;
    }

    if (this.tsUtils.isPossiblyImportedFromJS(node.left) || this.tsUtils.isPossiblyImportedFromJS(node.right)) {
      return;
    }

    if (isDivision && isInArrayContext) {
      this.checkNumericSemanticsForDivisionOperation(node);
    } else {
      this.checkNumericSemanticsForNullishCoalescing(node);
    }
  }

  private isInArrayAssignmentContext(node: ts.BinaryExpression): boolean {
    if (ts.isArrayLiteralExpression(node.parent)) {
      return true;
    }

    if (
      ts.isBinaryExpression(node.parent) &&
      node.parent.operatorToken.kind === ts.SyntaxKind.EqualsToken &&
      ts.isElementAccessExpression(node.parent.left)
    ) {
      return true;
    }

    if (
      ts.isBinaryExpression(node.parent) &&
      node.parent.operatorToken.kind === ts.SyntaxKind.EqualsToken &&
      this.tsUtils.isNumberArrayType(this.tsTypeChecker.getTypeAtLocation(node.parent.left))
    ) {
      return true;
    }

    return false;
  }

  private checkNumericSemanticsForDivisionOperation(node: ts.BinaryExpression): void {
    const left = node.left;
    const right = node.right;
    if (ts.isNumericLiteral(left)) {
      const leftText = left.getText();
      if (!leftText.includes('.')) {
        const autofix = this.autofixer?.fixNumericLiteralToFloat(left);
        this.incrementCounters(left, FaultID.NumericSemantics, autofix);
      }
    }

    if (ts.isNumericLiteral(right)) {
      const rightText = right.getText();
      if (!rightText.includes('.')) {
        const autofix = this.autofixer?.fixNumericLiteralToFloat(right);
        this.incrementCounters(right, FaultID.NumericSemantics, autofix);
      }
    }
  }

  private checkNumericSemanticsForNullishCoalescing(node: ts.BinaryExpression): void {
    if (!ts.isArrayLiteralExpression(node.right)) {
      return;
    }

    const leftType = this.tsTypeChecker.getTypeAtLocation(node.left);
    if (!this.tsUtils.isNumberArrayType(leftType)) {
      return;
    }
    this.checkNumericSemanticsForArrayLiteral(node.right);
  }

  private checkNumericSemanticsForArrayLiteral(node: ts.ArrayLiteralExpression): void {
    const arrayType = this.tsTypeChecker.getTypeAtLocation(node);
    if (!this.tsUtils.isNumberArrayType(arrayType)) {
      return;
    }

    for (const element of node.elements) {
      if (ts.isNumericLiteral(element) && !element.text.includes('.')) {
        const autofix = this.autofixer?.fixNumericLiteralToFloat(element);
        this.incrementCounters(element, FaultID.NumericSemantics, autofix);
      } else if (ts.isBinaryExpression(element) && element.operatorToken.kind === ts.SyntaxKind.SlashToken) {
        this.checkNumericSemanticsForDivisionOperation(element);
      }
    }
  }

  private checkNumericSemanticsForVariable(node: ts.VariableDeclaration): void {
    if (!this.options.arkts2 || !node.initializer) {
      return;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(node.name);
    if (!this.tsUtils.isNumberArrayType(type)) {
      return;
    }

    if (ts.isBinaryExpression(node.initializer) && node.initializer.operatorToken.kind === ts.SyntaxKind.BarBarToken) {
      this.checkNumericSemanticsForNullishCoalescing(node.initializer);
    } else if (ts.isConditionalExpression(node.initializer)) {
      this.checkNumericSemanticsForTernaryOperator(node.initializer);
    }
  }

  private checkNumericSemanticsForTernaryOperator(node: ts.ConditionalExpression): void {
    if (!ts.isArrayLiteralExpression(node.whenFalse)) {
      return;
    }
    const arrayLiteral = node.whenFalse;

    this.checkNumericSemanticsForArrayLiteral(arrayLiteral);
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
        this.checkTsLikeCatchType(statement, varDeclName, undefined);
      });
    }
  }

  private checkTsLikeCatchType(
    node: ts.Node,
    variableDeclarationName: string,
    typeNode: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined
  ): void {
    if (!node) {
      return;
    }
    const hasChecked = this.hasCheckedTsLikeCatchTypeInIfStatement(node, variableDeclarationName, typeNode);
    if (hasChecked) {
      return;
    }
    const hasCheckedInConditionalExpr = this.hasCheckedTsLikeCatchTypeInConditionalExpression(
      node,
      variableDeclarationName,
      typeNode
    );
    if (hasCheckedInConditionalExpr) {
      return;
    }
    this.checkTsLikeCatchTypeForAsExpr(node, variableDeclarationName);

    for (const child of node.getChildren()) {
      if (ts.isPropertyAccessExpression(child)) {
        this.checkTsLikeCatchTypeForPropAccessExpr(child, variableDeclarationName, typeNode);

        if (
          ts.isParenthesizedExpression(child.expression) &&
          ts.isAsExpression(child.expression.expression) &&
          child.expression.expression.expression.getText() === variableDeclarationName
        ) {
          this.checkTsLikeCatchTypePropForAsExpression(child, child.expression.expression);
        }
      }
      this.checkTsLikeCatchType(child, variableDeclarationName, typeNode);
    }
  }

  private hasCheckedTsLikeCatchTypeInIfStatement(
    node: ts.Node,
    variableDeclarationName: string,
    typeNode: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined
  ): boolean {
    const checkSubStatement = (node: ts.IfStatement, declaration: ts.ClassDeclaration): void => {
      if (!this.isErrorOrInheritError(declaration)) {
        this.incrementCounters(node.expression, FaultID.TsLikeCatchType);
      } else {
        this.checkTsLikeCatchType(node.thenStatement, variableDeclarationName, declaration);
      }
      const elseStatement = node.elseStatement;
      if (elseStatement) {
        this.checkTsLikeCatchType(elseStatement, variableDeclarationName, typeNode);
      }
    };

    if (
      ts.isIfStatement(node) &&
      ts.isBinaryExpression(node.expression) &&
      node.expression.operatorToken.kind === ts.SyntaxKind.InstanceOfKeyword &&
      node.expression.left.getText() === variableDeclarationName
    ) {
      const rightSym = this.tsTypeChecker.getSymbolAtLocation(node.expression.right);
      const decl = rightSym?.declarations?.[0];
      if (decl && ts.isClassDeclaration(decl)) {
        checkSubStatement(node, decl);
        return true;
      }
      if (decl && ts.isImportSpecifier(decl)) {
        const symbol = this.getSymbolByImportSpecifier(decl);
        const declaration = symbol?.declarations?.[0];
        if (declaration && ts.isClassDeclaration(declaration)) {
          checkSubStatement(node, declaration);
          return true;
        }
      }
    }
    return false;
  }

  private hasCheckedTsLikeCatchTypeInConditionalExpression(
    node: ts.Node,
    variableDeclarationName: string,
    typeNode: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined
  ): boolean {
    if (
      ts.isConditionalExpression(node) &&
      ts.isBinaryExpression(node.condition) &&
      node.condition.operatorToken.kind === ts.SyntaxKind.InstanceOfKeyword &&
      node.condition.left.getText() === variableDeclarationName
    ) {
      const rightSym = this.tsTypeChecker.getSymbolAtLocation(node.condition.right);
      const decl = rightSym?.declarations?.[0];
      if (decl && ts.isClassDeclaration(decl)) {
        this.checkTsLikeCatchTypeInConditionalExprSubStatement(node, decl, variableDeclarationName, typeNode);
        return true;
      } else if (decl && ts.isImportSpecifier(decl)) {
        const symbol = this.getSymbolByImportSpecifier(decl);
        const declaration = symbol?.declarations?.[0];
        if (declaration && ts.isClassDeclaration(declaration)) {
          this.checkTsLikeCatchTypeInConditionalExprSubStatement(node, declaration, variableDeclarationName, typeNode);
          return true;
        }
      }
    }
    return false;
  }

  private checkTsLikeCatchTypeInConditionalExprSubStatement(
    node: ts.ConditionalExpression,
    declarationType: ts.ClassDeclaration,
    variableDeclarationName: string,
    typeNode: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined
  ): void {
    const checkWhenFalseExpr = (
      whenFalse: ts.Node,
      typeNode: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined
    ): void => {
      if (ts.isPropertyAccessExpression(whenFalse) && whenFalse.expression.getText() === variableDeclarationName) {
        if (!typeNode) {
          if (!ERROR_PROP_LIST.has(whenFalse.name.getText())) {
            this.incrementCounters(whenFalse, FaultID.TsLikeCatchType);
          }
        } else {
          const isValidErrorPropAccess = this.isValidErrorPropAccess(whenFalse, typeNode);
          if (!isValidErrorPropAccess) {
            this.incrementCounters(whenFalse, FaultID.TsLikeCatchType);
          }
        }
      } else {
        this.checkTsLikeCatchType(whenFalse, variableDeclarationName, typeNode);
      }
    };

    if (!this.isErrorOrInheritError(declarationType)) {
      this.incrementCounters(node.condition, FaultID.TsLikeCatchType);
      checkWhenFalseExpr(node.whenFalse, typeNode);
    } else {
      if (
        ts.isPropertyAccessExpression(node.whenTrue) &&
        node.whenTrue.expression.getText() === variableDeclarationName
      ) {
        const whenTrue: ts.PropertyAccessExpression = node.whenTrue;
        const isValidErrorPropAccess = this.isValidErrorPropAccess(whenTrue, declarationType);
        if (!isValidErrorPropAccess) {
          this.incrementCounters(whenTrue, FaultID.TsLikeCatchType);
        }
      } else {
        this.checkTsLikeCatchType(node.whenTrue, variableDeclarationName, declarationType);
      }
      checkWhenFalseExpr(node.whenFalse, typeNode);
    }
  }

  private checkTsLikeCatchTypeForAsExpr(node: ts.Node, variableDeclarationName: string): void {
    if (!ts.isAsExpression(node) || node.expression.getText() !== variableDeclarationName) {
      return;
    }
    const asExprTypeNode = node.type;
    if (!asExprTypeNode || !ts.isTypeReferenceNode(asExprTypeNode)) {
      return;
    }
    const checkReport = (node: ts.AsExpression, declaration: ts.ClassDeclaration | ts.InterfaceDeclaration): void => {
      if (!this.isErrorOrInheritError(declaration)) {
        this.incrementCounters(node, FaultID.TsLikeCatchType);
      }
    };

    const checkImportSpecifier = (decl: ts.ImportSpecifier): void => {
      const symbol = this.getSymbolByImportSpecifier(decl);
      const declaration = symbol?.declarations?.[0];
      if (declaration && (ts.isClassDeclaration(declaration) || ts.isInterfaceDeclaration(declaration))) {
        checkReport(node, declaration);
      }
    };
    const typeName = asExprTypeNode.typeName;
    const sym = this.tsTypeChecker.getSymbolAtLocation(typeName);
    const decl = sym?.declarations?.[0];
    if (decl && (ts.isClassDeclaration(decl) || ts.isInterfaceDeclaration(decl))) {
      checkReport(node, decl);
    } else if (decl && ts.isImportSpecifier(decl)) {
      checkImportSpecifier(decl);
    }
  }

  private checkTsLikeCatchTypeHasPropInType(
    propAccessExpr: ts.PropertyAccessExpression,
    decl: ts.ClassDeclaration | ts.InterfaceDeclaration
  ): void {
    if (!decl) {
      return;
    }
    if (this.isErrorOrInheritError(decl)) {
      const isValidErrorPropAccess = this.isValidErrorPropAccess(propAccessExpr, decl);
      if (!isValidErrorPropAccess) {
        this.incrementCounters(propAccessExpr, FaultID.TsLikeCatchType);
      }
    }
  }

  private checkTsLikeCatchTypeForPropAccessExpr(
    propAccessExpr: ts.PropertyAccessExpression,
    variableDeclarationName: string,
    typeNode: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined
  ): void {
    const checkProp = (): void => {
      if (!typeNode) {
        if (!ERROR_PROP_LIST.has(propAccessExpr.name.getText())) {
          this.incrementCounters(propAccessExpr, FaultID.TsLikeCatchType);
        }
      } else {
        const isValidErrorPropAccess = this.isValidErrorPropAccess(propAccessExpr, typeNode);
        if (!isValidErrorPropAccess) {
          this.incrementCounters(propAccessExpr, FaultID.TsLikeCatchType);
        }
      }
    };

    if (propAccessExpr.expression.getText() === variableDeclarationName) {
      checkProp();
      return;
    }

    const sym = this.tsTypeChecker.getSymbolAtLocation(propAccessExpr.expression);
    const decl = sym?.declarations?.[0];
    if (decl && ts.isVariableDeclaration(decl) && decl.initializer) {
      if (decl.initializer.getText() === variableDeclarationName) {
        checkProp();
        return;
      }
      if (ts.isAsExpression(decl.initializer) && decl.initializer.expression.getText() === variableDeclarationName) {
        this.checkTsLikeCatchTypePropForAsExpression(propAccessExpr, decl.initializer);
      }
    }
  }

  private checkTsLikeCatchTypePropForAsExpression(
    propAccessExpr: ts.PropertyAccessExpression,
    asExpr: ts.AsExpression
  ): void {
    const asExprTypeNode = asExpr.type;
    if (asExprTypeNode && ts.isTypeReferenceNode(asExprTypeNode)) {
      const typeName = asExprTypeNode.typeName;
      const sym = this.tsTypeChecker.getSymbolAtLocation(typeName);
      const decl = sym?.declarations?.[0];
      if (decl && (ts.isClassDeclaration(decl) || ts.isInterfaceDeclaration(decl))) {
        this.checkTsLikeCatchTypeHasPropInType(propAccessExpr, decl);
      } else if (decl && ts.isImportSpecifier(decl)) {
        const symbol = this.getSymbolByImportSpecifier(decl);
        const declaration = symbol?.declarations?.[0];
        if (declaration && (ts.isClassDeclaration(declaration) || ts.isInterfaceDeclaration(declaration))) {
          this.checkTsLikeCatchTypeHasPropInType(propAccessExpr, declaration);
        }
      }
    }
  }

  private isErrorOrInheritError(declaration: ts.ClassDeclaration | ts.InterfaceDeclaration): boolean {
    const type = this.tsTypeChecker.getTypeAtLocation(declaration);
    return this.tsUtils.isOrDerivedFrom(type, this.tsUtils.isStdErrorType);
  }

  private isValidErrorPropAccess(
    propertyAccessExpr: ts.PropertyAccessExpression,
    decl: ts.ClassDeclaration | ts.InterfaceDeclaration | undefined
  ): boolean {
    void this;
    let containsMember = false;
    decl?.members.forEach((member) => {
      if (member.name?.getText() === propertyAccessExpr.name.getText()) {
        containsMember = true;
      }
    });
    return containsMember || ERROR_PROP_LIST.has(propertyAccessExpr.name.getText());
  }

  private getSymbolByImportSpecifier(declaration: ts.ImportSpecifier): ts.Symbol | undefined {
    if (!declaration?.parent?.parent) {
      return undefined;
    }
    if (!ts.isImportClause(declaration.parent.parent)) {
      return undefined;
    }
    const importClause = declaration.parent.parent;
    const namedBindings = importClause.namedBindings;
    let symbol: ts.Symbol | undefined;
    if (namedBindings) {
      if (ts.isNamedImports(namedBindings) && namedBindings.elements?.length > 0) {
        for (let i = 0; i < namedBindings.elements.length; i++) {
          if (namedBindings.elements[i].name.getText() === declaration.name.getText()) {
            symbol = this.tsUtils.trueSymbolAtLocation(namedBindings.elements[i].name);
            break;
          }
        }
      } else if (ts.isNamespaceImport(namedBindings)) {
        symbol = this.tsUtils.trueSymbolAtLocation(namedBindings.name);
      }
    }
    return symbol;
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
    this.handleLocalDeclarationOfClassAndIface(tsClassDecl);
    this.checkObjectPublicApiMethods(tsClassDecl);
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

  private handleTemplateType(node: ts.TemplateLiteralTypeNode): void {
    if (!this.options.arkts2) {
      return;
    }
    this.incrementCounters(node, FaultID.TemplateStringType);
  }

  private handleTupleType(node: ts.TupleTypeNode): void {
    if (!this.options.arkts2) {
      return;
    }

    this.checkOptionalTupleType(node);

    node.elements.forEach((elementType) => {
      if (elementType.kind === ts.SyntaxKind.VoidKeyword) {
        this.incrementCounters(elementType, FaultID.LimitedVoidType);
      }
    });
  }

  private checkOptionalTupleType(node: ts.TupleTypeNode): void {
    node.elements.forEach((elementType) => {
      if (elementType.kind === ts.SyntaxKind.OptionalType) {
        this.incrementCounters(elementType, FaultID.OptionalTupleType);
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
    if (this.options.arkts2) {
      this.handleParamType(tsMethodDecl);
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
    this.handleNoDeprecatedApi(tsMethodDecl);
    this.checkAbstractOverrideReturnType(tsMethodDecl);
  }

  private checkObjectPublicApiMethods(node: ts.ClassDeclaration | ts.InterfaceDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }
    for (const member of node.members) {
      if (!((ts.isMethodDeclaration(member) || ts.isMethodSignature(member)) && ts.isIdentifier(member.name))) {
        continue;
      }
      const methodName = member.name.text;
      const expectedSignature = OBJECT_PUBLIC_API_METHOD_SIGNATURES.get(methodName);
      if (!expectedSignature) {
        continue;
      }
      const methodType = this.tsTypeChecker.getTypeAtLocation(member);
      const signature = TsUtils.getFunctionalTypeSignature(methodType);
      if (!signature) {
        continue;
      }
      const actualSignature = this.tsTypeChecker.signatureToString(signature);
      if (actualSignature !== expectedSignature) {
        this.incrementCounters(member, FaultID.NoSignatureDistinctWithObjectPublicApi);
      }
    }
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
      this.handleMethodInheritForCommonApi(node);
      return;
    }
    const isStatic =
      node.modifiers?.some((mod) => {
        return mod.kind === ts.SyntaxKind.StaticKeyword;
      }) || false;
    const classType: ts.Type | undefined = this.getClassType(classDecl, isStatic);
    const allBaseTypes = classType && this.getAllBaseTypes(classType, classDecl, isStatic);

    if (!allBaseTypes || allBaseTypes.length === 0) {
      return;
    }
    const methodName = node.name.text;
    if (allBaseTypes && allBaseTypes.length > 0) {
      this.checkMethodType(allBaseTypes, methodName, node, isStatic);
    }
    this.checkIncompatibleFunctionTypes(node);
  }

  private handleMethodInheritForCommonApi(node: ts.MethodDeclaration): void {
    const commonApiInfos = getCommonApiInfoMap();
    commonApiInfos?.forEach((apiNode) => {
      if (node.name.getText() === apiNode.name.getText()) {
        this.checkMethodParameters(node, apiNode);
        this.checkMethodReturnType(node, apiNode);
      }
    });
  }

  private checkMethodType(
    allBaseTypes: ts.Type[],
    methodName: string,
    node: ts.MethodDeclaration,
    isStatic: boolean = false
  ): void {
    for (const baseType of allBaseTypes) {
      let baseMethod: ts.Symbol | undefined;
      const symbol = baseType.getSymbol();
      if (isStatic && symbol) {
        const constructorType = this.tsTypeChecker.getTypeOfSymbolAtLocation(symbol, node);
        baseMethod =
          constructorType.getProperty(methodName) || symbol.members?.get(ts.escapeLeadingUnderscores(methodName));
      } else {
        baseMethod = baseType.getProperty(methodName);
      }
      if (!baseMethod) {
        continue;
      }
      const baseMethodDecl = baseMethod.declarations?.find((d) => {
        return (
          (ts.isMethodDeclaration(d) || ts.isMethodSignature(d)) &&
          this.isSameDeclarationType(d.parent, baseType, isStatic)
        );
      }) as ts.MethodDeclaration | ts.MethodSignature;

      if (!baseMethodDecl) {
        continue;
      }

      this.checkMethodParameters(node, baseMethodDecl);

      this.checkMethodReturnType(node, baseMethodDecl);

      break;
    }
  }

  private isSameDeclarationType(decl: ts.Node, type: ts.Type, isStatic: boolean): boolean {
    if (isStatic && ts.isClassDeclaration(decl) || ts.isInterfaceDeclaration(decl)) {
      const staticType = this.tsTypeChecker.getTypeAtLocation(decl);
      return this.isSameType(staticType, type);
    }
    return this.tsTypeChecker.getTypeAtLocation(decl) === type;
  }

  private checkIncompatibleFunctionTypes(method: ts.MethodDeclaration): void {
    const declaredReturnType = this.getActualReturnType(method);
    if (!declaredReturnType) {
      return;
    }
    const returnStatements = this.collectReturnStatements(method);
    const declaredReturnTypeStr = this.tsTypeChecker.typeToString(declaredReturnType);
    for (const returnStmt of returnStatements) {
      if (!returnStmt.expression) {
        continue;
      }
      const actualReturnType = this.tsTypeChecker.getTypeAtLocation(returnStmt.expression);
      const actualReturnTypeStr = this.tsTypeChecker.typeToString(actualReturnType);
      if (declaredReturnTypeStr === actualReturnTypeStr) {
        return;
      }
      if (this.tsUtils.skipCheckForArrayBufferLike(declaredReturnTypeStr, actualReturnTypeStr)) {
        return;
      }
      if (actualReturnType.flags & ts.TypeFlags.Any || declaredReturnType.flags & ts.TypeFlags.Any) {
        return;
      }
      if (this.isSubtypeByBaseTypesList(actualReturnType, declaredReturnType)) {
        this.incrementCounters(returnStmt.expression, FaultID.IncompationbleFunctionType);
        return;
      }
    }
  }

  private collectReturnStatements(node: ts.Node): ts.ReturnStatement[] {
    const result: ts.ReturnStatement[] = [];

    ts.forEachChild(node, (child) => {
      if (ts.isReturnStatement(child)) {
        result.push(child);
      } else {
        result.push(...this.collectReturnStatements(child));
      }
    });

    return result;
  }

  private getClassType(classDecl: ts.ClassDeclaration, isStatic?: boolean): ts.Type | undefined {
    let classType: ts.Type;

    if (isStatic) {
      const classConstructorSymbol = classDecl.symbol;
      if (!classConstructorSymbol) {
        return undefined;
      }
      classType = this.tsTypeChecker.getTypeOfSymbolAtLocation(classConstructorSymbol, classDecl);
    } else {
      classType = this.tsTypeChecker.getTypeAtLocation(classDecl);
    }
    return classType;
  }

  private isDeclarationInType(decl: ts.Declaration, type: ts.Type, isStatic: boolean = false): boolean {
    const declParent = decl.parent;
    if (!declParent) {
      return false;
    }

    let declParentType: ts.Type;
    if (isStatic && ts.isClassDeclaration(declParent)) {
      if (!declParent.symbol) {
        return false;
      }
      declParentType = this.tsTypeChecker.getTypeOfSymbolAtLocation(declParent.symbol, declParent);
    } else {
      declParentType = this.tsTypeChecker.getTypeAtLocation(declParent);
    }

    return this.isSameType(declParentType, type);
  }

  private isSameType(type1: ts.Type, type2: ts.Type): boolean {
    if (type1.flags & ts.TypeFlags.Any || type2.flags & ts.TypeFlags.Any) {
      return true;
    }

    if (type1.flags & ts.TypeFlags.TypeParameter && type2.flags & ts.TypeFlags.TypeParameter) {
      const constraint1 = (type1 as ts.TypeParameter).getConstraint();
      const constraint2 = (type2 as ts.TypeParameter).getConstraint();
      if (constraint1 && constraint2) {
        return this.isSameType(constraint1, constraint2);
      }
    }

    if (!type1.symbol || type1.symbol !== type2.symbol) {
      return false;
    }
    const type1Args = (type1 as ts.TypeReference).typeArguments;
    const type2Args = (type2 as ts.TypeReference).typeArguments;

    if (type1Args && type2Args && type1Args.length === type2Args.length) {
      for (let i = 0; i < type1Args.length; i++) {
        if (!this.isTypeAssignable(type2Args[i], type1Args[i])) {
          return false;
        }
      }
      return true;
    }

    return this.tsTypeChecker.typeToString(type1) === this.tsTypeChecker.typeToString(type2);
  }

  private getAllBaseTypes(type: ts.Type, classDecl: ts.ClassDeclaration, isStatic?: boolean): ts.Type[] | undefined {
    if (isStatic) {
      return this.getStaticAllBaseTypes(classDecl);
    }

    const baseClasses = type.getBaseTypes() || [];
    const resolvedBaseClasses = baseClasses.flatMap((baseType) => {
      const symbol = baseType.getSymbol();
      return symbol ? [this.tsTypeChecker.getDeclaredTypeOfSymbol(symbol)] : [baseType];
    });

    if (!classDecl.heritageClauses) {
      return resolvedBaseClasses;
    }

    const interfaces: ts.Type[] = [];
    for (const clause of classDecl.heritageClauses) {
      if (clause.token !== ts.SyntaxKind.ImplementsKeyword) {
        continue;
      }
      for (const typeNode of clause.types) {
        const interfaceType = this.tsTypeChecker.getTypeAtLocation(typeNode);
        interfaces.push(interfaceType);

        const baseInterfaces = interfaceType.getBaseTypes() || [];
        baseInterfaces.forEach((baseInterface) => {
          const symbol = baseInterface.getSymbol();
          if (symbol) {
            interfaces.push(this.tsTypeChecker.getDeclaredTypeOfSymbol(symbol));
          }
        });
      }
    }
    return [...resolvedBaseClasses, ...interfaces];
  }

  private getStaticAllBaseTypes(classDecl: ts.ClassDeclaration): ts.Type[] | undefined {
    const baseTypes: ts.Type[] = [];
    if (!classDecl.heritageClauses) {
      return baseTypes;
    }

    for (const clause of classDecl.heritageClauses) {
      if (clause.token !== ts.SyntaxKind.ExtendsKeyword) {
        continue;
      }

      for (const typeNode of clause.types) {
        const baseType = this.tsTypeChecker.getTypeAtLocation(typeNode);
        baseTypes.push(baseType);

        const baseDecl = baseType.getSymbol()?.declarations?.[0];
        if (baseDecl && ts.isClassDeclaration(baseDecl)) {
          const staticBaseType = this.tsTypeChecker.getTypeAtLocation(baseDecl);
          const staticBaseTypes = this.getAllBaseTypes(staticBaseType, baseDecl, true) || [];
          baseTypes.push(...staticBaseTypes);
        }
      }
    }
    return baseTypes;
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

    for (let i = 0; i < Math.min(derivedParams.length, baseParams.length); i++) {
      const baseParam = baseParams[i];
      const derivedParam = derivedParams[i];

      if (!baseParam.questionToken && derivedParam.questionToken) {
        this.incrementCounters(derivedParam, FaultID.MethodInheritRule);
        return;
      }
    }

    if (derivedParams.length !== baseParams.length) {
      this.incrementCounters(derivedMethod.name, FaultID.MethodInheritRule);
      return;
    }

    const paramCount = Math.min(derivedParams.length, baseParams.length);

    for (let i = 0; i < paramCount; i++) {
      const baseParamType = this.tsTypeChecker.getTypeAtLocation(baseParams[i]);
      const derivedParamType = this.tsTypeChecker.getTypeAtLocation(derivedParams[i]);

      if (baseParamType.flags & ts.TypeFlags.TypeParameter) {
        if (!(derivedParamType.flags & ts.TypeFlags.TypeParameter)) {
          continue;
        }
      }

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
    if (this.shouldSkipTypeParameterCheck(derivedMethod, baseMethod)) {
      return;
    }
    const baseMethodType = this.getActualReturnType(baseMethod);
    const derivedMethodType = this.getActualReturnType(derivedMethod);
    const baseMethodTypeIsVoid = TypeScriptLinter.checkMethodTypeIsVoidOrAny(baseMethodType, true);
    const baseMethodTypeisAny = TypeScriptLinter.checkMethodTypeIsVoidOrAny(baseMethodType, false);
    const derivedMethodTypeIsVoid = TypeScriptLinter.checkMethodTypeIsVoidOrAny(derivedMethodType, true, true);
    const baseMethodTypeisAnyWithVoid = TypeScriptLinter.getRelationBaseMethodAndDerivedMethod(
      baseMethodTypeisAny,
      derivedMethodTypeIsVoid
    );
    const baseMethodTypeisAnyWithPromiseVoid = TypeScriptLinter.getRelationBaseMethodAndDerivedMethod(
      baseMethodTypeisAny,
      this.hasPromiseVoidReturn(derivedMethod)
    );
    const baseMethodTypeIsVoidWithoutVoid = TypeScriptLinter.getRelationBaseMethodAndDerivedMethod(
      baseMethodTypeIsVoid,
      !derivedMethodTypeIsVoid
    );
    const baseMethodTypeisAnyWithoutVoid = TypeScriptLinter.getRelationBaseMethodAndDerivedMethod(
      baseMethodTypeisAny,
      !derivedMethodTypeIsVoid
    );
    const baseMethodTypeIsVoidWithVoid = TypeScriptLinter.getRelationBaseMethodAndDerivedMethod(
      baseMethodTypeIsVoid,
      derivedMethodTypeIsVoid
    );
    if (baseMethodTypeisAnyWithVoid || baseMethodTypeIsVoidWithoutVoid || baseMethodTypeisAnyWithPromiseVoid) {
      this.incrementCounters(derivedMethod.type ? derivedMethod.type : derivedMethod.name, FaultID.MethodInheritRule);
      return;
    }
    const isNoNeedCheck =
      !baseMethodType || !derivedMethodType || baseMethodTypeisAnyWithoutVoid || baseMethodTypeIsVoidWithVoid;
    if (isNoNeedCheck) {
      return;
    }
    if (this.isDerivedTypeAssignable(derivedMethodType, baseMethodType)) {
      return;
    }
    if (!this.isTypeAssignable(derivedMethodType, baseMethodType)) {
      this.incrementCounters(derivedMethod.type ? derivedMethod.type : derivedMethod.name, FaultID.MethodInheritRule);
    }
  }

  private shouldSkipTypeParameterCheck(
    derivedMethod: ts.MethodDeclaration,
    baseMethod: ts.MethodDeclaration | ts.MethodSignature
  ): boolean {
    const baseMethodType = this.getActualReturnType(baseMethod);
    const derivedMethodType = this.getActualReturnType(derivedMethod);

    if (baseMethodType && baseMethodType.flags & ts.TypeFlags.TypeParameter) {
      if (derivedMethodType && !(derivedMethodType.flags & ts.TypeFlags.TypeParameter)) {
        return true;
      }
    }
    return false;
  }

  private static checkMethodTypeIsVoidOrAny(
    methodType: ts.Type | undefined,
    isVoidOrAny: boolean,
    isDerived?: boolean
  ): boolean | ts.TypeNode | undefined {
    if (isDerived && isVoidOrAny) {
      return methodType && TsUtils.isVoidType(methodType);
    } else if (isVoidOrAny) {
      return methodType && TsUtils.isVoidType(methodType);
    }
    return methodType && TsUtils.isAnyType(methodType);
  }

  private static getRelationBaseMethodAndDerivedMethod(
    baseMethodTypeIsVoidOrAny: boolean | ts.TypeNode | undefined,
    derivedMethodCheckFlag: boolean | ts.TypeNode | undefined
  ): boolean | ts.TypeNode | undefined {
    return baseMethodTypeIsVoidOrAny && derivedMethodCheckFlag;
  }

  private getActualReturnType(method: ts.MethodDeclaration | ts.MethodSignature): ts.Type | undefined {
    let type: ts.Type | undefined;
    if (method.type) {
      type = this.tsTypeChecker.getTypeAtLocation(method.type);
    } else {
      const signature = this.tsTypeChecker.getSignatureFromDeclaration(method);
      if (signature) {
        type = this.tsTypeChecker.getReturnTypeOfSignature(signature);
      }
    }
    return type;
  }

  private isTypeSameOrWider(baseType: ts.Type, derivedType: ts.Type): boolean {
    if (this.tsTypeChecker.typeToString(baseType) === this.tsTypeChecker.typeToString(derivedType)) {
      return true;
    }

    if (derivedType.flags & ts.TypeFlags.Any || baseType.flags & ts.TypeFlags.Never) {
      return true;
    }

    if (baseType.symbol === derivedType.symbol && baseType.symbol) {
      const baseArgs = (baseType as ts.TypeReference).typeArguments;
      const derivedArgs = (derivedType as ts.TypeReference).typeArguments;

      if (!baseArgs || !derivedArgs || baseArgs.length !== derivedArgs.length) {
        return false;
      }
      for (let i = 0; i < baseArgs.length; i++) {
        if (!this.isTypeAssignable(baseArgs[i], derivedArgs[i])) {
          return false;
        }
      }
      return true;
    }

    if (this.checkTypeInheritance(derivedType, baseType, false)) {
      return true;
    }

    const baseTypeSet = new Set(this.flattenUnionTypes(baseType));
    const derivedTypeSet = new Set(this.flattenUnionTypes(derivedType));

    for (const typeStr of baseTypeSet) {
      if (!derivedTypeSet.has(typeStr)) {
        if (TypeScriptLinter.areWrapperAndPrimitiveTypesEqual(typeStr, derivedTypeSet)) {
          continue;
        }
        return false;
      }
    }
    return true;
  }

  private isTypeAssignable(fromType: ts.Type, toType: ts.Type): boolean {
    if (fromType.flags & ts.TypeFlags.Any) {
      return true;
    }

    if (fromType.symbol === toType.symbol && fromType.symbol) {
      const fromArgs = (fromType as ts.TypeReference).typeArguments;
      const toArgs = (toType as ts.TypeReference).typeArguments;

      if (fromArgs && toArgs && fromArgs.length === toArgs.length) {
        for (let i = 0; i < fromArgs.length; i++) {
          if (!this.isTypeAssignable(fromArgs[i], toArgs[i])) {
            return false;
          }
        }
        return true;
      }
    }

    if (this.checkTypeInheritance(fromType, toType)) {
      return true;
    }

    const fromTypes = this.flattenUnionTypes(fromType);
    const toTypes = new Set(this.flattenUnionTypes(toType));

    return fromTypes.every((typeStr) => {
      if (toTypes.has(typeStr)) {
        return true;
      }
      return TypeScriptLinter.areWrapperAndPrimitiveTypesEqual(typeStr, toTypes);
    });
  }

  private checkTypeInheritance(sourceType: ts.Type, targetType: ts.Type, isSouceTotaqrget: boolean = true): boolean {
    // Early return if either type lacks symbol information
    if (!sourceType.symbol || !targetType.symbol) {
      return false;
    }

    // Determine which type's inheritance chain to examine based on check direction
    const typeToGetChain = isSouceTotaqrget ? sourceType : targetType;
    const typeToCheck = isSouceTotaqrget ? targetType : sourceType;

    // Get inheritance chain and check for relationship
    const inheritanceChain = this.getTypeInheritanceChain(typeToGetChain);
    return inheritanceChain.some((t) => {
      return t.symbol === typeToCheck.symbol;
    });
  }

  private getTypeInheritanceChain(type: ts.Type): ts.Type[] {
    const chain: ts.Type[] = [type];
    const declarations = type.symbol?.getDeclarations() || [];

    for (const declaration of declarations) {
      if (
        !ts.isClassDeclaration(declaration) && !ts.isInterfaceDeclaration(declaration) ||
        !declaration.heritageClauses
      ) {
        continue;
      }

      const heritageClauses = declaration.heritageClauses.filter((clause) => {
        return clause.token === ts.SyntaxKind.ExtendsKeyword || clause.token === ts.SyntaxKind.ImplementsKeyword;
      });

      for (const clause of heritageClauses) {
        for (const typeExpr of clause.types) {
          const baseType = this.tsTypeChecker.getTypeAtLocation(typeExpr.expression);
          chain.push(baseType, ...this.getTypeInheritanceChain(baseType));
        }
      }
    }

    return chain;
  }

  // Check if a type string has an equivalent primitive/wrapper type in a set
  private static areWrapperAndPrimitiveTypesEqual(typeStr: string, typeSet: Set<string>): boolean {
    const typePairs = [
      ['String', 'string'],
      ['Number', 'number'],
      ['Boolean', 'boolean']
    ];

    for (const [wrapper, primitive] of typePairs) {
      if (typeStr === wrapper && typeSet.has(primitive) || typeStr === primitive && typeSet.has(wrapper)) {
        return true;
      }
    }
    return false;
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

    if (
      baseTypeNode &&
      derivedTypeNode &&
      ts.isClassDeclaration(baseTypeNode) &&
      ts.isClassDeclaration(derivedTypeNode)
    ) {
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
        return TypeScriptLinter.normalizeTypeString(this.tsTypeChecker.typeToString(t));
      });
    }
    return [TypeScriptLinter.normalizeTypeString(this.tsTypeChecker.typeToString(type))];
  }

  // Normalize type string to handle primitive wrapper types consistently
  private static normalizeTypeString(typeStr: string): string {
    // Handle all primitive wrapper types
    const wrapperToPrimitive: Record<string, string> = {
      String: 'string',
      Number: 'number',
      Boolean: 'boolean'
    };

    // Replace wrapper types with their primitive counterparts
    let normalized = typeStr;
    for (const [wrapper, primitive] of Object.entries(wrapperToPrimitive)) {
      normalized = normalized.replace(new RegExp(wrapper, 'g'), primitive);
    }
    return normalized;
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
    this.checkCollectionsSymbol(node);
    this.handleInterfaceImport(node);
    this.checkAsonSymbol(node);
    const tsIdentifier = node;
    this.handleTsInterop(tsIdentifier, () => {
      const parent = tsIdentifier.parent;
      if (ts.isImportSpecifier(parent)) {
        return;
      }
      const type = this.tsTypeChecker.getTypeAtLocation(tsIdentifier);
      this.checkUsageOfTsTypes(type, tsIdentifier);
    });

    const tsIdentSym = this.tsUtils.trueSymbolAtLocation(tsIdentifier);
    if (!tsIdentSym) {
      return;
    }

    const isNewArkTS = this.options.arkts2;
    if (isNewArkTS) {
      this.checkWorkerSymbol(tsIdentSym, node);
      this.checkConcurrencySymbol(tsIdentSym, node);
    }

    const isGlobalThis = tsIdentifier.text === 'globalThis';
    if (
      isGlobalThis &&
      (tsIdentSym.flags & ts.SymbolFlags.Module) !== 0 &&
      (tsIdentSym.flags & ts.SymbolFlags.Transient) !== 0
    ) {
      this.handleGlobalThisCase(tsIdentifier, isNewArkTS);
    } else {
      if (isNewArkTS) {
        this.checkLimitedStdlibApi(tsIdentifier, tsIdentSym);
      }
      this.handleRestrictedValues(tsIdentifier, tsIdentSym);
    }

    if (isNewArkTS && this.tsTypeChecker.isArgumentsSymbol(tsIdentSym)) {
      this.incrementCounters(node, FaultID.ArgumentsObject);
    }
    this.checkInvalidNamespaceUsage(node);
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

  private checkInvalidNamespaceUsage(node: ts.Identifier): void {
    if (!this.options.arkts2) {
      return;
    }
    if (ts.isNamespaceImport(node.parent)) {
      return;
    }
    const symbol = this.tsTypeChecker.getSymbolAtLocation(node);
    if (!symbol) {
      return;
    }
    const isNamespace = symbol.declarations?.some((decl) => {
      return ts.isNamespaceImport(decl);
    });
    if (!isNamespace) {
      return;
    }
    const parent = node.parent;
    const isValidUsage = ts.isPropertyAccessExpression(parent) && parent.expression === node;
    if (!isValidUsage) {
      this.incrementCounters(node, FaultID.NoImportNamespaceStarAsVar);
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

    if (this.options.arkts2 && this.tsUtils.isOrDerivedFrom(tsElemAccessBaseExprType, TsUtils.isTuple)) {
      this.handleTupleIndex(tsElementAccessExpr);
    }

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
    this.handleNoDeprecatedApi(tsElementAccessExpr);
  }

  private handleTupleIndex(expr: ts.ElementAccessExpression): void {
    const value = expr.argumentExpression;

    if (this.isArgumentConstDotZero(value)) {
      this.incrementCounters(expr as ts.Node, FaultID.TupleIndex);
      return;
    }

    if (ts.isNumericLiteral(value)) {
      const indexText = value.getText();
      const indexValue = Number(indexText);
      const isValid = Number.isInteger(indexValue) && indexValue >= 0;

      if (!isValid) {
        this.incrementCounters(expr as ts.Node, FaultID.TupleIndex);
      }
      return;
    }

    if (ts.isPrefixUnaryExpression(value)) {
      const { operator, operand } = value;
      const resolved = this.evaluateValueFromDeclaration(operand);

      if (typeof resolved === 'number') {
        const final = operator === ts.SyntaxKind.MinusToken ? -resolved : resolved;
        const isValid = Number.isInteger(final) && final >= 0;
        if (!isValid) {
          this.incrementCounters(expr as ts.Node, FaultID.TupleIndex);
        }
        return;
      }
      this.incrementCounters(expr as ts.Node, FaultID.TupleIndex);
      return;
    }

    const resolved = this.evaluateValueFromDeclaration(value);
    if (typeof resolved === 'number') {
      const isValid = Number.isInteger(resolved) && resolved >= 0;
      if (!isValid) {
        this.incrementCounters(expr as ts.Node, FaultID.TupleIndex);
      }
      return;
    }

    this.incrementCounters(expr as ts.Node, FaultID.TupleIndex);
  }

  private isArgumentConstDotZero(expr: ts.Expression): boolean {
    if (ts.isNumericLiteral(expr)) {
      return expr.getText().endsWith('.0');
    }

    if (ts.isPrefixUnaryExpression(expr) && ts.isNumericLiteral(expr.operand)) {
      return expr.operand.getText().endsWith('.0');
    }

    if (ts.isIdentifier(expr)) {
      const declaration = this.tsUtils.getDeclarationNode(expr);
      if (declaration && ts.isVariableDeclaration(declaration) && declaration.initializer) {
        const init = declaration.initializer;

        if (ts.isNumericLiteral(init)) {
          return init.getText().endsWith('.0');
        }

        if (ts.isPrefixUnaryExpression(init) && ts.isNumericLiteral(init.operand)) {
          return init.operand.getText().endsWith('.0');
        }
      }
    }

    return false;
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
    const containsDot = initializerText.includes('.');

    if (containsDot || isUnsafeNumber || initializerText === 'undefined') {
      const autofix = this.autofixer?.fixArrayIndexExprType(argExpr);
      this.incrementCounters(argExpr, FaultID.ArrayIndexExprType, autofix);
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

    if (declaration.type !== undefined && declaration.type.getText() !== NUMBER_LITERAL) {
      return 'skip';
    }

    const initializer = declaration.initializer;
    if (!initializer) {
      return null;
    }
    let numericValue: number | null = null;
    if (ts.isNumericLiteral(initializer)) {
      numericValue = Number(initializer.text);
    } else if (ts.isPrefixUnaryExpression(initializer) && ts.isNumericLiteral(initializer.operand)) {
      const rawValue = Number(initializer.operand.text);
      numericValue =
        initializer.operator === ts.SyntaxKind.MinusToken ?
          -rawValue :
          initializer.operator === ts.SyntaxKind.PlusToken ?
            rawValue :
            null;
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

    if (!Number.isInteger(evaluatedValue)) {
      return false;
    }

    const valueString = String(evaluatedValue);
    if (valueString.includes('.') && !valueString.endsWith('.0')) {
      return false;
    }

    return evaluatedValue >= 0;
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
    if (ts.isAsExpression(initializer) || ts.isTypeAssertionExpression(initializer)) {
      const typeNode = ts.isAsExpression(initializer) ? initializer.type : initializer.type;

      if (typeNode.kind === ts.SyntaxKind.NumberKeyword) {
        this.incrementCounters(enumMember, FaultID.EnumMemberNonConstInit);
        return;
      }
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
    } else if (parName) {
      this.handleSdkApiThisArgs(tsCallExpr, calleeSym, name, parName);
      this.handleSdkApiThisArgs(tsCallExpr, calleeSym, name, parName, true);
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
    if (ts.isCallExpression(tsCallExpr) && tsCallExpr.expression.kind === ts.SyntaxKind.SuperKeyword) {
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

  private handleCallExpression(callExpr: ts.CallExpression): void {
    this.checkSdkAbilityLifecycleMonitor(callExpr);
    this.handleCallExpressionForUI(callExpr);
    this.handleBuiltinCtorCallSignature(callExpr);
    this.handleSdkConstructorIfaceForCallExpression(callExpr);
    if (this.options.arkts2 && callExpr.typeArguments !== undefined) {
      this.handleSdkPropertyAccessByIndex(callExpr);
    }
    const calleeSym = this.tsUtils.trueSymbolAtLocation(callExpr.expression);
    const callSignature = this.tsTypeChecker.getResolvedSignature(callExpr);
    this.handleImportCall(callExpr);
    this.handleRequireCall(callExpr);
    if (calleeSym !== undefined) {
      this.processCalleeSym(calleeSym, callExpr);
    }
    if (callSignature !== undefined) {
      if (!this.tsUtils.isLibrarySymbol(calleeSym)) {
        this.handleStructIdentAndUndefinedInArgs(callExpr, callSignature);
        this.handleGenericCallWithNoTypeArgs(callExpr, callSignature);
      } else if (this.options.arkts2) {
        this.handleGenericCallWithNoTypeArgs(callExpr, callSignature);
      }
      this.handleNotsLikeSmartTypeOnCallExpression(callExpr, callSignature);
    }
    this.handleInteropForCallExpression(callExpr);
    this.handleLibraryTypeCall(callExpr);
    if (
      ts.isPropertyAccessExpression(callExpr.expression) &&
      this.tsUtils.hasEsObjectType(callExpr.expression.expression)
    ) {
      const faultId = this.options.arkts2 ? FaultID.EsValueTypeError : FaultID.EsValueType;
      this.incrementCounters(callExpr, faultId);
    }
    this.handleLimitedVoidWithCall(callExpr);
    this.fixJsImportCallExpression(callExpr);
    this.handleInteropForCallJSExpression(callExpr, calleeSym, callSignature);
    this.handleNoTsLikeFunctionCall(callExpr);
    this.handleObjectLiteralInFunctionArgs(callExpr);
    this.handleSdkGlobalApi(callExpr);
    this.handleObjectLiteralAssignmentToClass(callExpr);
    this.checkRestrictedAPICall(callExpr);
    this.handleNoDeprecatedApi(callExpr);
    this.handleFunctionReturnThisCall(callExpr);
    this.handlePromiseTupleGeneric(callExpr);
    this.isSelectOfArkUI(callExpr, callSignature);
    this.handleTupleGeneric(callExpr);
  }

  private isSelectOfArkUI(callExpr: ts.CallExpression, signature: ts.Signature | undefined): void {
    if (!this.options.arkts2) {
      return;
    }

    if (callExpr.expression.getText() !== SELECT_IDENTIFIER) {
      return;
    }

    /*
     * for some reason UI component methods signatures cannot be accessed through here,
     * there should be no signature declaration of this callExpression,
     * if there is signature declaration we will assume this is not an ArkUI component
     */
    if (signature?.getDeclaration()) {
      return;
    }

    const insideArkUi = this.isInComponentBlock(callExpr.getSourceFile());
    if (!insideArkUi) {
      return;
    }

    const args = callExpr.arguments;
    if (args.length !== 1) {
      return;
    }

    const arg = args[0];
    const argumentType = this.tsTypeChecker.getTypeAtLocation(arg);
    const argumentTypeString = this.tsTypeChecker.typeToString(argumentType);

    if (SELECT_OPTIONS.includes(argumentTypeString)) {
      return;
    }

    this.incrementCounters(arg, FaultID.StructuralIdentity);
  }

  private isInComponentBlock(sourceFile: ts.SourceFile): boolean {
    void this;
    let isInside = false;
    for (const statement of sourceFile.statements) {
      statement.forEachChild((node) => {
        if (node.getText() === COMPONENT_DECORATOR) {
          isInside = true;
        }
      });
      if (isInside) {
        break;
      }
    }

    return isInside;
  }

  private handleTupleGeneric(callExpr: ts.CallExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    if (!ts.isPropertyAccessExpression(callExpr.expression)) {
      return;
    }
    const accessedProperty = callExpr.expression;

    if (!ts.isIdentifier(accessedProperty.expression)) {
      return;
    }

    if (accessedProperty.expression.text !== TASKPOOL) {
      return;
    }

    if (!callExpr.typeArguments) {
      return;
    }

    if (callExpr.parent) {
      callExpr.typeArguments.forEach((node) => {
        if (ts.isTupleTypeNode(node)) {
          this.incrementCounters(node, FaultID.NotSupportTupleGenericValidation);
        }
      });
    }
  }

  private handleCallExpressionForUI(node: ts.CallExpression): void {
    this.handleStateStyles(node);
    this.handleCallExpressionForRepeat(node);
    this.handleNodeForWrappedBuilder(node);
    this.handleCallExpressionForSerialization(node);
  }

  handleNoTsLikeFunctionCall(callExpr: ts.CallExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    const expression = callExpr.expression;
    const type = this.tsTypeChecker.getTypeAtLocation(expression);
    const typeText = this.tsTypeChecker.typeToString(type);

    if (LIKE_FUNCTION !== typeText) {
      return;
    }
    if (ts.isNewExpression(expression) || ts.isCallExpression(expression)) {
      const exprIndentifier = expression.expression;
      const typeExprIndent = this.tsTypeChecker.getTypeAtLocation(exprIndentifier);
      const typeTextExprIndent = this.tsTypeChecker.typeToString(typeExprIndent);
      if (typeTextExprIndent === LIKE_FUNCTION_CONSTRUCTOR) {
        this.incrementCounters(expression, FaultID.ExplicitFunctionType);
      }
    } else {
      const autofix = this.autofixer?.fixNoTsLikeFunctionCall(callExpr);
      this.incrementCounters(expression, FaultID.ExplicitFunctionType, autofix);
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
    callLikeExpr: ts.CallExpression | ts.NewExpression | ts.ExpressionWithTypeArguments,
    callSignature?: ts.Signature
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
    if (callSignature) {
      this.checkTypeArgumentsForGenericCallWithNoTypeArgs(callLikeExpr, callSignature);
    }
  }

  private static isInvalidBuiltinGenericConstructorCall(
    newExpression: ts.CallExpression | ts.NewExpression | ts.ExpressionWithTypeArguments
  ): boolean {
    const isBuiltin = BUILTIN_GENERIC_CONSTRUCTORS.has(newExpression.expression.getText().replace(/Constructor$/, ''));
    return isBuiltin && (!newExpression.typeArguments || newExpression.typeArguments.length === 0);
  }

  private checkTypeArgumentsForGenericCallWithNoTypeArgs(
    callLikeExpr: ts.CallExpression | ts.NewExpression | ts.ExpressionWithTypeArguments,
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
    const shouldCheck = this.shouldCheckGenericCallExpression(callLikeExpr as ts.CallExpression);
    if (
      this.options.arkts2 &&
      (ts.isNewExpression(callLikeExpr) || ts.isCallExpression(callLikeExpr) && shouldCheck)
    ) {
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

  private shouldCheckGenericCallExpression(callExpr: ts.CallExpression): boolean {
    const signature = this.tsTypeChecker.getResolvedSignature(callExpr);
    if (!signature?.declaration) {
      return false;
    }
    const typeParamsSafeToInfer = this.areTypeParametersReturnTypeOnly(signature.declaration);
    if (!typeParamsSafeToInfer) {
      return false;
    }
    return TypeScriptLinter.isInStrictTypeContext(callExpr);
  }

  private areTypeParametersReturnTypeOnly(decl: ts.SignatureDeclaration | ts.JSDocSignature): boolean {
    if (!decl.typeParameters?.length) {
      return false;
    }

    const typeParamNames = new Set(
      decl.typeParameters.map((tp) => {
        return tp.name.getText();
      })
    );
    let affectsParams = false;

    decl.parameters.forEach((param) => {
      if (param.type && this.containsTypeParameters(param.type, typeParamNames)) {
        affectsParams = true;
      }
    });

    return !affectsParams;
  }

  private containsTypeParameters(node: ts.Node, typeParamNames: Set<string>): boolean {
    let found = false;
    ts.forEachChild(node, (child) => {
      if (ts.isIdentifier(child) && typeParamNames.has(child.text)) {
        found = true;
      }
      if (!found) {
        found = this.containsTypeParameters(child, typeParamNames);
      }
    });
    return found;
  }

  private static isInStrictTypeContext(callExpr: ts.CallExpression): boolean {
    const parent = callExpr.parent;

    if ((ts.isVariableDeclaration(parent) || ts.isPropertyDeclaration(parent)) && parent.type) {
      return true;
    }

    if (ts.isAsExpression(parent) || ts.isTypeAssertionExpression(parent)) {
      return true;
    }

    if (ts.isCallExpression(parent.parent) && parent.parent.typeArguments) {
      return true;
    }
    return false;
  }

  private checkForUnknownTypeInNonArkTS2(
    callLikeExpr: ts.CallExpression | ts.NewExpression | ts.ExpressionWithTypeArguments,
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
        const autofix = ts.isCallExpression(callLikeExpr) ?
          this.autofixer?.fixGenericCallNoTypeArgsForUnknown(callLikeExpr) :
          undefined;
        this.incrementCounters(callLikeExpr, FaultID.GenericCallNoTypeArgs, autofix);
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

  private handleFunctionReturnThisCall(node: ts.CallExpression | ts.NewExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    const args = node.arguments;
    const isUnsafeCallee = this.checkUnsafeFunctionCalleeName(node.expression);
    if (!isUnsafeCallee) {
      return;
    }
    if (!args) {
      return;
    }
    if (args.length === 0) {
      return;
    }
    const isForbidden = this.isForbiddenBodyArgument(args[0]);
    if (isForbidden) {
      this.incrementCounters(node, FaultID.NoFunctionReturnThis);
    }
  }

  private isForbiddenBodyArgument(arg: ts.Expression): boolean {
    if ((ts.isStringLiteral(arg) || ts.isNoSubstitutionTemplateLiteral(arg)) && arg.text === FORBIDDEN_FUNCTION_BODY) {
      return true;
    }

    if (ts.isIdentifier(arg)) {
      const symbol = this.tsTypeChecker.getSymbolAtLocation(arg);
      const decl = symbol?.valueDeclaration;

      if (
        decl &&
        ts.isVariableDeclaration(decl) &&
        decl.initializer &&
        ts.isStringLiteral(decl.initializer) &&
        decl.initializer.text === FORBIDDEN_FUNCTION_BODY
      ) {
        return true;
      }
    }

    return false;
  }

  private checkUnsafeFunctionCalleeName(expr: ts.Expression): boolean {
    if (ts.isIdentifier(expr) && expr.text === LIKE_FUNCTION) {
      return true;
    }

    if (ts.isParenthesizedExpression(expr)) {
      return this.checkUnsafeFunctionCalleeName(expr.expression);
    }

    if (ts.isPropertyAccessExpression(expr)) {
      if (expr.name.text === LIKE_FUNCTION) {
        return true;
      }
      return this.checkUnsafeFunctionCalleeName(expr.expression);
    }

    if (ts.isCallExpression(expr)) {
      return this.checkUnsafeFunctionCalleeName(expr.expression);
    }

    if (ts.isBinaryExpression(expr) && expr.operatorToken.kind === ts.SyntaxKind.CommaToken) {
      return this.checkUnsafeFunctionCalleeName(expr.right);
    }

    return false;
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
    this.checkOnClickCallback(tsCallOrNewExpr);
  }

private checkOnClickCallback(tsCallOrNewExpr: ts.CallExpression | ts.NewExpression): void {
    if (!tsCallOrNewExpr.arguments || tsCallOrNewExpr.arguments.length === 0 && this.options.arkts2) {
      return;
    }

    const isOnClick =
      ts.isPropertyAccessExpression(tsCallOrNewExpr.expression) && tsCallOrNewExpr.expression.name.text === 'onClick';
    if (!isOnClick) {
      return;
    }

    const objType = this.tsTypeChecker.getTypeAtLocation(tsCallOrNewExpr.expression.expression);
    const declNode = TsUtils.getDeclaration(objType.getSymbol());
    if (declNode) {
      const fileName = declNode.getSourceFile().fileName;
      if (!fileName.includes('@ohos/')) {
        return;
      }
    }

    const callback = tsCallOrNewExpr.arguments[0];
    if (!ts.isArrowFunction(callback)) {
      return;
    }

    this.checkAsyncOrPromiseFunction(callback);
  }

  private checkAsyncOrPromiseFunction(callback: ts.ArrowFunction): void {
    const returnsPromise = this.checkReturnsPromise(callback);
    const isAsync = callback.modifiers?.some((m) => { 
      return m.kind === ts.SyntaxKind.AsyncKeyword; 
    });

    if (isAsync || returnsPromise) {
      const startPos = callback.modifiers?.[0]?.getStart() ?? callback.getStart();
      const endPos = callback.body.getEnd();

      const errorNode = {
        getStart: () => { return startPos; },
        getEnd: () => { return endPos; },
        getSourceFile: () => { return callback.getSourceFile(); }
      } as ts.Node;

      this.incrementCounters(errorNode, FaultID.IncompationbleFunctionType);
    }
  }

  private checkReturnsPromise(callback: ts.ArrowFunction): boolean {
    const callbackType = this.tsTypeChecker.getTypeAtLocation(callback);
    const signatures = this.tsTypeChecker.getSignaturesOfType(callbackType, ts.SignatureKind.Call);
    if (signatures.length === 0) {
      return false;
    }

    const returnType = this.tsTypeChecker.getReturnTypeOfSignature(signatures[0]);
    return !!returnType.getProperty('then');
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

  private handleSdkApiThisArgs(
    callExpr: ts.CallExpression,
    calleeSym: ts.Symbol,
    name: string,
    parName: string | undefined,
    isSdkCommon?: boolean
  ): void {
    const builtinThisArgsInfos = isSdkCommon ?
      TypeScriptLinter.sdkCommonFuncMap.get(name + '_' + parName) :
      TypeScriptLinter.funcMap.get(name);
    if (!builtinThisArgsInfos) {
      return;
    }

    const sourceFile = calleeSym?.declarations?.[0]?.getSourceFile();
    const fileName = path.basename(sourceFile?.fileName + '');
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
    this.handleNodeForWrappedBuilder(tsNewExpr);
    this.checkForInterfaceInitialization(tsNewExpr);
    this.handleSharedArrayBuffer(tsNewExpr);
    this.handleSdkGlobalApi(tsNewExpr);
    this.checkCreatingPrimitiveTypes(tsNewExpr);
    this.handleNoDeprecatedApi(tsNewExpr);
    this.handleNodeForBuilderNode(tsNewExpr);

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
    this.handleFunctionReturnThisCall(tsNewExpr);
    this.checkArrayInitialization(tsNewExpr);
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
      if (ts.isFunctionTypeNode(resolveParam.type) && resolveParam.type.parameters.length === 0) {
        this.incrementCounters(resolveParam.type, FaultID.PromiseVoidNeedResolveArg);
      }
    }
    if (executor.body) {
      ts.forEachChild(executor.body, (node) => {
        if (
          ts.isCallExpression(node) &&
          ts.isIdentifier(node.expression) &&
          node.expression.text === 'resolve' &&
          node.arguments.length === 0
        ) {
          this.incrementCounters(node, FaultID.PromiseVoidNeedResolveArg);
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
    this.handleAsExprStructuralTyping(tsAsExpr, targetType, exprType);
    this.handleAsExpressionImport(tsAsExpr);
    this.handleNoTuplesArrays(node, targetType, exprType);
    this.handleObjectLiteralAssignmentToClass(tsAsExpr);
    this.handleArrayTypeImmutable(tsAsExpr, exprType, targetType);
    this.handleNotsLikeSmartTypeOnAsExpression(tsAsExpr);
    this.handleLimitedVoidTypeOnAsExpression(tsAsExpr);
  }

  private handleAsExprStructuralTyping(asExpr: ts.AsExpression, targetType: ts.Type, exprType: ts.Type): void {
    if (
      this.options.arkts2 &&
      this.tsUtils.needToDeduceStructuralIdentity(targetType, exprType, asExpr.expression, true) &&
      this.tsUtils.needToDeduceStructuralIdentity(exprType, targetType, asExpr.expression, true)
    ) {
      if (this.isExemptedAsExpression(asExpr)) {
        return;
      }
      if (!this.tsUtils.isObject(exprType)) {
        this.incrementCounters(asExpr, FaultID.StructuralIdentity);
      }
    }
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
    this.handleESObjectUsage(typeRef);
    this.handleBuiltinCtorCallSignature(typeRef);
    this.handleSharedArrayBuffer(typeRef);
    this.handleSdkGlobalApi(typeRef);
    this.handleSdkConstructorIface(typeRef);
    this.handleNodeForWrappedBuilder(typeRef);
    this.handleNoDeprecatedApi(typeRef);
    this.handleNodeForBuilderNode(typeRef);

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

    this.handleInvalidIdentifier(exportDecl);

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
   *
   * Also delegates the object-literal → union rule to `handleObjectLiteralUnionArg`.
   */
  private checkAssignmentMatching(
    contextNode: ts.Node,
    lhsType: ts.Type,
    rhsExpr: ts.Expression,
    isNewStructuralCheck: boolean = false
  ): void {
    const rhsType = this.tsTypeChecker.getTypeAtLocation(rhsExpr);

    // Object-literal to union rule (non-call contexts)
    this.handleObjectLiteralUnionArg(lhsType, rhsExpr);

    this.handleNoTuplesArrays(contextNode, lhsType, rhsType);
    this.handleArrayTypeImmutable(contextNode, lhsType, rhsType, rhsExpr);
    // check that 'sendable typeAlias' is assigned correctly
    if (this.tsUtils.isWrongSendableFunctionAssignment(lhsType, rhsType)) {
      this.incrementCounters(contextNode, FaultID.SendableFunctionAssignment);
    }
    const isStrict = this.tsUtils.needStrictMatchType(lhsType, rhsType);
    // 'isNewStructuralCheck' means that this assignment scenario was previously omitted, so only strict matches are checked now
    if (isNewStructuralCheck && !isStrict) {
      return;
    }
    this.handleStructuralTyping(contextNode, lhsType, rhsType, rhsExpr, isStrict);
    this.checkFunctionalTypeCompatibility(lhsType, rhsType, rhsExpr);
  }

  /**
   * Flags `{ ... }` used where the LHS type is a union with
   * more than one non-nullish member and the object literal
   * is not already asserted (e.g., `{...} as A`).
   * Applies to variable initializers, assignments, call expressions and returns
   * that route through `checkAssignmentMatching`.
   */
  private handleObjectLiteralUnionArg(lhsType: ts.Type, rhsExpr: ts.Expression): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!ts.isObjectLiteralExpression(rhsExpr) || !lhsType?.isUnion()) {
      return;
    }

    // Already asserted/cast? Allowed.
    const parent = rhsExpr.parent;
    if (ts.isAsExpression(parent) || ts.isTypeAssertionExpression(parent)) {
      return;
    }

    // Allow nullish unions like 'T | null | undefined'
    const nonNullishMembers = lhsType.types.filter((t) => {
      return !TsUtils.isNullishType(t);
    });
    if (nonNullishMembers.length <= 1) {
      return;
    }

    // Skip any types that are from standard lib
    const nonStdlibMembers = nonNullishMembers.filter((t) => {
      return !(t.getSymbol() && isStdLibrarySymbol(t.getSymbol()));
    });

    const hasClassOrInterfaceMember = nonStdlibMembers.some((t) => {
      const sym = t.aliasSymbol ?? t.getSymbol();
      if (!sym) {
        return false;
      }
      const decls = sym.getDeclarations() ?? [];

      return decls.some((d) => {
        return ts.isClassDeclaration(d) || ts.isInterfaceDeclaration(d);
      });
    });
    if (!hasClassOrInterfaceMember) {
      return;
    }

    this.incrementCounters(rhsExpr, FaultID.ObjectLiteralUnionNeedsCast);
  }

  private handleStructuralTyping(
    contextNode: ts.Node,
    lhsType: ts.Type,
    rhsType: ts.Type,
    rhsExpr: ts.Expression,
    isStrict: boolean
  ): void {
    if (TypeScriptLinter.isValidPromiseReturnedFromAsyncFunction(lhsType, rhsType, rhsExpr)) {
      return;
    }
    if (this.tsUtils.needToDeduceStructuralIdentity(lhsType, rhsType, rhsExpr, isStrict)) {
      this.incrementCounters(contextNode, FaultID.StructuralIdentity);
    }
  }

  private static isValidPromiseReturnedFromAsyncFunction(
    lhsType: ts.Type,
    rhsType: ts.Type,
    rhsExpr: ts.Expression
  ): boolean {

    /*
     * When resolving the contextual type for return expression in async function, the TS compiler
     * infers 'PromiseLike<T>' type instead of standard 'Promise<T>' (see following link:
     * https://github.com/microsoft/TypeScript/pull/27270). In this special case, we treat
     * these two types as equal and only need to validate the type argument.
     */

    if (!ts.isReturnStatement(rhsExpr.parent)) {
      return false;
    }
    const enclosingFunction = ts.findAncestor(rhsExpr, ts.isFunctionLike);
    if (!TsUtils.hasModifier(enclosingFunction?.modifiers, ts.SyntaxKind.AsyncKeyword)) {
      return false;
    }

    const lhsPromiseLikeType = lhsType.isUnion() && lhsType.types.find(TsUtils.isStdPromiseLikeType);
    if (!lhsPromiseLikeType || !TsUtils.isStdPromiseType(rhsType)) {
      return false;
    }

    const lhsTypeArg = TsUtils.isTypeReference(lhsPromiseLikeType) && lhsPromiseLikeType.typeArguments?.[0];
    const rhsTypeArg = TsUtils.isTypeReference(rhsType) && rhsType.typeArguments?.[0];
    return lhsTypeArg !== undefined && lhsTypeArg === rhsTypeArg;
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
    switch (decorator.parent.kind) {
      case ts.SyntaxKind.PropertyDeclaration:
      case ts.SyntaxKind.ClassDeclaration:
      case ts.SyntaxKind.MethodDeclaration:
      case ts.SyntaxKind.Parameter:
        this.handleBuiltinDisableDecorator(decorator);
        break;
      default:
        break;
    }
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
      this.incrementCounters(typeNode, FaultID.LimitedVoidType);
    }
  }

  private handleLimitedVoidTypeOnAsExpression(node: ts.AsExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    const targetType = this.tsTypeChecker.getTypeAtLocation(node.type);
    if (TsUtils.isVoidType(targetType)) {
      this.incrementCounters(node.type, FaultID.LimitedVoidType);
    }
  }

  private handleLimitedVoidWithCall(node: ts.CallExpression): void {
    if (
      ts.isExpressionStatement(node.parent) ||
      ts.isVoidExpression(node.parent) ||
      ts.isArrowFunction(node.parent) ||
      ts.isConditionalExpression(node.parent) && ts.isExpressionStatement(node.parent.parent)
    ) {
      return;
    }
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
    const { isLiteralInitialized, hasExplicitTypeAnnotation } = this.getDeclarationInfo(expr);

    const isUnionType = (nodeType.flags & ts.TypeFlags.Union) !== 0;

    const isTypeAllowed = (t: ts.Type): boolean => {
      const typeText = this.tsTypeChecker.typeToString(t);
      return Boolean(
        t.flags & ts.TypeFlags.StringLike ||
          typeText === 'String' ||
          typeText === 'number' ||
          t.flags & ts.TypeFlags.NumberLike && (/^\d+$/).test(typeText) ||
          isLiteralInitialized && !hasExplicitTypeAnnotation ||
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
    hasExplicitTypeAnnotation: boolean;
  } {
    const symbol = this.tsTypeChecker.getSymbolAtLocation(expression);
    const declaration = symbol?.valueDeclaration;

    if (!declaration || !ts.isVariableDeclaration(declaration)) {
      return { isLiteralInitialized: false, hasExplicitTypeAnnotation: false };
    }

    const hasExplicitTypeAnnotation = !!declaration.type;
    const initializerInfo = TypeScriptLinter.getInitializerInfo(declaration.initializer);

    return {
      isLiteralInitialized: initializerInfo.isLiteralInitialized,
      hasExplicitTypeAnnotation
    };
  }

  private static getInitializerInfo(initializer?: ts.Expression): {
    isLiteralInitialized: boolean;
  } {
    if (!initializer) {
      return { isLiteralInitialized: false };
    }

    const isLiteralInitialized = ts.isNumericLiteral(initializer) || ts.isStringLiteral(initializer);

    return { isLiteralInitialized };
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

  isExprReturnedFromAsyncFunction(rhsExpr: ts.Expression | undefined, lhsType: ts.Type): ts.Type | undefined {
    void this;
    if (!rhsExpr) {
      return undefined;
    }

    const enclosingFunction = ts.findAncestor(rhsExpr, ts.isFunctionLike);
    const isReturnExpr = ts.isReturnStatement(rhsExpr.parent) || ts.isArrowFunction(rhsExpr.parent);
    if (!enclosingFunction) {
      return undefined;
    }
    if (!isReturnExpr) {
      return undefined;
    }

    if (!TsUtils.hasModifier(enclosingFunction.modifiers, ts.SyntaxKind.AsyncKeyword)) {
      return undefined;
    }

    const lhsPromiseLikeType = lhsType.isUnion() && lhsType.types.find(TsUtils.isStdPromiseLikeType);
    if (!lhsPromiseLikeType) {
      return undefined;
    }

    if (!TsUtils.isTypeReference(lhsPromiseLikeType) || !lhsPromiseLikeType.typeArguments?.length) {
      return undefined;
    }
    return lhsPromiseLikeType.typeArguments[0];
  }

  private handleArrayTypeImmutable(node: ts.Node, lhsType: ts.Type, rhsType: ts.Type, rhsExpr?: ts.Expression): void {
    if (!this.options.arkts2) {
      return;
    }

    const possibleLhsType = this.isExprReturnedFromAsyncFunction(rhsExpr, lhsType);
    if (possibleLhsType) {
      lhsType = possibleLhsType;
    }

    const isArray = this.tsUtils.isArray(lhsType) && this.tsUtils.isArray(rhsType);
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

    const possibleLhsTypeStr = this.checkLhsTypeString(node, rhsTypeStr);
    if (possibleLhsTypeStr) {
      lhsTypeStr = possibleLhsTypeStr;
    }

    if (lhsTypeStr !== rhsTypeStr) {
      this.incrementCounters(node, FaultID.ArrayTypeImmutable);
    }
  }

  private checkLhsTypeString(node: ts.Node, rhsTypeStr: string): string | undefined {
    void this;
    if (!ts.isAsExpression(node) || !ts.isArrayLiteralExpression(node.expression)) {
      return undefined;
    }
    let lhsTypeStr: string | undefined;
    node.expression.elements.forEach((elem) => {
      if (elem.kind === ts.SyntaxKind.FalseKeyword || elem.kind === ts.SyntaxKind.TrueKeyword) {
        lhsTypeStr = rhsTypeStr.replace(elem.getText(), 'boolean');
      }
    });

    return lhsTypeStr;
  }

  private isSubtypeByBaseTypesList(baseType: ts.Type, actualType: ts.Type): boolean {
    if (this.isTypeAssignable(actualType, baseType)) {
      return true;
    }
    const actualBaseTypes = actualType.getBaseTypes() || [];
    return actualBaseTypes.some((base) => {
      return this.isSubtypeByBaseTypesList(baseType, base);
    });
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

    const decl = this.tsUtils.getDeclarationNode(currentParam);
    if (!decl || !ts.isPropertyDeclaration(decl)) {
      return;
    }

    const autofix = this.autofixer?.fixCustomBidirectionalBinding(originalExpr, decl.type, currentParam, customParam);
    this.incrementCounters(firstExpr, FaultID.DoubleExclaBindingNotSupported, autofix);
  }

  private handleDoubleDollar(node: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    if (
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
      if (node.expression.expression.text === CustomInterfaceName.Extend) {
        const autofix = this.autofixer?.fixExtendDecorator(node, false, this.interfacesNeedToImport);
        this.incrementCounters(node.parent, FaultID.ExtendDecoratorNotSupported, autofix);
      } else if (node.expression.expression.text === CustomInterfaceName.AnimatableExtend) {
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
    this.handleNoDeprecatedApi(node as ts.TaggedTemplateExpression);
    this.incrementCounters(node, FaultID.TaggedTemplates);
  }

  private checkFunctionalTypeCompatibility(lhsType: ts.Type, rhsType: ts.Type, rhsExpr: ts.Expression): void {
    if (this.options.arkts2 && !this.tsUtils.areCompatibleFunctionalTypes(lhsType, rhsType)) {
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
      | ts.ExportDeclaration
  ): void {
    if (!this.options.arkts2) {
      return;
    }
    const checkIdentifier = (identifier: ts.Identifier | undefined): void => {
      const text = identifier && ts.isIdentifier(identifier) ? identifier.text : '';
      if (identifier && text && INVALID_IDENTIFIER_KEYWORDS.includes(text) && !this.checkImportSymbol(identifier)) {
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
    } else if (ts.isExportDeclaration(decl)) {
      if (decl.exportClause && ts.isNamedExports(decl.exportClause)) {
        for (const exportSpecifier of decl.exportClause.elements) {
          checkIdentifier(exportSpecifier.name);
        }
      }
    } else if (isStructDeclaration(decl)) {
      checkIdentifier((decl as ts.StructDeclaration).name);
    } else {
      checkIdentifier(decl.name as ts.Identifier);
    }
  }

  private checkImportSymbol(identifier: ts.Identifier): boolean {
    let symbol = this.tsUtils.trueSymbolAtLocation(identifier);
    if (symbol && 'unknown' === symbol.name) {
      symbol = this.tsTypeChecker.getSymbolAtLocation(identifier);
    }
    let res = false;
    const cb = (): void => {
      res = true;
    };
    if (symbol) {
      this.checkSymbolAndExecute(symbol, [identifier.text], SYSTEM_MODULES, cb);
    }
    return res;
  }

  private handleHeritageClause(node: ts.HeritageClause): void {
    this.checkEWTArgumentsForSdkDuplicateDeclName(node);
    if (!this.options.arkts2 || !this.useStatic) {
      return;
    }
    if (node.token === ts.SyntaxKind.ExtendsKeyword || node.token === ts.SyntaxKind.ImplementsKeyword) {
      node.types.forEach((type) => {
        const expr = type.expression;
        this.handleGenericCallWithNoTypeArgs(type);
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
      this.handleInterfaceFieldImplementation(node);
      this.handleMissingSuperCallInExtendedClass(node);
      this.handleFieldTypesMatchingBetweenDerivedAndBaseClass(node);
      this.checkReadonlyOverridesFromBase(node);
      this.handleNoDeprecatedApi(node);
    }
  }

  private checkReadonlyOverridesFromBase(node: ts.HeritageClause): void {
    if (!this.options.arkts2) {
      return;
    }
    if (node.token !== ts.SyntaxKind.ExtendsKeyword) {
      return;
    }
    const childClass = node.parent;
    const baseTypeNode = node.types[0];
    if (!ts.isClassDeclaration(childClass) || !baseTypeNode) {
      return;
    }
    const baseType = this.tsTypeChecker.getTypeAtLocation(baseTypeNode);
    if (!baseType) {
      return;
    }
    const baseProps = baseType.getProperties();
    this.validateReadonlyOverrides(childClass, baseProps);
  }

  private validateReadonlyOverrides(childClass: ts.ClassDeclaration, baseProps: ts.Symbol[]): void {
    for (const member of childClass.members) {
      if (!ts.isPropertyDeclaration(member) || !member.name) {
        continue;
      }
      const isDerivedReadonly = TsUtils.hasModifier(member.modifiers, ts.SyntaxKind.ReadonlyKeyword);
      if (!isDerivedReadonly) {
        continue;
      }
      const memberName = ts.isIdentifier(member.name) ? member.name.text : undefined;
      if (!memberName) {
        continue;
      }
      const baseProp = baseProps.find((p) => {
        return p.name === memberName;
      });
      if (!baseProp) {
        continue;
      }

      const baseDecl = baseProp.valueDeclaration;
      if (!baseDecl || !ts.isPropertyDeclaration(baseDecl)) {
        continue;
      }
      const isBaseReadonly = TsUtils.hasModifier(baseDecl.modifiers, ts.SyntaxKind.ReadonlyKeyword);
      if (!isBaseReadonly) {
        this.incrementCounters(member, FaultID.NoClassSuperPropReadonly);
      }
    }
  }

  /**
   * Ensures classes fully implement all properties from their interfaces.
   */
  private handleInterfaceFieldImplementation(clause: ts.HeritageClause): void {
    // Only process implements clauses
    if (clause.token !== ts.SyntaxKind.ImplementsKeyword) {
      return;
    }
    const classDecl = clause.parent as ts.ClassDeclaration;
    if (!ts.isClassDeclaration(classDecl) || !classDecl.name) {
      return;
    }

    for (const interfaceType of clause.types) {
      const expr = interfaceType.expression;
      if (!ts.isIdentifier(expr)) {
        continue;
      }
      const sym = this.tsUtils.trueSymbolAtLocation(expr);
      const interfaceDecl = sym?.declarations?.find(ts.isInterfaceDeclaration);
      if (!interfaceDecl) {
        continue;
      }
      // Gather all inherited interfaces
      const allInterfaces = this.getAllInheritedInterfaces(interfaceDecl);
      // If the class fails to implement any member, report once and exit
      if (!this.classImplementsAllMembers(classDecl, allInterfaces)) {
        this.incrementCounters(classDecl.name, FaultID.InterfaceFieldNotImplemented);
        return;
      }
    }
  }

  /**
   * Recursively collects an interface and all its ancestor interfaces.
   */
  private getAllInheritedInterfaces(root: ts.InterfaceDeclaration): ts.InterfaceDeclaration[] {
    const collected: ts.InterfaceDeclaration[] = [];
    const stack: ts.InterfaceDeclaration[] = [root];
    while (stack.length) {
      const current = stack.pop()!;
      collected.push(current);
      if (!current.heritageClauses) {
        continue;
      }
      for (const clause of current.heritageClauses) {
        if (clause.token !== ts.SyntaxKind.ExtendsKeyword) {
          continue;
        }
        for (const typeNode of clause.types) {
          const expr = typeNode.expression;
          if (!ts.isIdentifier(expr)) {
            continue;
          }
          const sym = this.tsUtils.trueSymbolAtLocation(expr);
          const decl = sym?.declarations?.find(ts.isInterfaceDeclaration);
          if (decl) {
            stack.push(decl);
          }
        }
      }
    }
    return collected;
  }

  /**
   * Returns true if the class declaration declares every property or method
   * signature from the provided list of interface declarations.
   */
  private classImplementsAllMembers(classDecl: ts.ClassDeclaration, interfaces: ts.InterfaceDeclaration[]): boolean {
    void this;

    for (const intf of interfaces) {
      for (const member of intf.members) {
        if ((ts.isPropertySignature(member) || ts.isMethodSignature(member)) && ts.isIdentifier(member.name)) {
          const name = member.name.text;
          const found = classDecl.members.some((m) => {
            return (
              (ts.isPropertyDeclaration(m) || ts.isMethodDeclaration(m)) &&
              ts.isIdentifier(m.name) &&
              m.name.text === name
            );
          });
          if (!found) {
            return false;
          }
        }
      }
    }
    return true;
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

  private checkCollectionsForPropAccess(node: ts.Node, ident: ts.Node): void {
    if (!ts.isIdentifier(ident)) {
      return;
    }
    if (this.isBitVector(ident)) {
      return;
    }
    const autofix = this.autofixer?.replaceNode(node, ident.getText());

    this.incrementCounters(node, FaultID.NoNeedStdLibSendableContainer, autofix);
  }

  private checkCollectionsSymbol(node: ts.Node): void {
    if (!this.options.arkts2) {
      return;
    }

    const cb = (): void => {
      const parent = node.parent;
      if (!parent) {
        return;
      }
      const shouldSkipFix = TypeScriptLinter.shouldSkipFixForCollectionsArray(node);
      if (shouldSkipFix) {
        if (ts.isPropertyAccessExpression(parent)) {
          this.incrementCounters(node, FaultID.NoNeedStdLibSendableContainer);
        }
        if (ts.isQualifiedName(parent)) {
          this.incrementCounters(node, FaultID.NoNeedStdLibSendableContainer);
        }
      } else {
        if (ts.isPropertyAccessExpression(parent)) {
          this.checkCollectionsForPropAccess(parent, parent.name);
          return;
        }
        if (ts.isQualifiedName(parent)) {
          this.checkCollectionsForPropAccess(parent, parent.right);
          return;
        }
      }
      if (ts.isImportSpecifier(parent) && ts.isIdentifier(node)) {
        const bitVectorUsed = this.checkBitVector(node.getSourceFile());

        if (bitVectorUsed?.used) {
          const ns = bitVectorUsed.ns;
          if (parent.name.text === ns) {
            return;
          }
        }

        if (parent.propertyName && node.text === parent.propertyName.text) {
          return;
        }

        const autofix = this.autofixer?.removeImport(node, parent);
        this.incrementCounters(node, FaultID.NoNeedStdLibSendableContainer, autofix);
      }
    };

    this.checkNodeForUsage(node, COLLECTIONS_TEXT, COLLECTIONS_MODULES, cb);
  }

  private static shouldSkipFixForCollectionsArray(node: ts.Node): boolean {
    const isArrayWithNumericArg = (n: ts.Node | undefined): boolean => {
      return !!(
        n &&
        ts.isNewExpression(n) &&
        ts.isPropertyAccessExpression(n.expression) &&
        n.expression.name.text === 'Array' &&
        n.arguments?.some((arg) => {
          return ts.isNumericLiteral(arg);
        })
      );
    };
    if (isArrayWithNumericArg(node.parent)) {
      return true;
    }

    let currentNode: ts.Node | undefined = node;
    while (currentNode) {
      if (ts.isVariableDeclaration(currentNode)) {
        if (isArrayWithNumericArg(currentNode.initializer)) {
          return true;
        }
        break;
      }
      currentNode = currentNode.parent;
    }

    return false;
  }

  private checkWorkerSymbol(symbol: ts.Symbol, node: ts.Node): void {
    const cb = (): void => {
      this.incrementCounters(node, FaultID.NoNeedStdlibWorker);
    };

    this.checkSymbolAndExecute(symbol, [WORKER_TEXT], WORKER_MODULES, cb);
  }

  private checkConcurrencySymbol(symbol: ts.Symbol, node: ts.Node): void {
    const cb = (): void => {
      const parent = node.parent;

      if (!ts.isPropertyAccessExpression(parent)) {
        return;
      }

      if (parent.name.text === ARKTSUTILS_LOCKS_MEMBER) {
        const autofix = this.autofixer?.fixConcurrencyLock(parent);
        this.incrementCounters(node, FaultID.LimitedStdLibNoImportConcurrency, autofix);
      }

      if (PROCESS_DEPRECATED_INTERFACES.includes(parent.name.text)) {
        this.incrementCounters(node, FaultID.DeprecatedProcessApi);
      }
    };

    this.checkSymbolAndExecute(symbol, [ARKTSUTILS_LOCKS_MEMBER, ARKTSUTILS_PROCESS_MEMBER], ARKTSUTILS_MODULES, cb);
  }

  private checkSymbolAndExecute(symbol: ts.Symbol, symbolNames: string[], modules: string[], cb: () => void): void {
    void this;

    // Only execute if the provided list contains the symbol’s actual name
    if (!symbolNames.includes(symbol.name)) {
      return;
    }

    const decl = TsUtils.getDeclaration(symbol);
    if (!decl) {
      cb();
      return;
    }

    const fileName = TypeScriptLinter.getFileName(decl);
    if (
      modules.some((moduleName) => {
        return fileName.startsWith(moduleName);
      })
    ) {
      cb();
    }
  }

  private checkNodeForUsage(node: ts.Node, symbolName: string, modules: string[], cb: () => void): void {
    const symbol = this.tsUtils.trueSymbolAtLocation(node);
    if (symbol) {
      this.checkSymbolAndExecute(symbol, [symbolName], modules, cb);

      return;
    }

    if (node.getText() === symbolName) {
      cb();
    }
  }

  private checkBitVector(node: ts.Node): BitVectorUsage {
    if (!ts.isIdentifier(node)) {
      let isBitVector: BitVectorUsage;
      node.forEachChild((child) => {
        const checked = this.checkBitVector(child);
        if (checked?.used) {
          isBitVector = checked;
        }
      });

      return isBitVector;
    }

    if (!this.isBitVector(node)) {
      return { ns: '', used: false };
    }

    if (!ts.isPropertyAccessExpression(node.parent)) {
      return undefined;
    }

    return { ns: node.parent.expression.getText(), used: true };
  }

  private isBitVector(ident: ts.Identifier): boolean {
    void this;

    return ident.text === BIT_VECTOR;
  }

  interfacesNeedToAlarm: ts.Identifier[] = [];
  interfacesNeedToImport: Set<string> = new Set<string>();
  interfacesAlreadyImported: Set<string> = new Set<string>();

  private handleInterfaceImport(identifier: ts.Identifier): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!this.isInterfaceImportNeeded(identifier)) {
      return;
    }

    const name = identifier.getText();
    if (!this.interfacesNeedToImport.has(name)) {
      this.interfacesNeedToImport.add(name);
    }

    this.interfacesNeedToAlarm.push(identifier);
  }

  private isInterfaceImportNeeded(identifier: ts.Identifier): boolean {
    const name = identifier.getText();
    return (
      arkuiImportList.has(name) &&
      !skipImportDecoratorName.has(name) &&
      !this.interfacesAlreadyImported.has(name) &&
      !this.isParentAlreadyImported(identifier.parent) &&
      !this.isDeclarationInSameFile(identifier) &&
      !this.isDeprecatedInterface(identifier) &&
      !TypeScriptLinter.isWrappedByExtendDecorator(identifier)
    );
  }

  private isDeprecatedInterface(node: ts.Identifier): boolean {
    const symbol = this.tsUtils.trueSymbolAtLocation(node);
    const decl = TsUtils.getDeclaration(symbol);
    if (!decl) {
      return false;
    }

    const parName = this.tsUtils.getParentSymbolName(symbol);
    const parameters = ts.isFunctionLike(decl) ? decl.parameters : undefined;
    const returnType = ts.isFunctionLike(decl) ? decl.type?.getText() : undefined;
    const fileName = path.basename(decl.getSourceFile().fileName) + '';

    const deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
      parName === undefined ? DEPRECATE_UNNAMED : parName,
      parameters,
      returnType,
      fileName
    );
    return this.getFaultIdWithMatchedDeprecatedApi(node.getText(), deprecatedApiCheckMap).length > 0;
  }

  private static isWrappedByExtendDecorator(node: ts.Identifier): boolean {
    const wrappedSkipComponents = new Set<string>([CustomInterfaceName.AnimatableExtend, CustomInterfaceName.Extend]);
    if (ts.isCallExpression(node.parent)) {
      const expr = node.parent.expression;
      if (wrappedSkipComponents.has(expr.getText()) && node.getText() !== CustomInterfaceName.AnimatableExtend) {
        return true;
      }
    }
    return false;
  }

  private isParentAlreadyImported(node: ts.Node): boolean {
    let identifier: ts.Identifier | undefined;

    while (
      ts.isPropertyAccessExpression(node) ||
      ts.isParenthesizedExpression(node) ||
      ts.isCallExpression(node) ||
      ts.isQualifiedName(node)
    ) {
      const nextNode = ts.isQualifiedName(node) ? node.left : node.expression;
      if (!nextNode) {
        break;
      }

      if (ts.isIdentifier(nextNode)) {
        identifier = nextNode;
        break;
      }

      node = nextNode;
    }

    return identifier !== undefined && this.isDeclarationInSameFile(identifier);
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

    if (!ts.isIdentifier(node.expression) || node.expression.text !== CustomInterfaceName.Styles) {
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
    const autofix = this.autofixer?.removeNode(stringLiteral.parent);

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
      this.interfacesNeedToImport.add(CustomInterfaceName.Observed);
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
        return decorator.getText() === '@' + CustomInterfaceName.Observed;
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

  private handleLocalDeclarationOfClassAndIface(node: ts.ClassDeclaration | ts.InterfaceDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }
    this.isLocalClass(node);
  }

  private isLocalClass(node: ts.Node): void {
    if (
      ts.findAncestor(node, (node) => {
        return (
          ts.isArrowFunction(node) ||
          ts.isFunctionDeclaration(node) ||
          ts.isMethodDeclaration(node) ||
          ts.isFunctionExpression(node) ||
          ts.isConstructorDeclaration(node) ||
          ts.isGetAccessor(node) ||
          ts.isSetAccessor(node) ||
          ts.isBlock(node)
        );
      })
    ) {
      this.incrementCounters(node, FaultID.NoLocalClass);
    }
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
    if (TypeScriptLinter.isInterfaceImplementation(errorNode)) {
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

  static isInterfaceImplementation(node: ts.Node): boolean {
    const classDeclaration = ts.findAncestor(node, ts.isClassDeclaration);
    if (!classDeclaration) {
      return false;
    }

    if (classDeclaration.heritageClauses) {
      return classDeclaration.heritageClauses.some((clause) => {
        return clause.token === ts.SyntaxKind.ImplementsKeyword;
      });
    }
    return false;
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
      if (
        ts.isVariableDeclaration(declaration) ||
        ts.isTypeAliasDeclaration(declaration) ||
        ts.isClassDeclaration(declaration) ||
        ts.isInterfaceDeclaration(declaration) ||
        ts.isFunctionDeclaration(declaration) ||
        ts.isEnumDeclaration(declaration)
      ) {
        isLocal = true;
        break;
      }
    }

    if (isLocal) {
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

  private checkSdkAbilityLifecycleMonitor(callExpr: ts.CallExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    // Guard: must be a property-access .on
    if (!this.isOnMethod(callExpr)) {
      return;
    }

    // Guard: left side must be applicationContext
    if (!this.isApplicationContext(callExpr)) {
      return;
    }

    // Guard: exactly two arguments
    const args = callExpr.arguments;
    if (args.length !== 2) {
      return;
    }

    // Guard: first arg must be string literal "abilityLifecycle"
    const eventArg = args[0];
    if (!ts.isStringLiteral(eventArg) || eventArg.text !== 'abilityLifecycle') {
      return;
    }

    // Guard: second arg must be a variable declared as AbilityLifecycleCallback
    const cbArg = args[1];
    if (!ts.isIdentifier(cbArg)) {
      return;
    }
    const varSym = this.tsUtils.trueSymbolAtLocation(cbArg);
    const decl = varSym?.declarations?.find(ts.isVariableDeclaration);
    if (
      !decl?.type ||
      !ts.isTypeReferenceNode(decl.type) ||
      decl.type.typeName.getText() !== 'AbilityLifecycleCallback'
    ) {
      return;
    }

    // Report the legacy callback usage
    this.incrementCounters(callExpr, FaultID.SdkAbilityLifecycleMonitor);
  }

  private isOnMethod(node: ts.CallExpression): boolean {
    void this;
    const expr = node.expression;
    return ts.isPropertyAccessExpression(expr) && expr.name.text === 'on';
  }

  private isApplicationContext(node: ts.CallExpression): boolean {
    const expr = node.expression as ts.PropertyAccessExpression;
    if (!ts.isIdentifier(expr.expression)) {
      return false;
    }
    const type = this.tsTypeChecker.getTypeAtLocation(expr.expression);
    const symbol = type.getSymbol();
    return symbol ? this.checkApplicationContextSymbol(symbol) : false;
  }

  private checkApplicationContextSymbol(symbol: ts.Symbol): boolean {
    void this;
    if (symbol.getName() === 'default') {
      const declarations = symbol.getDeclarations() || [];
      for (const decl of declarations) {
        if (
          ts.isClassDeclaration(decl) &&
          decl.name?.getText() === ABILITY_LIFECYCLE_SDK &&
          decl.getSourceFile().fileName.endsWith(`${ABILITY_LIFECYCLE_SDK}${EXTNAME_D_TS}`)
        ) {
          return true;
        }
      }
      return false;
    }
    const symbolName = symbol.getName();
    const hasValidName = symbolName === ABILITY_LIFECYCLE_SDK;
    if (hasValidName) {
      const declarations = symbol.getDeclarations() || [];
      return declarations.some((decl) => {
        return decl.getSourceFile().fileName.endsWith(`${ABILITY_LIFECYCLE_SDK}${EXTNAME_D_TS}`);
      });
    }
    return false;
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

    const autofix = this.autofixer?.applyForOfJsArrayFix(node);
    this.incrementCounters(node, FaultID.InteropJsObjectTraverseJsInstance, autofix);
  }

  private checkStdLibConcurrencyImport(importDeclaration: ts.ImportDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const importClause = importDeclaration.importClause;
    if (!importClause) {
      return;
    }

    const moduleName = path.basename((importDeclaration.moduleSpecifier as ts.StringLiteral).text);
    const expectedImports = MODULE_IMPORTS[moduleName];
    if (!expectedImports) {
      return;
    }

    const defaultImport = importClause.name;
    const namedBindings = importClause.namedBindings;

    const namedImports = namedBindings && ts.isNamedImports(namedBindings) ? namedBindings.elements : [];

    const FORBIDDEN_DEFAULT_IMPORT_MODULES = Object.keys(MODULE_IMPORTS).filter((name) => {
      return name !== '@kit.ArkTS';
    });

    const defaultIsForbidden = defaultImport && FORBIDDEN_DEFAULT_IMPORT_MODULES.includes(moduleName);
    const forbiddenNamed = namedImports.filter((spec) => {
      const name = spec.propertyName ? spec.propertyName.getText() : spec.name.getText();
      return expectedImports.includes(name);
    });

    if (defaultIsForbidden) {
      if (defaultImport?.getText() === 'process') {
        this.incrementCounters(defaultImport, FaultID.LimitedStdLibNoImportConcurrency);
      } else {
        const autofix = this.autofixer?.removeDefaultImport(importDeclaration, defaultImport, expectedImports[0]);
        this.incrementCounters(defaultImport, FaultID.LimitedStdLibNoImportConcurrency, autofix);
      }
    }

    this.processImportSpecifier(forbiddenNamed, importDeclaration);
  }

  private processImportSpecifier(forbiddenNamed: ts.ImportSpecifier[], importDeclaration: ts.ImportDeclaration): void {
    for (const spec of forbiddenNamed) {
      const bitVectorUsed = this.checkBitVector(spec.getSourceFile());

      if (bitVectorUsed?.used) {
        const ns = bitVectorUsed.ns;
        if (spec.name.text === ns) {
          continue;
        }
      }
      const autofix = this.autofixer?.removeImportSpecifier(spec, importDeclaration);
      this.incrementCounters(spec, FaultID.LimitedStdLibNoImportConcurrency, autofix);
    }
  }

  /**
   * Checks that each field in a subclass matches the type of the same-named field
   * in its base class or implemented interfaces.
   */
  private handleFieldTypesMatchingBetweenDerivedAndBaseClass(node: ts.HeritageClause): void {
    if (node.token !== ts.SyntaxKind.ExtendsKeyword && node.token !== ts.SyntaxKind.ImplementsKeyword) {
      return;
    }

    const derivedClass = node.parent;
    if (!ts.isClassDeclaration(derivedClass)) {
      return;
    }

    for (const member of derivedClass.members) {
      if (!ts.isPropertyDeclaration(member) || !ts.isIdentifier(member.name) || !member.type) {
        continue;
      }
      const propName = member.name.text;

      // Delegate heritage comparison logic
      if (this.hasFieldTypeMismatchWithBases(node, propName, member)) {
        this.incrementCounters(member.name, FaultID.FieldTypeMismatch);
      }
    }
  }

  /**
   * Checks the given derived property against all base classes/interfaces
   * in the heritage clause. Returns true if a mismatch is found.
   */
  private hasFieldTypeMismatchWithBases(
    node: ts.HeritageClause,
    propName: string,
    member: ts.PropertyDeclaration
  ): boolean {
    for (const hType of node.types) {
      const baseExpr = hType?.expression;
      if (!ts.isIdentifier(baseExpr)) {
        continue;
      }

      const baseSym = this.tsUtils.trueSymbolAtLocation(baseExpr);
      const baseDecl = baseSym?.declarations?.find(TsUtils.isClassOrInterfaceDeclaration);
      if (!baseDecl) {
        continue;
      }

      const baseProp = this.findPropertyDeclarationInBaseChain(baseDecl, propName);
      if (!baseProp?.type) {
        continue;
      }

      const derivedType = this.tsTypeChecker.getTypeAtLocation(member.type!);
      const baseType = this.tsTypeChecker.getTypeAtLocation(baseProp.type);

      if (!this.isFieldTypeMatchingBetweenDerivedAndBase(derivedType, baseType)) {
        return true;
      }
    }
    return false;
  }

  /**
   * Searches the base chain (classes or interfaces) to find the first declaration
   * of the given property (with a type annotation). Avoids cycles.
   */
  private findPropertyDeclarationInBaseChain(
    decl: ts.ClassDeclaration | ts.InterfaceDeclaration,
    propName: string,
    visited: Set<ts.Node> = new Set()
  ): ts.PropertyDeclaration | ts.PropertySignature | undefined {
    if (visited.has(decl)) {
      return undefined;
    }
    visited.add(decl);

    if (ts.isClassDeclaration(decl)) {
      return this.findPropertyInClassChain(decl, propName, visited);
    }

    // Interface path
    return this.findPropertyInInterfaceChain(decl, propName, visited);
  }

  /** Look for a property in a class declaration or its base classes */
  private findPropertyInClassChain(
    decl: ts.ClassDeclaration,
    propName: string,
    visited: Set<ts.Node>
  ): ts.PropertyDeclaration | ts.PropertySignature | undefined {
    // Check current class members
    const member = decl.members.find((m): m is ts.PropertyDeclaration => {
      return ts.isPropertyDeclaration(m) && ts.isIdentifier(m.name) && m.name.text === propName && !!m.type;
    });
    if (member) {
      return member;
    }

    // Otherwise, follow the extends clause (single inheritance)
    const ext = decl.heritageClauses?.find((c) => {
      return c.token === ts.SyntaxKind.ExtendsKeyword;
    });
    if (!ext || ext.types.length === 0) {
      return undefined;
    }

    const expr = ext.types[0].expression;
    if (!ts.isIdentifier(expr)) {
      return undefined;
    }

    const sym = this.tsUtils.trueSymbolAtLocation(expr);
    const nextDecl = sym?.declarations?.find(ts.isClassDeclaration);
    return nextDecl ? this.findPropertyInClassChain(nextDecl, propName, visited) : undefined;
  }

  /** Look for a property in an interface declaration or its extended interfaces */
  private findPropertyInInterfaceChain(
    decl: ts.InterfaceDeclaration,
    propName: string,
    visited: Set<ts.Node>
  ): ts.PropertySignature | ts.PropertyDeclaration | undefined {
    // Check current interface members
    const member = decl.members.find((m): m is ts.PropertySignature => {
      return ts.isPropertySignature(m) && ts.isIdentifier(m.name) && m.name.text === propName && !!m.type;
    });
    if (member) {
      return member;
    }

    // Otherwise, follow extended interfaces
    const ext = decl.heritageClauses?.find((c) => {
      return c.token === ts.SyntaxKind.ExtendsKeyword;
    });
    if (!ext) {
      return undefined;
    }

    for (const t of ext.types) {
      const expr = t.expression;
      if (!ts.isIdentifier(expr)) {
        continue;
      }
      const sym = this.tsUtils.trueSymbolAtLocation(expr);
      const nextDecl = sym?.declarations?.find(ts.isInterfaceDeclaration);
      if (nextDecl) {
        const found = this.findPropertyInInterfaceChain(nextDecl, propName, visited);
        if (found) {
          return found;
        }
      }
    }
    return undefined;
  }

  /**
   * Returns true if the union type members of subclass field's type
   * exactly match those of the base field's type (order-insensitive).
   * So `number|string` ↔ `string|number` passes, but `number` ↔ `number|string` fails.
   */
  private isFieldTypeMatchingBetweenDerivedAndBase(derivedType: ts.Type, baseType: ts.Type): boolean {
    // Split union type strings into trimmed member names
    const derivedNames = this.tsTypeChecker.
      typeToString(derivedType).
      split('|').
      map((s) => {
        return s.trim();
      });
    const baseNames = this.tsTypeChecker.
      typeToString(baseType).
      split('|').
      map((s) => {
        return s.trim();
      });

    // Only match if both unions contain exactly the same members
    if (derivedNames.length !== baseNames.length) {
      return false;
    }
    return (
      derivedNames.every((name) => {
        return baseNames.includes(name);
      }) &&
      baseNames.every((name) => {
        return derivedNames.includes(name);
      })
    );
  }

  /**
   * If a class method overrides a base-class abstract method that had no explicit return type,
   * then any explicit return type other than `void` is an error.
   * Also flags async overrides with no explicit annotation.
   */
  private checkAbstractOverrideReturnType(method: ts.MethodDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const baseClass = this.getDirectBaseClassOfGivenMethodDecl(method);
    if (!baseClass) {
      return;
    }

    // Locate the abstract method in the inheritance chain
    const methodName = method.name.getText();
    const baseMethod = this.findAbstractMethodInBaseChain(baseClass, methodName);
    if (!baseMethod) {
      return;
    }

    // Only if base had no explicit return type
    if (baseMethod.type) {
      return;
    }

    // If override declares a return type, and it isn't void → error
    if (method.type && method.type.kind !== ts.SyntaxKind.VoidKeyword) {
      const target = ts.isIdentifier(method.name) ? method.name : method;
      this.incrementCounters(target, FaultID.InvalidAbstractOverrideReturnType);

      // Also catch async overrides with no explicit annotation (defaulting to Promise<void>)
    } else if (TsUtils.hasModifier(method.modifiers, ts.SyntaxKind.AsyncKeyword)) {
      const target = ts.isIdentifier(method.name) ? method.name : method;
      this.incrementCounters(target, FaultID.InvalidAbstractOverrideReturnType);
    }
  }

  /**
   * Finds the direct superclass declaration for the given method's containing class.
   * Returns undefined if the class has no extends clause or cannot resolve the base class.
   */
  private getDirectBaseClassOfGivenMethodDecl(method: ts.MethodDeclaration): ts.ClassDeclaration | undefined {
    // Must live in a class with an extends clause
    const classDecl = method.parent;
    if (!ts.isClassDeclaration(classDecl) || !classDecl.heritageClauses) {
      return undefined;
    }

    return this.getBaseClassDeclFromHeritageClause(classDecl.heritageClauses);
  }

  /**
   * Walks up the inheritance chain starting from `startClass` to find an abstract method
   * named `methodName`. Returns the MethodDeclaration if found, otherwise `undefined`.
   */
  private findAbstractMethodInBaseChain(
    startClass: ts.ClassDeclaration,
    methodName: string
  ): ts.MethodDeclaration | undefined {
    // Prevent infinite loops from circular extends
    const visited = new Set<ts.ClassDeclaration>();
    let current: ts.ClassDeclaration | undefined = startClass;
    while (current && !visited.has(current)) {
      visited.add(current);
      const found = current.members.find((m) => {
        return (
          ts.isMethodDeclaration(m) &&
          ts.isIdentifier(m.name) &&
          m.name.text === methodName &&
          TsUtils.hasModifier(m.modifiers, ts.SyntaxKind.AbstractKeyword)
        );
      }) as ts.MethodDeclaration | undefined;
      if (found) {
        return found;
      }
      current = this.getBaseClassDeclFromHeritageClause(current.heritageClauses);
    }
    return undefined;
  }

  getBaseClassDeclFromHeritageClause(clauses?: ts.NodeArray<ts.HeritageClause>): ts.ClassDeclaration | undefined {
    if (!clauses) {
      return undefined;
    }

    const ext = clauses.find((h) => {
      return h.token === ts.SyntaxKind.ExtendsKeyword;
    });
    if (!ext || ext.types.length === 0) {
      return undefined;
    }

    // Resolve the base-class declaration
    const expr = ext.types[0].expression;
    if (!ts.isIdentifier(expr)) {
      return undefined;
    }

    const sym = this.tsUtils.trueSymbolAtLocation(expr);
    return sym?.declarations?.find(ts.isClassDeclaration);
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
    if (!this.options.arkts2) {
      return;
    }

    // We are only interested in 'extends' clauses
    if (node.token !== ts.SyntaxKind.ExtendsKeyword) {
      return;
    }

    if (!ts.isClassDeclaration(node.parent)) {
      return;
    }

    /*
     * Get the parent class declaration (what the child class extends)
     * This could be a stdlib error type
     */
    const identInfo = this.getExtendedIdentifiersInfo(node);
    if (identInfo.type === ExtendedIdentifierType.UNKNOWN) {
      // if it's unknown return
      return;
    }

    if (identInfo.type === ExtendedIdentifierType.ERROR) {
      this.handleErrorClassExtend(node.parent);
      // handled error case return
      return;
    }

    if (identInfo.type === ExtendedIdentifierType.CLASS) {
      // If it's class, get the constructor's parameters and match against it.
      const extendedClassInfo = this.extractExtendedClassConstructorInfo(identInfo.decl);
      if (!extendedClassInfo) {
        return;
      }

      // If there are only non parametric constructions, do not check.
      const value = extendedClassInfo.values().next().value;
      if (extendedClassInfo.size === 1 && value && value.length === 0) {
        return;
      }

      this.handleExtendCustomClass(node.parent, extendedClassInfo, identInfo.decl.name?.text + '');
    }
  }

  private handleExtendCustomClass(
    classDecl: ts.ClassDeclaration,
    extendedClassInfo: Set<ConstructorParameter[]>,
    extendedClassName: string
  ): void {
    const superCall = TypeScriptLinter.checkIfSuperCallExists(classDecl);
    if (!superCall) {
      this.incrementCounters(classDecl, FaultID.MissingSuperCall);
      return;
    }
    outer: for (const ctorParams of extendedClassInfo) {
      const matches: boolean[] = [];
      if (superCall.arguments.length > ctorParams.length) {
        continue;
      }

      for (const [idx, param] of ctorParams.entries()) {
        const argument = superCall.arguments[idx];
        if (!this.checkParameter(param, argument, matches, idx)) {
          continue outer;
        }
      }

      if (
        matches.some((val) => {
          return !val;
        })
      ) {
        continue;
      }
      this.handleExtendCustomClassForSdkApiDeprecated(extendedClassName, superCall, SDK_COMMON_TYPE);
      this.handleExtendCustomClassForSdkApiDeprecated(extendedClassName, superCall, BUILTIN_TYPE);
      return;
    }

    this.incrementCounters(classDecl, FaultID.MissingSuperCall);
  }

  private checkParameter(
    param: ConstructorParameter,
    argument: ts.Expression | undefined,
    matches: boolean[],
    idx: number
  ): boolean {
    if (!param.isOptional && !argument) {
      matches[idx] = false;
      return false;
    }

    if (!argument && param.isOptional) {
      matches[idx] = true;
      return true;
    }

    if (argument !== undefined) {
      if (this.isEnumArgument(argument)) {
        matches[idx] = true;
        return true;
      }
      matches[idx] = this.checkIfArgumentAndParamMatches(param, argument);
      return matches[idx];
    }
    return true;
  }

  private isEnumArgument(argument: ts.Expression): boolean {
    if (!ts.isPropertyAccessExpression(argument)) {
      return false;
    }

    const leftSide = argument.expression;
    const symbol = this.tsTypeChecker?.getSymbolAtLocation(leftSide);

    return (
      symbol?.declarations?.some((decl) => {
        return (
          ts.isEnumDeclaration(decl) ||
          ts.isVariableDeclaration(decl) && decl.initializer && ts.isEnumDeclaration(decl.initializer)
        );
      }) ?? false
    );
  }

  private handleExtendCustomClassForSdkApiDeprecated(
    extendedClassName: string,
    superCall: ts.CallExpression,
    apiType: string
  ): void {
    const problemStr = TypeScriptLinter.getFaultIdSdkApiInfoWithConstructorDecl(extendedClassName, apiType);
    if (problemStr) {
      const faultID = sdkCommonAllDeprecatedTypeName.has(extendedClassName) ?
        FaultID.SdkCommonApiDeprecated :
        TypeScriptLinter.getFinalSdkFaultIdByProblem(problemStr, apiType);
      if (!faultID) {
        return;
      }
      this.incrementCounters(
        superCall,
        faultID,
        undefined,
        apiType === SDK_COMMON_TYPE ?
          TypeScriptLinter.getErrorMsgForSdkCommonApi(extendedClassName, faultID) :
          undefined
      );
    }
  }

  private static getErrorMsgForSdkCommonApi(name: string, faultID: number): string {
    let errorMsg = cookBookMsg[faultID];
    if (faultID === FaultID.SdkCommonApiDeprecated || faultID === FaultID.SdkCommonApiWhiteList) {
      errorMsg = `The "${name}" in SDK is no longer supported.(sdk-method-not-supported)`;
    } else if (faultID === FaultID.SdkCommonApiBehaviorChange) {
      errorMsg = `The "${name}" in SDK has been changed.(sdk-method-changed)`;
    } else if (faultID === FaultID.NoDeprecatedApi) {
      errorMsg = `The ArkUI interface "${name}" is deprecated (arkui-deprecated-interface)`;
    }
    return errorMsg;
  }

  private checkIfArgumentAndParamMatches(param: ConstructorParameter, argument: ts.Expression): boolean {
    const typeNode = this.tsTypeChecker.getTypeAtLocation(argument);
    const typeString = this.tsTypeChecker.typeToString(typeNode);

    if (param.type.includes(STRINGLITERAL_STRING) && argument.kind === ts.SyntaxKind.StringLiteral) {
      return true;
    }
    if (param.type.includes(NUMBER_LITERAL) && argument.kind === ts.SyntaxKind.NumericLiteral) {
      return true;
    }

    if (
      param.type.includes('boolean') &&
      (argument.kind === ts.SyntaxKind.FalseKeyword || argument.kind === ts.SyntaxKind.TrueKeyword)
    ) {
      return true;
    }

    if (param.type === typeString) {
      return true;
    }

    return false;
  }

  private handleErrorClassExtend(classDecl: ts.ClassDeclaration): void {
    // if it's Error, the super method should be called with no arguments or a single string argument
    const superCall = TypeScriptLinter.checkIfSuperCallExists(classDecl);
    if (!superCall) {
      this.incrementCounters(classDecl, FaultID.MissingSuperCall);
      return;
    }

    if (superCall.arguments.length > 1) {

      /*
       * STD Error Type have two constructors
       * either empty constructor which is just "Error" message
       * or the message you provide, so if it's more than one argument provided,
       * this should be raised as an issue
       */
      this.incrementCounters(classDecl, FaultID.MissingSuperCall);
      return;
    }

    if (superCall.arguments.length === 1) {
      const argument = superCall.arguments[0];
      const typeNode = this.tsTypeChecker.getTypeAtLocation(argument);
      const typeString = this.tsTypeChecker.typeToString(typeNode);

      if (typeString === 'string' || ts.isStringLiteral(argument) || ts.isNumericLiteral(argument)) {
        return;
      }
      this.incrementCounters(classDecl, FaultID.MissingSuperCall);
    }
  }

  private static checkIfSuperCallExists(classDecl: ts.ClassDeclaration): ts.CallExpression | undefined {
    // check if current class has constructor
    const constructor = TypeScriptLinter.getConstructorOfClass(classDecl);
    if (!constructor) {
      return undefined;
    }
    const superCallExpr = TypeScriptLinter.getSuperCallExpr(constructor);
    if (!superCallExpr) {
      return undefined;
    }

    return superCallExpr;
  }

  /**
   * Extracts the type of the Identifier node from an extends heritage clause.
   */
  private getExtendedIdentifiersInfo(node: ts.HeritageClause): ExtendedIdentifierInfo {
    const extendedIdentifier = node.types[0]?.expression;
    if (!extendedIdentifier) {
      return { type: ExtendedIdentifierType.UNKNOWN };
    }

    const symbol = this.tsUtils.trueSymbolAtLocation(extendedIdentifier);
    if (!symbol) {
      return { type: ExtendedIdentifierType.UNKNOWN };
    }

    if (symbol.getName().includes(STRING_ERROR_LITERAL)) {
      const declaration = this.tsUtils.getDeclarationNode(extendedIdentifier);
      if (!declaration) {
        return { type: ExtendedIdentifierType.ERROR };
      }

      if (declaration.getSourceFile().fileName !== this.sourceFile.fileName) {
        return { type: ExtendedIdentifierType.ERROR };
      }
    }

    const classDecl = symbol?.declarations?.find(ts.isClassDeclaration);
    if (!classDecl) {
      return { type: ExtendedIdentifierType.UNKNOWN };
    }

    return { type: ExtendedIdentifierType.CLASS, decl: classDecl };
  }

  private extractExtendedClassConstructorInfo(extendedClass: ts.ClassDeclaration): BaseClassConstructorInfo {
    const constructors = extendedClass.members.filter(ts.isConstructorDeclaration);
    if (constructors.length === 0) {
      return undefined;
    }

    const allConstructorInformation: BaseClassConstructorInfo = new Set();
    for (const ctor of constructors) {
      const allParams: ConstructorParameter[] = [];
      const parameters = ctor.parameters;
      for (const param of parameters) {
        const ident = param.name;
        const name = ident.getText();
        const type = this.tsTypeChecker.getTypeAtLocation(ident);
        const typeString = this.tsTypeChecker.typeToString(type);
        const isOptional = !!param.questionToken;
        const info = { name, type: typeString, isOptional };

        allParams.push(info);
      }
      allConstructorInformation.add(allParams);
    }

    return allConstructorInformation;
  }

  private static getConstructorOfClass(classDecl: ts.ClassDeclaration): ts.ConstructorDeclaration | undefined {
    if (classDecl.members.length === 0) {
      return undefined;
    }

    for (const member of classDecl.members) {
      if (!ts.isConstructorDeclaration(member)) {
        continue;
      }
      return member;
    }
    return undefined;
  }

  private static getSuperCallExpr(constructor: ts.ConstructorDeclaration): ts.CallExpression | undefined {
    if (!constructor.body) {
      return undefined;
    }

    for (const stmt of constructor.body.statements) {
      if (!ts.isExpressionStatement(stmt)) {
        continue;
      }
      const callExpr = stmt.expression;
      if (!ts.isCallExpression(callExpr)) {
        continue;
      }
      if (callExpr.expression.kind !== ts.SyntaxKind.SuperKeyword) {
        continue;
      }

      return callExpr;
    }
    return undefined;
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
      this.tryAutoFixInterOpImportJs(importDecl, symbolToUse);
    }
  }

  private tryAutoFixInterOpImportJs(importDecl: ts.ImportDeclaration, symbolToUse: ts.Symbol): void {
    const declaration = symbolToUse.declarations?.[0];
    if (declaration) {
      const sourceFile = declaration.getSourceFile();
      if (sourceFile.fileName.endsWith(EXTNAME_JS)) {
        this.incrementCounters(importDecl, FaultID.InterOpImportJs);
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
        if (!this.tsUtils.isArkts12File(declaration.getSourceFile()) && !isStdLibrarySymbol(symbol)) {
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
    if (decoratorName === CustomInterfaceName.LocalBuilder) {
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
    const indexType = this.tsTypeChecker.getTypeAtLocation(indexExpr);
    const typeString = this.tsTypeChecker.typeToString(indexType);
    if (typeString === 'number' || typeString === 'string') {
      return true;
    }
    const baseExprSym = this.tsUtils.trueSymbolAtLocation(node.expression);
    if (indexType.isUnion()) {
      return indexType.types.some((t) => {
        return this.isInvalidEnumMemberType(t, baseExprSym);
      });
    }
    return this.isInvalidEnumMemberType(indexType, baseExprSym);
  }

  private isInvalidEnumMemberType(indexType: ts.Type, baseExprSym: ts.Symbol | undefined): boolean {
    const indexSym = indexType.getSymbol();
    if (!indexSym) {
      return false;
    }
    return !indexSym.declarations?.some((decl) => {
      if (decl && ts.isEnumDeclaration(decl.parent) && ts.isEnumMember(decl)) {
        const enumDeclSym = this.tsUtils.trueSymbolAtLocation(decl.parent.name);
        return enumDeclSym === baseExprSym;
      }
      return false;
    });
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
    if (moduleSpecifier.text !== ARKUI_MODULE && moduleSpecifier.text !== STATE_MANAGEMENT_MODULE) {
      return;
    }

    this.incrementCounters(node, FaultID.MakeObservedIsNotSupported);
  }

  private handlePropertyDeclarationForProp(node: ts.PropertyDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const decorator = ts.getDecorators(node)?.[0];
    if (!decorator) {
      return;
    }

    let identifier: ts.Identifier | undefined;
    if (ts.isIdentifier(decorator.expression)) {
      identifier = decorator.expression;
    } else if (ts.isCallExpression(decorator.expression) && ts.isIdentifier(decorator.expression.expression)) {
      identifier = decorator.expression.expression;
    }

    if (!identifier) {
      return;
    }

    const decoratorName = identifier.getText();
    const autofix = this.autofixer?.fixPropDecorator(identifier, decoratorName);
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

    if (ts.isIdentifier(tsCallExpr.expression)) {
      const funcName = tsCallExpr.expression.text;
      if (funcName === 'setTimeout') {
        return;
      }
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
      let expectedUnionType: string[] = [];
      if (expectedType.includes('|')) {
        expectedUnionType = expectedType.split('|').map((item) => {
          return item.trim();
        });
      }
      this.checkParameterTypeCompatibility(arg, expectedUnionType, expectedType);
    });
  }

  private checkParameterTypeCompatibility(arg: ts.Expression, expectedUnionType: string[], expectedType: string): void {
    const actualSym = this.tsTypeChecker.getSymbolAtLocation(arg);
    const decl = TsUtils.getDeclaration(actualSym);
    if (decl && ts.isParameter(decl) && decl.type) {
      const actualType = this.tsTypeChecker.getTypeFromTypeNode(decl.type);
      const actualTypeName = this.tsTypeChecker.typeToString(actualType);
      if (expectedUnionType.length > 0) {
        if (!expectedUnionType.includes(actualTypeName)) {
          this.incrementCounters(arg, FaultID.NoTsLikeSmartType);
        }
        return;
      }
      if (actualTypeName !== expectedType) {
        this.incrementCounters(arg, FaultID.NoTsLikeSmartType);
      }
    }
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
    const right: ts.Expression = ts.isAsExpression(node.right) ? node.right.expression : node.right;
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

  private handleUnsafeOptionalCallComparison(expr: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    const declaration = this.tsUtils.getDeclarationNode(expr.expression);
    if (!declaration) {
      return;
    }

    if (
      (ts.isParameter(declaration) || ts.isPropertyDeclaration(declaration)) &&
      !!declaration.questionToken &&
      !ts.isPropertyAccessChain(expr)
    ) {
      this.incrementCounters(expr, FaultID.NoTsLikeSmartType);
    }
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
    const stopCondition = (node: ts.Node): boolean => {
      return (
        ts.isFunctionDeclaration(node) ||
        ts.isFunctionExpression(node) ||
        ts.isMethodDeclaration(node) ||
        ts.isAccessor(node) ||
        ts.isArrowFunction(node)
      );
    };
    const callback = (node: ts.Node): void => {
      if (!ts.isReturnStatement(node) || !node.expression) {
        return;
      }
      const getPropertyAccess = (expr: ts.Expression): ts.PropertyAccessExpression | undefined => {
        if (ts.isPropertyAccessExpression(expr)) {
          return expr;
        }
        if (ts.isCallExpression(expr) && ts.isPropertyAccessExpression(expr.expression)) {
          return expr.expression;
        }

        return undefined;
      };

      const isStaticPropertyAccess = (expr: ts.PropertyAccessExpression, className: string): boolean => {
        return ts.isIdentifier(expr.expression) && expr.expression.text === className;
      };

      const isInstancePropertyAccess = (node: ts.Expression): boolean => {
        return ts.isPropertyAccessExpression(node) && node.expression.kind === ts.SyntaxKind.ThisKeyword;
      };

      const propExp = getPropertyAccess(node.expression);
      if (className && propExp && isStaticPropertyAccess(propExp, className)) {
        this.checkPropertyAccess(node, propExp, staticProps, methodReturnType);
      }

      if (isInstancePropertyAccess(node.expression)) {
        this.checkPropertyAccess(node, node.expression as ts.PropertyAccessExpression, instanceProps, methodReturnType);
      }
    };
    forEachNodeInSubtree(body, callback, stopCondition);
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

    this.tsUtils.collectPropertiesFromClass(classDecl, result);
    return result;
  }

  private isExactlySameType(type1: ts.Type, type2: ts.Type): boolean {
    if (type2.getCallSignatures().length > 0) {
      const returnType = TsUtils.getFunctionReturnType(type2);
      return returnType ? this.isExactlySameType(type1, returnType) : false;
    }

    const type1String = this.tsTypeChecker.typeToString(type1);
    const type2String = this.tsTypeChecker.typeToString(type2);
    if (type1String === type2String) {
      return true;
    }

    if (this.checkBaseTypes(type1, type2) || this.checkBaseTypes(type2, type1)) {
      return true;
    }
    return type1String === type2String;
  }

  private checkBaseTypes(type1: ts.Type, type2: ts.Type): boolean {
    const isClassType =
      (type1.getFlags() & ts.TypeFlags.Object) !== 0 &&
      ((type1 as ts.ObjectType).objectFlags & ts.ObjectFlags.Class) !== 0;
    if (isClassType) {
      const baseTypes = (type1 as any).getBaseTypes?.() || [];
      for (const baseType of baseTypes) {
        if (this.isExactlySameType(baseType, type2)) {
          return true;
        }
      }
    }
    return false;
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
        if (decoratorName && decoratorName === CustomInterfaceName.CustomLayout) {
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
    this.handleLargeNumericLiteral(node);
  }

  private handleLargeNumericLiteral(node: ts.NumericLiteral): void {
    const parent = node.parent;
    const isPrefix = ts.isPrefixUnaryExpression(parent) && parent.operator === ts.SyntaxKind.MinusToken;

    if (TsUtils.isLargeNumericLiteral(node, isPrefix)) {
      this.incrementCounters(node, FaultID.LargeNumericLiteral);
      return;
    }

    // Check for int overflow (existing logic)
    const type = isPrefix ? this.tsTypeChecker.getContextualType(parent) : this.tsTypeChecker.getContextualType(node);
    const isLarge = TsUtils.ifLargerThanInt(node, isPrefix);
    if (!isLarge) {
      return;
    }
    const isLong = this.tsUtils.isStdLongType(type);
    if (isLong) {
      return;
    }
    this.incrementCounters(node, FaultID.LongNumeric);
  }

  private checkArrayUsageWithoutBound(accessExpr: ts.ElementAccessExpression): void {
    if (this.shouldSkipArrayBoundCheck(accessExpr)) {
      return;
    }

    this.performArrayBoundValidation(accessExpr);
  }

  private shouldSkipArrayBoundCheck(accessExpr: ts.ElementAccessExpression): boolean {
    if (!this.options.arkts2 || !this.useStatic) {
      return true;
    }
    if (this.isDirectLengthCheckWithNumber(accessExpr)) {
      return true;
    }
    if (
      this.isInMaxLengthControlledLoop(accessExpr) ||
      TypeScriptLinter.hasDefaultValueProtection(accessExpr) ||
      this.isInLengthCheckedBlock(accessExpr) ||
      this.isInStandardLengthControlledLoop(accessExpr)
    ) {
      return true;
    }

    if (ts.isConditionalExpression(accessExpr.parent)) {
      const conditional = accessExpr.parent;
      if (this.isLengthCheckCondition(conditional.condition, accessExpr)) {
        return true;
      }
    }

    if (ts.isNumericLiteral(accessExpr.argumentExpression)) {
      return false;
    }

    if (this.isObjectPropertyAccess(accessExpr)) {
      return true;
    }

    return this.isInstanceOfCheck(accessExpr.parent, accessExpr);
  }

  private performArrayBoundValidation(accessExpr: ts.ElementAccessExpression): void {
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
    if (this.isInLoopWithArrayLengthModification(accessExpr, arrayIdent.text)) {
      this.incrementCounters(accessExpr, FaultID.RuntimeArrayCheck);
      return;
    }
    const arrayDecl = TypeScriptLinter.findArrayDeclaration(arraySym);
    if (arrayDecl && TypeScriptLinter.isArrayCreatedWithOtherArrayLength(arrayDecl)) {
      return;
    }
    const indexExpr = accessExpr.argumentExpression;
    const loopVarName = ts.isIdentifier(indexExpr) ? indexExpr.text : undefined;
    if (ts.isPrefixUnaryExpression(indexExpr) && indexExpr.operator === ts.SyntaxKind.PlusPlusToken) {
      this.incrementCounters(arrayIdent.parent, FaultID.RuntimeArrayCheck);
      return;
    }
    const { isInSafeContext, isValidBoundCheck, isVarModifiedBeforeAccess } = this.analyzeSafeContext(
      accessExpr,
      loopVarName,
      arraySym
    );
    if (TypeScriptLinter.isIncrementOrDecrement(indexExpr)) {
      return;
    }
    if (isInSafeContext) {
      if (!isValidBoundCheck || isVarModifiedBeforeAccess) {
        this.incrementCounters(arrayIdent.parent, FaultID.RuntimeArrayCheck);
      }
      return;
    }
    this.incrementCounters(arrayIdent.parent, FaultID.RuntimeArrayCheck);
  }

  static isIncrementOrDecrement(expr: ts.Expression): boolean {
    if (ts.isPostfixUnaryExpression(expr)) {
      return expr.operator === ts.SyntaxKind.PlusPlusToken || expr.operator === ts.SyntaxKind.MinusMinusToken;
    }

    if (ts.isPrefixUnaryExpression(expr)) {
      return expr.operator === ts.SyntaxKind.PlusPlusToken || expr.operator === ts.SyntaxKind.MinusMinusToken;
    }

    return false;
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
    if (!ts.isBinaryExpression(condition)) {
      return false;
    }
    return this.checkBinaryExpressionBound(condition, varName, arraySym);
  }

  private checkBinaryExpressionBound(expr: ts.BinaryExpression, varName: string, arraySym: ts.Symbol): boolean {
    if (this.checkDirectBoundChecks(expr, varName, arraySym)) {
      return true;
    }

    if (TypeScriptLinter.checkNumericBoundChecks(expr, varName)) {
      return true;
    }

    return (
      this.checkBoundCondition(expr.left, varName, arraySym) || this.checkBoundCondition(expr.right, varName, arraySym)
    );
  }

  private checkDirectBoundChecks(expr: ts.BinaryExpression, varName: string, arraySym: ts.Symbol): boolean {
    const { left, right, operatorToken } = expr;

    if (this.checkVarLessThanArrayLength(left, right, operatorToken, varName, arraySym)) {
      return true;
    }

    return this.checkArrayLengthGreaterThanVar(left, right, operatorToken, varName, arraySym);
  }

  static checkNumericBoundChecks(expr: ts.BinaryExpression, varName: string): boolean {
    const { left, right, operatorToken } = expr;

    if (ts.isIdentifier(left) && left.text === varName && ts.isNumericLiteral(right)) {
      const value = parseFloat(right.text);
      return (
        operatorToken.kind === ts.SyntaxKind.GreaterThanEqualsToken && value <= 0 ||
        operatorToken.kind === ts.SyntaxKind.GreaterThanToken && value < 0
      );
    }

    if (ts.isPropertyAccessExpression(left) && left.name.text === LENGTH_IDENTIFIER && ts.isNumericLiteral(right)) {
      const constantValue = parseInt(right.text);
      return (
        operatorToken.kind === ts.SyntaxKind.LessThanToken && constantValue > 0 ||
        operatorToken.kind === ts.SyntaxKind.LessThanEqualsToken && constantValue >= 0
      );
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
      left.name.text === LENGTH_IDENTIFIER &&
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
      right.name.text === LENGTH_IDENTIFIER &&
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

  private isConcatArray(accessedNode: ts.Node): boolean {
    if (!ts.isIdentifier(accessedNode)) {
      return false;
    }
    const decl = this.tsUtils.getDeclarationNode(accessedNode);
    if (!decl) {
      return false;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(decl);
    return TsUtils.isConcatArrayType(type);
  }

  private getArrayAccessInfo(expr: ts.ElementAccessExpression): false | ArrayAccess {
    let accessedExpression: ts.Node = expr.expression;
    if (ts.isPropertyAccessExpression(accessedExpression)) {
      accessedExpression = accessedExpression.name;
    }
    if (!ts.isIdentifier(accessedExpression)) {
      return false;
    }

    const baseType = this.tsTypeChecker.getTypeAtLocation(accessedExpression);
    if (!this.tsUtils.isArray(baseType) && !this.isConcatArray(accessedExpression)) {
      return false;
    }

    const accessArgument = expr.argumentExpression;
    if (this.isInstanceOfCheck(expr.parent, expr)) {
      return false;
    }

    if (TypeScriptLinter.isFunctionCall(accessArgument)) {
      return false;
    }

    if (this.checkNumericType(accessArgument) || this.isEnumMember(accessArgument)) {
      return {
        pos: expr.getEnd(),
        accessingIdentifier: accessArgument,
        arrayIdent: accessedExpression
      };
    }

    return false;
  }

  private checkNumericType(node: ts.Node): boolean {
    const argType = this.tsTypeChecker.getTypeAtLocation(node);
    return (
      (argType.flags & ts.TypeFlags.NumberLike) !== 0 ||
      argType.isUnionOrIntersection() &&
        argType.types.some((t) => {
          return t.flags & ts.TypeFlags.NumberLike;
        })
    );
  }

  private isEnumMember(node: ts.Node): boolean {
    if (ts.isPropertyAccessExpression(node)) {
      const symbol = this.tsUtils.trueSymbolAtLocation(node);
      return !!symbol && (symbol.flags & ts.SymbolFlags.EnumMember) !== 0;
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
        return ts.isPropertyAccessExpression(arg) && arg.name.text === LENGTH_IDENTIFIER;
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

  private isInstanceOfCheck(node: ts.Node, accessExpr: ts.ElementAccessExpression): boolean {
    if (!ts.isBinaryExpression(node)) {
      return false;
    }

    if (node.operatorToken.kind === ts.SyntaxKind.AmpersandAmpersandToken) {
      return this.isInstanceOfCheck(node.right, accessExpr);
    }

    return (
      ts.isBinaryExpression(node) &&
      node.operatorToken.kind === ts.SyntaxKind.InstanceOfKeyword &&
      node.left === accessExpr
    );
  }

  private isLengthCheckCondition(condition: ts.Expression, accessExpr: ts.ElementAccessExpression): boolean {
    if (!ts.isBinaryExpression(condition)) {
      return false;
    }

    const arrayAccessInfo = this.getArrayAccessInfo(accessExpr);
    if (!arrayAccessInfo) {
      return false;
    }

    const { arrayIdent } = arrayAccessInfo;
    const arraySym = this.tsUtils.trueSymbolAtLocation(arrayIdent);
    if (!arraySym) {
      return false;
    }

    if (
      ts.isPropertyAccessExpression(condition.left) &&
      condition.left.name.text === LENGTH_IDENTIFIER &&
      this.tsUtils.trueSymbolAtLocation(condition.left.expression) === arraySym
    ) {
      return true;
    }

    return false;
  }

  private isInLengthCheckedBlock(accessExpr: ts.ElementAccessExpression): boolean {
    let parent: ts.Node | undefined = accessExpr.parent;

    while (parent) {
      if (ts.isBlock(parent) && ts.isIfStatement(parent.parent)) {
        const ifStatement = parent.parent;
        const arrayAccessInfo = this.getArrayAccessInfo(accessExpr);

        if (arrayAccessInfo && this.isArrayLengthCheck(ifStatement.expression, arrayAccessInfo.arrayIdent)) {
          return true;
        }
      }

      parent = parent.parent;
    }

    return false;
  }

  private isArrayLengthCheck(condition: ts.Expression, arrayIdent: ts.Identifier): boolean {
    if (TypeScriptLinter.isAndExpression(condition)) {
      const binaryExpr = condition as ts.BinaryExpression;
      return (
        this.isArrayLengthCheck(binaryExpr.left, arrayIdent) || this.isArrayLengthCheck(binaryExpr.right, arrayIdent)
      );
    }

    if (ts.isBinaryExpression(condition)) {
      return TypeScriptLinter.checkBinaryLengthConditions(condition, arrayIdent);
    }

    return false;
  }

  static isAndExpression(condition: ts.Expression): boolean {
    return ts.isBinaryExpression(condition) && condition.operatorToken.kind === ts.SyntaxKind.AmpersandAmpersandToken;
  }

  static checkBinaryLengthConditions(condition: ts.BinaryExpression, arrayIdent: ts.Identifier): boolean {
    if (TypeScriptLinter.checkSimpleArrayCheck(condition, arrayIdent)) {
      return true;
    }

    return TypeScriptLinter.checkArrayLengthComparisons(condition, arrayIdent);
  }

  static checkSimpleArrayCheck(condition: ts.BinaryExpression, arrayIdent: ts.Identifier): boolean {
    return (
      ts.isIdentifier(condition.left) &&
      condition.left.text === arrayIdent.text &&
      condition.operatorToken.kind === ts.SyntaxKind.AmpersandAmpersandToken
    );
  }

  static checkArrayLengthComparisons(condition: ts.BinaryExpression, arrayIdent: ts.Identifier): boolean {
    const { left, right, operatorToken } = condition;

    if (
      !ts.isPropertyAccessExpression(left) ||
      left.expression.getText() !== arrayIdent.text ||
      left.name.text !== LENGTH_IDENTIFIER ||
      !ts.isNumericLiteral(right)
    ) {
      return false;
    }

    if (right.text === '0') {
      return (
        operatorToken.kind === ts.SyntaxKind.GreaterThanToken ||
        operatorToken.kind === ts.SyntaxKind.GreaterThanEqualsToken
      );
    }

    const constantValue = parseInt(right.text);
    return (
      operatorToken.kind === ts.SyntaxKind.LessThanToken && constantValue > 0 ||
      operatorToken.kind === ts.SyntaxKind.LessThanEqualsToken && constantValue >= 0
    );
  }

  private isInMaxLengthControlledLoop(accessExpr: ts.ElementAccessExpression): boolean {
    const arrayAccessInfo = this.getArrayAccessInfo(accessExpr);
    if (!arrayAccessInfo) {
      return false;
    }

    const accessedArrayName = arrayAccessInfo.arrayIdent.text;
    return TypeScriptLinter.checkParentLoopForMaxLength(accessExpr, accessedArrayName);
  }

  static checkParentLoopForMaxLength(node: ts.Node, arrayName: string): boolean {
    let current: ts.Node | undefined = node;

    while (current) {
      if (TypeScriptLinter.isValidForStatementWithMaxLength(current, arrayName)) {
        return true;
      }
      current = current.parent;
    }

    return false;
  }

  static isValidForStatementWithMaxLength(node: ts.Node, arrayName: string): boolean {
    if (!ts.isForStatement(node)) {
      return false;
    }

    const forStmt = node;
    if (!forStmt.condition || !ts.isBinaryExpression(forStmt.condition)) {
      return false;
    }

    const condition = forStmt.condition;
    const isValidOperator =
      condition.operatorToken.kind === ts.SyntaxKind.LessThanToken ||
      condition.operatorToken.kind === ts.SyntaxKind.LessThanEqualsToken;

    return isValidOperator && TypeScriptLinter.isMathMaxCallWithArrayLength(condition.right, arrayName);
  }

  static isMathMaxCallWithArrayLength(expr: ts.Expression, arrayName: string): boolean {
    if (!ts.isCallExpression(expr)) {
      return false;
    }

    if (
      !(
        ts.isPropertyAccessExpression(expr.expression) &&
        expr.expression.name.text === 'max' &&
        ts.isIdentifier(expr.expression.expression) &&
        expr.expression.expression.text === 'Math'
      )
    ) {
      return false;
    }

    return (
      expr.arguments?.some((arg) => {
        return (
          ts.isPropertyAccessExpression(arg) &&
          arg.name.text === LENGTH_IDENTIFIER &&
          ts.isIdentifier(arg.expression) &&
          arg.expression.text === arrayName
        );
      }) ?? false
    );
  }

  static hasDefaultValueProtection(accessExpr: ts.ElementAccessExpression): boolean {
    if (
      ts.isBinaryExpression(accessExpr.parent) &&
      accessExpr.parent.operatorToken.kind === ts.SyntaxKind.BarBarToken
    ) {
      const defaultValue = accessExpr.parent.right;
      return ts.isNumericLiteral(defaultValue) || ts.isIdentifier(defaultValue) && defaultValue.text === 'undefined';
    }
    return false;
  }

  private isDirectLengthCheckWithNumber(accessExpr: ts.ElementAccessExpression): boolean {
    if (!ts.isBlock(accessExpr.parent.parent) || !ts.isIfStatement(accessExpr.parent.parent.parent)) {
      return false;
    }

    const ifStatement = accessExpr.parent.parent.parent;

    if (!ts.isBinaryExpression(ifStatement.expression)) {
      return false;
    }

    const condition = ifStatement.expression;
    const arrayAccessInfo = this.getArrayAccessInfo(accessExpr);
    if (!arrayAccessInfo) {
      return false;
    }

    if (
      !ts.isPropertyAccessExpression(condition.left) ||
      condition.left.name.text !== LENGTH_IDENTIFIER ||
      !ts.isIdentifier(condition.left.expression) ||
      condition.left.expression.text !== arrayAccessInfo.arrayIdent.text
    ) {
      return false;
    }

    if (!ts.isNumericLiteral(condition.right)) {
      return false;
    }

    if (!ts.isNumericLiteral(accessExpr.argumentExpression)) {
      return false;
    }

    const conditionNumber = parseFloat(condition.right.text);
    const accessNumber = parseFloat(accessExpr.argumentExpression.text);

    switch (condition.operatorToken.kind) {
      case ts.SyntaxKind.GreaterThanToken:
        return accessNumber <= conditionNumber;
      case ts.SyntaxKind.GreaterThanEqualsToken:
        return accessNumber < conditionNumber;
      default:
        return false;
    }
  }

  private isInLoopWithArrayLengthModification(accessExpr: ts.ElementAccessExpression, arrayName: string): boolean {
    let current: ts.Node | undefined = accessExpr;
    let inLoop = false;

    while (current) {
      if (ts.isForStatement(current) || ts.isWhileStatement(current) || ts.isDoStatement(current)) {
        inLoop = true;
        break;
      }
      current = current.parent;
    }

    if (!inLoop) {
      return false;
    }

    return this.checkArrayLengthModifiedBeforeAccess(accessExpr, arrayName);
  }

  private checkArrayLengthModifiedBeforeAccess(accessExpr: ts.ElementAccessExpression, arrayName: string): boolean {
    const container = TypeScriptLinter.findContainingBlock(accessExpr);
    if (!container) {
      return false;
    }

    const state = { foundModification: false, foundAccess: false };
    this.checkForArrayModificationBeforeAccess(container, accessExpr, arrayName, state);
    return state.foundModification;
  }

  private checkForArrayModificationBeforeAccess(
    node: ts.Node,
    accessExpr: ts.ElementAccessExpression,
    arrayName: string,
    state: { foundModification: boolean; foundAccess: boolean }
  ): void {
    if (node === accessExpr) {
      state.foundAccess = true;
      return;
    }

    if (state.foundAccess || state.foundModification) {
      return;
    }

    if (TypeScriptLinter.isArrayModification(node, arrayName)) {
      state.foundModification = true;
      return;
    }

    ts.forEachChild(node, (child) => {
      this.checkForArrayModificationBeforeAccess(child, accessExpr, arrayName, state);
    });
  }

  static isArrayModification(node: ts.Node, arrayName: string): boolean {
    return (
      TypeScriptLinter.isArrayModificationCall(node, arrayName) || TypeScriptLinter.isLengthAssignment(node, arrayName)
    );
  }

  static isArrayModificationCall(node: ts.Node, arrayName: string): boolean {
    if (!ts.isCallExpression(node)) {
      return false;
    }
    const expr = node.expression;
    return (
      ts.isPropertyAccessExpression(expr) &&
      ts.isIdentifier(expr.expression) &&
      expr.expression.text === arrayName &&
      ['pop', 'shift', 'splice'].includes(expr.name.text)
    );
  }

  static isLengthAssignment(node: ts.Node, arrayName: string): boolean {
    if (!ts.isBinaryExpression(node) || node.operatorToken.kind !== ts.SyntaxKind.EqualsToken) {
      return false;
    }
    return (
      ts.isPropertyAccessExpression(node.left) &&
      node.left.name.text === LENGTH_IDENTIFIER &&
      ts.isIdentifier(node.left.expression) &&
      node.left.expression.text === arrayName
    );
  }

  static findContainingBlock(node: ts.Node): ts.Block | undefined {
    let current: ts.Node | undefined = node;
    while (current) {
      if (ts.isBlock(current)) {
        return current;
      }
      current = current.parent;
    }
    return undefined;
  }

  private isObjectPropertyAccess(accessExpr: ts.ElementAccessExpression): boolean {
    if (this.isObjectAccess(accessExpr)) {
      return true;
    }

    let current: ts.Node = accessExpr.expression;
    while (current) {
      if (ts.isElementAccessExpression(current)) {
        if (this.isObjectAccess(current)) {
          return true;
        }
        current = current.expression;
        continue;
      }
      if (ts.isPropertyAccessExpression(current)) {
        return true;
      }
      break;
    }

    return false;
  }

  private isObjectAccess(accessExpr: ts.ElementAccessExpression): boolean {
    const baseType = this.tsTypeChecker.getTypeAtLocation(accessExpr.expression);

    const isObjectType = (baseType.flags & ts.TypeFlags.Object) !== 0;
    if (!isObjectType) {
      return false;
    }

    const indexType = this.tsTypeChecker.getTypeAtLocation(accessExpr.argumentExpression);
    const isStringIndex =
      (indexType.flags & ts.TypeFlags.StringLike) !== 0 || (indexType.flags & ts.TypeFlags.StringLiteral) !== 0;

    if (isStringIndex) {
      return true;
    }

    const symbol = this.tsUtils.trueSymbolAtLocation(accessExpr.expression);
    if (!symbol) {
      return false;
    }

    const declarations = symbol.getDeclarations();
    if (!declarations) {
      return false;
    }

    for (const decl of declarations) {
      if (!ts.isParameter(decl) && !ts.isVariableDeclaration(decl)) {
        continue;
      }

      const typeNode = decl.type;
      if (!typeNode || !ts.isTypeReferenceNode(typeNode)) {
        continue;
      }

      if (ts.isIdentifier(typeNode.typeName) && typeNode.typeName.text === 'Record') {
        return true;
      }
    }

    return false;
  }

  private isInStandardLengthControlledLoop(accessExpr: ts.ElementAccessExpression): boolean {
    const arrayAccessInfo = this.getArrayAccessInfo(accessExpr);
    if (!arrayAccessInfo) {
      return false;
    }
    let parent: ts.Node | undefined = accessExpr;
    while (parent && !ts.isForStatement(parent)) {
      parent = parent.parent;
    }
    if (!parent) {
      return false;
    }

    const forStmt = parent;

    if (!forStmt.condition || !ts.isBinaryExpression(forStmt.condition)) {
      return false;
    }

    const condition = forStmt.condition;
    const isStandardLoop =
      (condition.operatorToken.kind === ts.SyntaxKind.LessThanToken ||
        condition.operatorToken.kind === ts.SyntaxKind.LessThanEqualsToken) &&
      ts.isPropertyAccessExpression(condition.right) &&
      condition.right.name.text === LENGTH_IDENTIFIER;

    if (!isStandardLoop) {
      return false;
    }

    return !this.hasDangerousArrayOperationsInForLoop(forStmt, arrayAccessInfo.arrayIdent.text);
  }

  private hasDangerousArrayOperationsInForLoop(forStmt: ts.ForStatement, arrayName: string): boolean {
    if (this.checkArrayModifications(forStmt.statement, arrayName)) {
      return true;
    }

    if (forStmt.initializer && ts.isVariableDeclarationList(forStmt.initializer)) {
      const indexVar = forStmt.initializer.declarations[0]?.name.getText();
      if (indexVar && this.checkIndexModifications(forStmt.statement, indexVar)) {
        return true;
      }
    }

    if (this.checkOutOfBoundAccess(forStmt.statement, arrayName)) {
      return true;
    }

    return false;
  }

  private checkArrayModifications(node: ts.Node, arrayName: string): boolean {
    let hasModification = false;
    ts.forEachChild(node, (child) => {
      if (TypeScriptLinter.isArrayModification(child, arrayName)) {
        hasModification = true;
      }
      if (!hasModification) {
        hasModification = this.checkArrayModifications(child, arrayName);
      }
    });
    return hasModification;
  }

  private checkIndexModifications(node: ts.Node, indexVar: string): boolean {
    let hasModification = false;
    ts.forEachChild(node, (child) => {
      if (
        ts.isBinaryExpression(child) &&
        child.operatorToken.kind === ts.SyntaxKind.EqualsToken &&
        ts.isIdentifier(child.left) &&
        child.left.text === indexVar
      ) {
        hasModification = true;
      }
      if (!hasModification) {
        hasModification = this.checkIndexModifications(child, indexVar);
      }
    });
    return hasModification;
  }

  private checkOutOfBoundAccess(node: ts.Node, arrayName: string): boolean {
    let hasOutOfBound = false;
    ts.forEachChild(node, (child) => {
      if (
        ts.isElementAccessExpression(child) &&
        ts.isIdentifier(child.expression) &&
        child.expression.text === arrayName &&
        ts.isNumericLiteral(child.argumentExpression)
      ) {
        hasOutOfBound = true;
      }
      if (!hasOutOfBound) {
        hasOutOfBound = this.checkOutOfBoundAccess(child, arrayName);
      }
    });
    return hasOutOfBound;
  }

  private handleCallExpressionForRepeat(node: ts.CallExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    if (
      !ts.isIdentifier(node.expression) ||
      node.expression.getText() !== CustomInterfaceName.Repeat ||
      this.isDeclarationInSameFile(node.expression)
    ) {
      return;
    }

    const stmt = ts.findAncestor(node, ts.isExpressionStatement);
    if (!stmt || TsUtils.checkStmtHasTargetIdentifier(stmt, VIRTUAL_SCROLL_IDENTIFIER)) {
      return;
    }

    const autofix = this.autofixer?.fixRepeat(stmt);
    this.incrementCounters(node, FaultID.RepeatDisableVirtualScroll, autofix);
  }

  private handleNodeForWrappedBuilder(node: ts.TypeReferenceNode | ts.NewExpression | ts.CallExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    const identifier = ts.isTypeReferenceNode(node) ? node.typeName : node.expression;
    if (this.isDeclarationInSameFile(identifier)) {
      return;
    }

    switch (identifier.getText()) {
      case CustomInterfaceName.WrappedBuilder:
        this.incrementCounters(node, FaultID.WrappedBuilderGenericNeedArrowFunc);
        break;
      case CustomInterfaceName.wrapBuilder: {
        const args = node.typeArguments;
        if (args && args.length > 0 && ts.isTupleTypeNode(args[0])) {
          this.incrementCounters(node, FaultID.WrapBuilderGenericNeedArrowFunc);
        }
        break;
      }
      default:
    }
  }

  private checkImportJsonFile(node: ts.ImportDeclaration): void {
    if (!this.options.arkts2) {
      return;
    }

    const moduleSpecifier = node.moduleSpecifier;

    if (!ts.isStringLiteral(moduleSpecifier)) {
      return;
    }

    const importPath = moduleSpecifier.text;

    if (importPath.endsWith(EXTNAME_JSON)) {
      this.incrementCounters(moduleSpecifier, FaultID.NoImportJsonFile);
    }
  }

  private handleNoDeprecatedApi(
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
      | ts.PropertyAssignment
      | ts.PropertyAccessExpression
      | ts.ElementAccessExpression
      | ts.HeritageClause
      | ts.TaggedTemplateExpression
  ): void {
    if (!this.options.arkts2) {
      return;
    }
    switch (node.kind) {
      case ts.SyntaxKind.TypeReference:
        this.checkTypeReferenceForDeprecatedApi(node);
        break;
      case ts.SyntaxKind.HeritageClause:
        this.checkHeritageClauseForDeprecatedApi(node);
        break;
      case ts.SyntaxKind.PropertyDeclaration:
        this.checkPropertyDeclarationForDeprecatedApi(node);
        break;
      case ts.SyntaxKind.Parameter:
      case ts.SyntaxKind.VariableDeclaration:
        this.checkDeclarationForDeprecatedApi(node);
        break;
      case ts.SyntaxKind.MethodDeclaration:
        this.checkMethodDeclarationForDeprecatedApi(node);
        break;
      case ts.SyntaxKind.PropertyAssignment:
      case ts.SyntaxKind.TaggedTemplateExpression:
      case ts.SyntaxKind.NewExpression:
      case ts.SyntaxKind.CallExpression:
      case ts.SyntaxKind.BinaryExpression:
      case ts.SyntaxKind.PropertyAccessExpression:
      case ts.SyntaxKind.ElementAccessExpression:
        this.handleNoDeprecatedApiForExpression(node);
        break;
      default:
    }
  }

  handleNoDeprecatedApiForExpression(
    node:
      | ts.NewExpression
      | ts.CallExpression
      | ts.BinaryExpression
      | ts.PropertyAccessExpression
      | ts.ElementAccessExpression
      | ts.TaggedTemplateExpression
      | ts.PropertyAssignment
  ): void {
    switch (node.kind) {
      case ts.SyntaxKind.NewExpression:
        this.checkNewExpressionForDeprecatedApi(node);
        break;
      case ts.SyntaxKind.CallExpression:
        this.checkCallExpressionForDeprecatedApi(node);
        break;
      case ts.SyntaxKind.BinaryExpression:
        this.checkBinaryExpressionForDeprecatedApi(node);
        break;
      case ts.SyntaxKind.PropertyAccessExpression:
        this.checkPropertyAccessExpressionForDeprecatedApi(node);
        break;
      case ts.SyntaxKind.ElementAccessExpression:
        this.checkSdkCommonOnElementAccess(node);
        break;
      case ts.SyntaxKind.TaggedTemplateExpression:
        this.checkTaggedTemplateExpressionForBuiltinApi(node);
        break;
      case ts.SyntaxKind.PropertyAssignment:
        this.checkPropertyAssignmentForDeprecatedApi(node);
        break;
      default:
    }
  }

  private checkSdkCommonOnElementAccess(elemAccessExp: ts.ElementAccessExpression): void {
    const indexAccess = elemAccessExp.argumentExpression;
    if (!indexAccess || !ts.isNumericLiteral(indexAccess)) {
      return;
    }
    let express = elemAccessExp.expression;
    const isNewExpression = ts.isNewExpression(elemAccessExp.expression);
    if (isNewExpression) {
      express = elemAccessExp.expression.expression;
    }
    const exprSym = this.tsUtils.trueSymbolAtLocation(express);
    const exprDecl = TsUtils.getDeclaration(exprSym);
    if (!exprDecl) {
      return;
    }
    if (exprSym && isNewExpression) {
      this.reportForSdkCommonOnElementAccess(
        elemAccessExp,
        exprSym.name,
        path.basename(exprDecl.getSourceFile().fileName)
      );
      return;
    }
    if (!ts.isVariableDeclaration(exprDecl)) {
      return;
    }
    const initializer = exprDecl.initializer;

    if (!initializer || !ts.isNewExpression(initializer)) {
      return;
    }
    const constructorIdentifier = initializer.expression;
    if (!constructorIdentifier || !ts.isIdentifier(constructorIdentifier)) {
      return;
    }
    const decl = this.tsUtils.getDeclarationNode(constructorIdentifier);
    this.reportForSdkCommonOnElementAccess(
      elemAccessExp,
      constructorIdentifier.text,
      path.basename(decl?.getSourceFile().fileName + '')
    );
  }

  private reportForSdkCommonOnElementAccess(node: ts.Node, importName: string, filePath: string): void {
    if (TypeScriptLinter.isImportedFromOhos(importName, filePath)) {
      this.incrementCounters(
        node,
        FaultID.SdkCommonApiWhiteList,
        undefined,
        TypeScriptLinter.getErrorMsgForSdkCommonApi(importName, FaultID.SdkCommonApiWhiteList)
      );
    }
  }

  private checkTypeReferenceForDeprecatedApi(node: ts.TypeReferenceNode): void {
    let typeName = node.typeName;
    if (ts.isQualifiedName(node.typeName)) {
      typeName = node.typeName.right;
    }
    const sym = this.tsUtils.trueSymbolAtLocation(typeName);
    const decl = TsUtils.getDeclaration(sym);
    if (sym) {
      this.hanldeSdkCommonTypeName(node, sym, sym.name, decl);
    }
    if (decl && (ts.isInterfaceDeclaration(decl) || ts.isClassDeclaration(decl) || ts.isTypeAliasDeclaration(decl))) {
      let parentName = decl.name ? decl.name.text : 'unnamed';
      if (ts.isQualifiedName(node.typeName)) {
        parentName = node.typeName.getText();
      } else if (ts.isTypeAliasDeclaration(decl)) {
        parentName = '';
      }
      const deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
        parentName,
        undefined,
        ts.isTypeAliasDeclaration(decl) ? decl.type.getText() : undefined,
        path.basename(decl.getSourceFile().fileName + '')
      );
      this.processApiNodeDeprecatedApi(typeName.getText(), typeName, deprecatedApiCheckMap);
      this.processApiNodeDeprecatedApi(typeName.getText(), typeName, deprecatedApiCheckMap, undefined, BUILTIN_TYPE);
    }
  }

  private checkNewExpressionForDeprecatedApi(node: ts.NewExpression): void {
    const expression = node.expression;
    this.checkNewExpressionForSdkApi(node);
    if (ts.isIdentifier(expression)) {
      const decl = this.tsUtils.getDeclarationNode(expression);
      if (decl && ts.isClassDeclaration(decl)) {
        const deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
          decl.name?.text + '',
          undefined,
          undefined,
          path.basename(decl.getSourceFile().fileName + '')
        );
        this.processApiNodeDeprecatedApi(expression.text, expression, deprecatedApiCheckMap);
      }
    }
  }

  private checkNewExpressionForSdkApi(newExpr: ts.NewExpression): void {
    const type = this.tsTypeChecker.getTypeAtLocation(newExpr.expression);
    const resolvedSignature = this.tsTypeChecker.getResolvedSignature(newExpr);
    if (!resolvedSignature) {
      return;
    }
    const constructorDeclaration = resolvedSignature.declaration;
    const parentName = type.symbol ?
      this.tsTypeChecker.getFullyQualifiedName(type.symbol) :
      newExpr.expression.getText();
    if (constructorDeclaration) {
      const deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
        parentName,
        constructorDeclaration.parameters as ts.NodeArray<ts.ParameterDeclaration>,
        SDK_COMMON_VOID,
        path.basename(constructorDeclaration.getSourceFile().fileName + '')
      );
      this.processApiNodeDeprecatedApi(
        SDK_COMMON_CONSTRUCTOR,
        newExpr.expression,
        deprecatedApiCheckMap,
        undefined,
        SDK_COMMON_TYPE
      );
      if (BUILTIN_CALLSIGNATURE_NEWCTOR.includes(newExpr.expression.getText())) {
        this.handleNewExpressionForBuiltNewCtor(newExpr.expression, deprecatedApiCheckMap);
      } else {
        this.processApiNodeDeprecatedApi(
          BUILTIN_CONSTRUCTOR_API_NAME,
          newExpr.expression,
          deprecatedApiCheckMap,
          undefined,
          BUILTIN_TYPE
        );
      }
    }
  }

  private handleNewExpressionForBuiltNewCtor(
    errorNode: ts.Node,
    deprecatedApiCheckMap?: Map<string, string | ts.NodeArray<ts.ParameterDeclaration>>
  ): void {
    if (TypeScriptLinter.builtinNewCtorSet.size === 0 || !deprecatedApiCheckMap) {
      return;
    }
    [...TypeScriptLinter.builtinNewCtorSet].some((item) => {
      if (item.api_info.parent_api?.length <= 0) {
        return false;
      }
      const isBuiltinNewConstruct =
        BUILTIN_CONSTRUCTOR_API_TYPE.includes(item.api_info.api_type) &&
        item.api_info.parent_api[0].api_name === deprecatedApiCheckMap?.get(DEPRECATE_CHECK_KEY.PARENT_NAME) + '' &&
        path.basename(item.file_path) === deprecatedApiCheckMap?.get(DEPRECATE_CHECK_KEY.FILE_NAME) + '';
      if (isBuiltinNewConstruct) {
        const problemStr = item.api_info.problem;
        if (problemStr.length > 0) {
          this.incrementCounters(errorNode, FaultID.BuiltinNewCtor);
        }
        return true;
      }
      return false;
    });
  }

  private checkHeritageClauseForDeprecatedApi(node: ts.HeritageClause): void {
    node.types.forEach((type) => {
      let expr = type.expression;
      if (ts.isIdentifier(expr)) {
        this.checkHeritageClauseForDeprecatedApiOnIdentifier(expr);
      }
      if (ts.isPropertyAccessExpression(type.expression) && ts.isIdentifier(type.expression.name)) {
        expr = type.expression.name;
      }
      const decl = this.tsUtils.getDeclarationNode(expr);
      this.checkHeritageClauseForSdkApiDeprecated(node, decl, SDK_COMMON_TYPE);
      this.checkHeritageClauseForSdkApiDeprecated(node, decl, BUILTIN_TYPE);
    });
  }

  private checkHeritageClauseForSdkApiDeprecated(
    node: ts.HeritageClause,
    decl: ts.Node | undefined,
    apiType: string
  ): void {
    if (
      decl &&
      (ts.isClassDeclaration(decl) || ts.isInterfaceDeclaration(decl)) &&
      ts.isClassDeclaration(node.parent) &&
      decl.name
    ) {
      const extendClassName = decl.name.text;
      const newSet = TypeScriptLinter.refactorSetWhitSameAsParenName(extendClassName, apiType);
      if (newSet && newSet.size > 0) {
        const sourceFunlikeArrs = node.parent.members.filter(ts.isFunctionLike);
        const sourceProDeclArrs = node.parent.members.filter(ts.isPropertyDeclaration);
        this.checkSdkApiInfoWithClassMember(sourceFunlikeArrs, decl, SDK_COMMON_TYPEKEY[0], apiType, newSet);
        this.checkSdkApiInfoWithClassMember(sourceProDeclArrs, decl, SDK_COMMON_TYPEKEY[1], apiType, newSet);
      }
    }
  }

  private checkSdkApiInfoWithClassMember(
    sourceMembers: ts.ClassElement[] | ts.PropertyDeclaration[],
    decl: ts.ClassDeclaration | ts.InterfaceDeclaration,
    typeKey: string,
    apiType: string,
    mergedSet: Set<ApiListItem>
  ): void {
    sourceMembers.some((func) => {
      if (!func.name || !decl.name) {
        return;
      }
      const funcName = func.name.getText();
      const extendClassName = decl.name.text;
      const problemStr = TypeScriptLinter.getFaultIdSdkApiInfoWithClassMember(decl, funcName, typeKey, mergedSet);
      if (problemStr) {
        let faultID = TypeScriptLinter.getFinalSdkFaultIdByProblem(problemStr, apiType);
        if (apiType === SDK_COMMON_TYPE) {
          faultID = sdkCommonAllDeprecatedTypeName.has(extendClassName) ? FaultID.SdkCommonApiDeprecated : faultID;
        }
        if (!faultID) {
          return;
        }
        this.incrementCounters(
          func,
          faultID,
          undefined,
          apiType === SDK_COMMON_TYPE ?
            TypeScriptLinter.getErrorMsgForSdkCommonApi(extendClassName, faultID) :
            undefined
        );
      }
    });
  }

  private checkHeritageClauseForDeprecatedApiOnIdentifier(node: ts.Identifier): void {
    const sym = this.tsUtils.trueSymbolAtLocation(node);
    const decl = this.tsUtils.getDeclarationNode(node);
    if (decl && (ts.isInterfaceDeclaration(decl) || ts.isClassDeclaration(decl))) {
      const fileName = path.basename(decl.getSourceFile().fileName + '');
      const deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
        decl.name?.getText() + '',
        undefined,
        undefined,
        fileName
      );
      this.processApiNodeDeprecatedApi(node.getText(), node, deprecatedApiCheckMap);
      this.hanldeSdkCommonTypeName(node, sym, decl.name?.getText() + '', decl);
      this.hanldeBuiltinFinalClassOnHeritageClause(node, fileName);
    }
  }

  private hanldeBuiltinFinalClassOnHeritageClause(node: ts.Node, fileName: string): void {
    let isMatch = false;
    for (const item of TypeScriptLinter.builtinFinalClassSet) {
      isMatch = item.api_info.api_name === node.getText() && path.basename(item.file_path) === fileName;
      if (isMatch) {
        this.incrementCounters(node, FaultID.BuiltinFinalClass);
        break;
      }
    }
  }

  private checkDeclarationForDeprecatedApi(
    node: ts.VariableDeclaration | ts.PropertyDeclaration | ts.ParameterDeclaration
  ): void {
    const expression = node.initializer;
    const getParaExcute = (expression: ts.Expression): void => {
      if (expression && ts.isIdentifier(expression)) {
        const funSymbol = this.tsUtils.trueSymbolAtLocation(expression);
        const decl = TsUtils.getDeclaration(funSymbol);
        const parName = this.tsUtils.getParentSymbolName(funSymbol);
        if (decl && (ts.isFunctionLike(decl) || ts.isVariableDeclaration(decl))) {
          const returnType = decl.type?.getText();
          const deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
            parName === undefined ? DEPRECATE_UNNAMED : parName + '',
            undefined,
            returnType,
            path.basename(decl.getSourceFile().fileName)
          );
          this.processApiNodeDeprecatedApi(expression.text, expression, deprecatedApiCheckMap);
        }
      }
    };

    if (expression && ts.isIdentifier(expression)) {
      getParaExcute(expression);
    } else if (expression && ts.isObjectLiteralExpression(expression)) {
      const properties = expression.properties;
      for (const prop of properties) {
        const propExpression = ts.isPropertyAssignment(prop) && prop.initializer;
        if (propExpression && ts.isIdentifier(propExpression)) {
          getParaExcute(propExpression);
        }
      }
    }
  }

  private checkCallExpressionForDeprecatedApi(node: ts.CallExpression): void {
    let name: ts.Identifier | undefined;
    if (ts.isIdentifier(node.expression)) {
      name = node.expression;
    } else if (ts.isPropertyAccessExpression(node.expression)) {
      name = ts.isIdentifier(node.expression.name) ? node.expression.name : undefined;
    }
    if (!name) {
      return;
    }
    let funSymbol = this.tsUtils.trueSymbolAtLocation(name);
    if (!funSymbol && ts.isPropertyAccessExpression(node.expression)) {
      funSymbol = this.tsTypeChecker.getSymbolAtLocation(node.expression.expression);
    }
    const isNeedGetResolvedSignature = funSymbol?.declarations && funSymbol.declarations.length > 1;
    const decl = TsUtils.getDeclaration(funSymbol);
    const parName = this.tsUtils.getParentSymbolName(funSymbol);
    this.handleCallExpressionBufferIndexOf(node, name, parName + '', funSymbol, decl);
    const deprecatedApiCheckMap = TypeScriptLinter.getDeprecatedApiCheckMapForCallExpression(decl, parName);
    this.reportDeprecatedApi(node, name, deprecatedApiCheckMap);
    this.checkCallExpressionForSdkApi(node, name, parName, !!isNeedGetResolvedSignature, deprecatedApiCheckMap);
    this.checkSpecialApiForDeprecatedApi(node, name, decl);
  }

  private static getDeprecatedApiCheckMapForCallExpression(
    decl: ts.Node | undefined,
    parName: string | undefined
  ): Map<string, string | ts.NodeArray<ts.ParameterDeclaration>> | undefined {
    if (decl && (ts.isFunctionLike(decl) || ts.isVariableDeclaration(decl))) {
      const deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
        parName === undefined ? DEPRECATE_UNNAMED : parName + '',
        ts.isFunctionLike(decl) ? decl.parameters : undefined,
        decl.type?.getText() === undefined ? 'any' : decl.type?.getText() + '',
        path.basename(decl.getSourceFile().fileName)
      );
      return deprecatedApiCheckMap;
    }
    return undefined;
  }

  private checkCallExpressionForSdkApi(
    node: ts.CallExpression,
    name: ts.Identifier,
    parName: string | undefined,
    isNeedGetResolvedSignature: boolean,
    deprecatedApiCheckMap: Map<string, string | ts.NodeArray<ts.ParameterDeclaration>> | undefined
  ): void {
    if (isNeedGetResolvedSignature) {
      this.checkCallExpressionForSdkApiWithSignature(node, name, parName);
    } else {
      this.reportDeprecatedApi(node, name, deprecatedApiCheckMap, SDK_COMMON_TYPE);
      this.reportDeprecatedApi(node, name, deprecatedApiCheckMap, BUILTIN_TYPE);
    }
  }

  private checkCallExpressionForSdkApiWithSignature(
    node: ts.CallExpression,
    name: ts.Identifier,
    parName: string | undefined
  ): void {
    const signature = this.tsTypeChecker.getResolvedSignature(node);
    if (!signature?.declaration) {
      return;
    }
    const functionSymbol = this.getFunctionSymbol(signature.declaration);
    const functionDeclaration = functionSymbol?.valueDeclaration;
    let returnType = this.tsTypeChecker.typeToString(signature.getReturnType());
    let isSpecialTypeForBuiltIn = false;
    if (!functionDeclaration) {
      const signatureDecl = signature.getDeclaration();
      if (signatureDecl && ts.isFunctionLike(signatureDecl) && signatureDecl.type) {
        returnType = signatureDecl.type.getText();
        if (!parName && BUILTIN_CALLSIGNATURE_NEWCTOR.includes(name.text)) {
          const type = this.tsTypeChecker.getTypeAtLocation(name);
          parName = this.tsTypeChecker.typeToString(type);
          isSpecialTypeForBuiltIn = !!parName;
        }
      }
    }
    const fileName = signature.getDeclaration().getSourceFile().fileName;
    const deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
      parName === undefined ? '' : parName + '',
      TypeScriptLinter.getParameterDeclarationsBySignature(signature),
      returnType,
      path.basename(fileName)
    );
    this.reportDeprecatedApi(node, name, deprecatedApiCheckMap, SDK_COMMON_TYPE);
    if (isSpecialTypeForBuiltIn) {
      this.processApiNodeDeprecatedApi(
        BUILTIN_CONSTRUCTOR_API_NAME,
        name,
        deprecatedApiCheckMap,
        undefined,
        BUILTIN_TYPE
      );
    } else {
      this.reportDeprecatedApi(node, name, deprecatedApiCheckMap, BUILTIN_TYPE);
    }
  }

  private reportDeprecatedApi(
    node: ts.CallExpression,
    name: ts.Identifier,
    deprecatedApiCheckMap?: Map<string, string | ts.NodeArray<ts.ParameterDeclaration>>,
    apiType?: string
  ): void {
    const problemStr = this.getFaultIdWithMatchedDeprecatedApi(name.text, deprecatedApiCheckMap, apiType);
    if (problemStr.length > 0) {
      const autofix = this.autofixer?.fixDeprecatedApiForCallExpression(node);
      if (autofix) {
        this.interfacesNeedToImport.add('getUIContext');
      }
      const isSdkCommon = apiType === SDK_COMMON_TYPE;
      const faultID = TypeScriptLinter.getFinalSdkFaultIdByProblem(problemStr, apiType);
      if (!faultID) {
        return;
      }
      this.incrementCounters(
        name,
        faultID,
        isSdkCommon || apiType === BUILTIN_TYPE ? undefined : autofix,
        isSdkCommon || apiType === undefined ?
          TypeScriptLinter.getErrorMsgForSdkCommonApi(name.text, faultID) :
          undefined
      );
    }
  }

  private static getFinalSdkFaultIdByProblem(problem: string, apiType: string | undefined): number | undefined {
    let sdkFaultId: number | undefined = FaultID.NoDeprecatedApi;
    if (apiType === SDK_COMMON_TYPE) {
      sdkFaultId = SdkCommonApiProblemInfos.get(problem);
      return sdkFaultId ? sdkFaultId : FaultID.SdkCommonApiWhiteList;
    } else if (apiType === BUILTIN_TYPE) {
      sdkFaultId = BuiltinProblemInfos.get(problem);
      return sdkFaultId ? sdkFaultId : undefined;
    }
    return sdkFaultId;
  }

  private checkSpecialApiForDeprecatedApi(
    node: ts.CallExpression,
    name: ts.Identifier,
    decl: ts.Declaration | undefined
  ): void {
    if (('mask' === name.getText() || 'clip' === name.getText()) && node.arguments.length === 1) {
      const types = ['CircleAttribute', 'EllipseAttribute', ' PathAttribute', 'RectAttribute'];
      const arg = node.arguments[0];
      const argType = this.tsTypeChecker.typeToString(this.tsTypeChecker.getTypeAtLocation(arg));
      if (types.includes(argType)) {
        if (name.getText() === 'clip') {
          const typeMapping = {
            CircleAttribute: 'CircleShape',
            EllipseAttribute: 'EllipseShape',
            PathAttribute: 'PathShape',
            RectAttribute: 'RectShape'
          } as const;

          if (argType in typeMapping) {
            this.interfacesNeedToImport.add(typeMapping[argType as keyof typeof typeMapping]);
          }

          const autofix = this.autofixer?.fixSpecialDeprecatedApiForCallExpression(node, name);
          this.incrementCounters(name, FaultID.NoDeprecatedApi, autofix);
          return;
        }
        this.incrementCounters(name, FaultID.NoDeprecatedApi);
        return;
      }
    }
    if (decl?.parent && ts.isClassDeclaration(decl.parent) && 'onScroll' === name.getText()) {
      let parentName = '';
      decl.parent.heritageClauses?.forEach((clause) => {
        clause.types.forEach((type) => {
          if (ts.isExpressionWithTypeArguments(type)) {
            parentName = type.expression.getText();
          }
        });
      });
      if (parentName === 'ScrollableCommonMethod') {
        this.incrementCounters(name, FaultID.NoDeprecatedApi);
      }
    }
  }

  private checkBinaryExpressionForDeprecatedApi(node: ts.BinaryExpression): void {
    const expression = node.right;
    if (ts.isIdentifier(expression)) {
      this.processApiNodeDeprecatedApi(expression.text, expression);
    }
  }

  private checkMethodDeclarationForDeprecatedApi(node: ts.MethodDeclaration): void {
    const expression = node.name;
    if (!ts.isIdentifier(expression)) {
      return;
    }
    if (
      (expression.getText() === 'onLayout' || expression.getText() === 'onMeasure') &&
      node.type?.getText() === 'void' &&
      node.parent &&
      ts.isStructDeclaration(node.parent)
    ) {
      const argsType = ['LayoutChild[]', 'ConstraintSizeOptions'];
      const parameters = node.parameters;
      if (parameters && parameters.length === 2) {
        let paramMatch = true;
        for (let i = 0; i < parameters.length; i++) {
          if (this.tsTypeChecker.typeToString(this.tsTypeChecker.getTypeAtLocation(parameters[i])) !== argsType[i]) {
            paramMatch = false;
            break;
          }
        }
        if (paramMatch) {
          this.incrementCounters(
            expression,
            FaultID.NoDeprecatedApi,
            undefined,
            TypeScriptLinter.getErrorMsgForSdkCommonApi(expression.getText(), FaultID.NoDeprecatedApi)
          );
          return;
        }
      }
    }
    this.processApiNodeDeprecatedApi(expression.text, expression);
  }

  private checkPropertyAssignmentForDeprecatedApi(node: ts.PropertyAssignment): void {
    const expression = node.name;
    const contextualType = this.tsTypeChecker.getContextualType(node.parent);
    if (contextualType) {
      this.processApiNodeDeprecatedApi(
        expression.getText(),
        expression,
        this.getPropertyTypeForPropertyAssignment(node, contextualType)
      );
      this.processApiNodeDeprecatedApi(
        expression.getText(),
        expression,
        this.getPropertyTypeForPropertyAssignment(node, contextualType, true),
        undefined,
        SDK_COMMON_TYPE
      );
      this.processApiNodeDeprecatedApi(
        expression.getText(),
        expression,
        this.getPropertyTypeForPropertyAssignment(node, contextualType),
        undefined,
        BUILTIN_TYPE
      );
    }
  }

  private checkPropertyAccessExpressionForDeprecatedApi(node: ts.PropertyAccessExpression): void {
    this.handleSymbolIteratorForSdkCommon(node);
    node.forEachChild((expression) => {
      if (!ts.isIdentifier(expression)) {
        return;
      }
      const funSymbol = this.tsUtils.trueSymbolAtLocation(expression);
      const decl = TsUtils.getDeclaration(funSymbol);
      let parName = this.tsUtils.getParentSymbolName(funSymbol);
      this.hanldeSdkCommonTypeName(expression, funSymbol, parName ? parName : expression.text, decl);
      if (decl && TypeScriptLinter.checkIsAppropriateTypeWithNode(decl)) {
        let returnType: string | undefined = this.tsTypeChecker.typeToString(
          this.tsTypeChecker.getTypeAtLocation(decl)
        );
        if (ts.isPropertySignature(decl) && decl.type) {
          returnType = decl.type.getText();
        } else if (ts.isEnumMember(decl)) {
          returnType = TypeScriptLinter.getReturnTypeForEnumMember(decl);
        } else if (!parName && ts.isEnumDeclaration(decl)) {
          parName = decl.name.text;
          returnType = undefined;
        }
        const deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
          parName === undefined ? DEPRECATE_UNNAMED : parName + '',
          undefined,
          returnType,
          path.basename(decl.getSourceFile().fileName)
        );
        this.processApiNodeDeprecatedApi(expression.text, expression, deprecatedApiCheckMap);
        this.processApiNodeDeprecatedApi(
          expression.text,
          expression,
          deprecatedApiCheckMap,
          undefined,
          SDK_COMMON_TYPE
        );
        this.processApiNodeDeprecatedApi(expression.text, expression, deprecatedApiCheckMap, undefined, BUILTIN_TYPE);
      }
    });
  }

  private static checkIsAppropriateTypeWithNode(decl: ts.Node): boolean {
    return (
      ts.isPropertyDeclaration(decl) ||
      ts.isPropertySignature(decl) ||
      ts.isEnumMember(decl) ||
      ts.isEnumDeclaration(decl)
    );
  }

  private hanldeSdkCommonTypeName(
    node: ts.Node,
    symbol: ts.Symbol | undefined,
    parentName: string,
    decl?: ts.Declaration | undefined
  ): void {
    const filePath = decl?.getSourceFile().fileName;
    const newName = sdkCommonAllDeprecatedFullTypeName.has(symbol?.name + '') ? symbol?.name + '' : parentName;
    const isParentNameMatch = sdkCommonAllDeprecatedFullTypeName.has(newName);
    const newFilePath = path.basename(filePath + '');
    let isFilePathMatch = false;
    for (const item of TypeScriptLinter.sdkCommonAllDeprecatedTypeNameSet) {
      isFilePathMatch = path.basename(item.file_path) === newFilePath;
      if (isFilePathMatch) {
        break;
      }
    }
    const isMatch = isParentNameMatch && isFilePathMatch;
    if (isMatch || TypeScriptLinter.isJsonTransformer(decl)) {
      this.incrementCounters(
        node,
        FaultID.SdkCommonApiDeprecated,
        undefined,
        TypeScriptLinter.getErrorMsgForSdkCommonApi(newName, FaultID.SdkCommonApiDeprecated)
      );
    }
  }

  private handleCallExpressionBufferIndexOf(
    callExpr: ts.CallExpression,
    node: ts.Node,
    parentName: string,
    symbol?: ts.Symbol,
    decl?: ts.Declaration
  ): void {
    if (!symbol || !decl) {
      return;
    }

    const isIndexOfWithEmptyString = TypeScriptLinter.checkIsIndexOfWithEmptyString(callExpr);
    if (!isIndexOfWithEmptyString) {
      return;
    }
    const isNameMatch = symbol.name === SDK_COMMON_BUFFER_API.indexof && parentName === SDK_COMMON_BUFFER_API.full_api;
    const isPathMatch = TypeScriptLinter.isImportedFromOhos(
      SDK_COMMON_BUFFER_API.apiName,
      path.basename(decl.getSourceFile().fileName)
    );
    if (isNameMatch && isPathMatch) {
      this.incrementCounters(
        node,
        FaultID.SdkCommonApiBehaviorChange,
        undefined,
        TypeScriptLinter.getErrorMsgForSdkCommonApi(SDK_COMMON_BUFFER_API.indexof, FaultID.SdkCommonApiBehaviorChange)
      );
    }
  }

  private static checkIsIndexOfWithEmptyString(callExpr: ts.CallExpression): boolean {
    const isIndexOfCall =
      ts.isPropertyAccessExpression(callExpr.expression) &&
      SDK_COMMON_BUFFER_API.indexof === callExpr.expression.name.text;
    const hasEmptyStringArgument =
      callExpr.arguments.length === 1 && ts.isStringLiteral(callExpr.arguments[0]) && callExpr.arguments[0].text === '';

    const hasNoArguments = callExpr.arguments.length === 0;

    return isIndexOfCall && (hasEmptyStringArgument || hasNoArguments);
  }

  private static isJsonTransformer(decl: ts.Declaration | undefined): boolean {
    if (
      decl &&
      ts.isTypeAliasDeclaration(decl) &&
      ts.isFunctionTypeNode(decl.type) &&
      decl.name.getText() === SDK_COMMON_TRANSFORMER
    ) {
      return decl.type.parameters.length > 0 && decl.type.parameters[0].name.getText() === 'this';
    }
    return false;
  }

  private handleSymbolIteratorForSdkCommon(decl: ts.PropertyAccessExpression): boolean {
    if (
      this.checkPropertyAccessExpressionForSdkCommonSymbotIter(
        decl,
        SDK_COMMON_SYMBOL_ITERATOR,
        TypeScriptLinter.sdkCommonSymbotIterSet
      )
    ) {
      this.incrementCounters(
        decl,
        FaultID.SdkCommonApiWhiteList,
        undefined,
        TypeScriptLinter.getErrorMsgForSdkCommonApi(SDK_COMMON_SYMBOL_ITERATOR, FaultID.SdkCommonApiWhiteList)
      );
      return true;
    }
    return false;
  }

  private checkPropertyAccessExpressionForSdkCommonSymbotIter(
    node: ts.PropertyAccessExpression,
    name: string,
    set: Set<ApiListItem>
  ): boolean {
    if (set.size === 0 || node.getText() !== name) {
      return false;
    }
    let isFileMatch = false;
    if (node.parent && ts.isElementAccessExpression(node.parent)) {
      let decl = this.tsUtils.getDeclarationNode(node.parent.expression);
      if (decl && ts.isVariableDeclaration(decl) && decl.initializer && ts.isNewExpression(decl.initializer)) {
        decl = this.tsUtils.getDeclarationNode(decl.initializer.expression);
      }
      if (ts.isNewExpression(node.parent.expression)) {
        decl = this.tsUtils.getDeclarationNode(node.parent.expression.expression);
      }
      const fileName = path.basename(decl?.getSourceFile().fileName + '');
      for (const item of set) {
        isFileMatch = path.basename(item.file_path) === fileName;
        if (isFileMatch) {
          break;
        }
      }
    }
    return isFileMatch;
  }

  private checkTaggedTemplateExpressionForBuiltinApi(node: ts.TaggedTemplateExpression): void {
    const expression = node.tag;
    if (ts.isPropertyAccessExpression(expression)) {
      const funSymbol = this.tsUtils.trueSymbolAtLocation(expression.name);
      const decl = TsUtils.getDeclaration(funSymbol);
      const parName = this.tsUtils.getParentSymbolName(funSymbol);
      if (decl) {
        const returnType: string | undefined = this.tsTypeChecker.typeToString(
          this.tsTypeChecker.getTypeAtLocation(decl)
        );
        const deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
          parName === undefined ? DEPRECATE_UNNAMED : parName + '',
          undefined,
          expression.name.text === 'raw' ? 'string' : returnType,
          path.basename(decl.getSourceFile().fileName)
        );
        this.processApiNodeDeprecatedApi(
          expression.name.text,
          expression.name,
          deprecatedApiCheckMap,
          undefined,
          BUILTIN_TYPE
        );
      }
    }
  }

  private checkPropertyDeclarationForDeprecatedApi(node: ts.PropertyDeclaration): void {
    const expression = node.name;
    if (ts.isIdentifier(expression)) {
      this.processApiNodeDeprecatedApi(expression.text, expression);
    }
  }

  private processApiNodeDeprecatedApi(
    apiName: string,
    errorNode: ts.Node,
    deprecatedApiCheckMap?: Map<string, string | ts.NodeArray<ts.ParameterDeclaration>>,
    autofix?: Autofix[],
    apiType?: string
  ): void {
    const problemStr = this.getFaultIdWithMatchedDeprecatedApi(apiName, deprecatedApiCheckMap, apiType);
    if (problemStr.length > 0) {
      const isSdkCommon = apiType === SDK_COMMON_TYPE;
      const faultID = TypeScriptLinter.getFinalSdkFaultIdByProblem(problemStr, apiType);
      if (!faultID) {
        return;
      }
      this.incrementCounters(
        errorNode,
        faultID,
        isSdkCommon || apiType === BUILTIN_TYPE ? undefined : autofix,
        isSdkCommon || apiType === undefined ? TypeScriptLinter.getErrorMsgForSdkCommonApi(apiName, faultID) : undefined
      );
    }
  }

  private getFaultIdWithMatchedDeprecatedApi(
    apiName: string,
    deprecatedApiCheckMap?: Map<string, string | ts.NodeArray<ts.ParameterDeclaration>>,
    apiType?: string
  ): string {
    void this;
    if (!apiType) {
      apiType = DEPRECATE_TYPE;
    }
    const setApiListItem = apiType && TypeScriptLinter.getApiListItemSetFromAllWhiteList(apiType);
    if (!setApiListItem || !deprecatedApiCheckMap) {
      return '';
    }
    const apiNamesArr = [...setApiListItem];
    const isSpecial = apiType === SDK_COMMON_TYPE || apiType === BUILTIN_TYPE;
    let problem = '';
    apiNamesArr.some((apiInfoItem) => {
      if (!apiInfoItem.api_info.api_name && !apiName || BUILTIN_CONSTRUCTOR_API_NAME === apiName) {
        problem = TypeScriptLinter.getFaultIdWithMatchedBuiltinConstructApi(
          apiInfoItem,
          deprecatedApiCheckMap?.get(DEPRECATE_CHECK_KEY.PARENT_NAME) + ''
        );
        if (problem) {
          return true;
        }
      }
      const isSameApi = this.checkIsSameApiWithSdkList(apiInfoItem, apiName, deprecatedApiCheckMap, apiType);
      const fileName = deprecatedApiCheckMap?.get(DEPRECATE_CHECK_KEY.FILE_NAME) + '';
      const isSameFile = fileName.endsWith(path.basename(apiInfoItem.file_path));
      const res = isSameApi && isSameFile;
      if (res) {
        problem = isSpecial ? apiInfoItem.api_info.problem : DeprecateProblem.NoDeprecatedApi;
      }
      return res;
    });
    return problem;
  }

  private checkIsSameApiWithSdkList(
    apiInfoItem: ApiListItem,
    apiName: string,
    deprecatedApiCheckMap: Map<string, string | ts.NodeArray<ts.ParameterDeclaration>>,
    apiType?: string
  ): boolean {
    let isSameApi = apiInfoItem.api_info.api_name === apiName;
    isSameApi &&= TypeScriptLinter.checkParentNameUnderSdkList(
      apiInfoItem,
      deprecatedApiCheckMap.get(DEPRECATE_CHECK_KEY.PARENT_NAME) + '',
      apiType === SDK_COMMON_TYPE || apiType === BUILTIN_TYPE
    );
    const return_type = this.getReturnTypeByApiInfoItem(
      apiInfoItem,
      apiType === SDK_COMMON_TYPE || apiType === BUILTIN_TYPE
    );
    const actual_return_type = this.normalizeTypeString(deprecatedApiCheckMap.get(DEPRECATE_CHECK_KEY.RETURN_TYPE));
    isSameApi &&= return_type === actual_return_type;
    const api_func_args = apiInfoItem.api_info.api_func_args;
    const params = deprecatedApiCheckMap.get(DEPRECATE_CHECK_KEY.PARAM_SET);
    if (api_func_args && params) {
      const isParametersEqual = TypeScriptLinter.areParametersEqualForDeprecated(
        api_func_args,
        params as ts.NodeArray<ts.ParameterDeclaration>
      );
      isSameApi &&= isParametersEqual;
    }
    return isSameApi;
  }

  private static getFaultIdWithMatchedBuiltinConstructApi(apiInfoItem: ApiListItem, parentName: string): string {
    if (apiInfoItem.api_info.parent_api?.length <= 0) {
      return '';
    }
    const isBuiltinConstruct =
      BUILTIN_CONSTRUCTOR_API_TYPE.includes(apiInfoItem.api_info.api_type) &&
      apiInfoItem.api_info.parent_api[0].api_name === parentName;
    return isBuiltinConstruct ? apiInfoItem.api_info.problem : '';
  }

  private getReturnTypeByApiInfoItem(
    apiInfoItem: ApiListItem,
    isSpecial: boolean | undefined
  ): string | ts.NodeArray<ts.ParameterDeclaration> | undefined {
    let return_type = this.normalizeTypeString(apiInfoItem.api_info.method_return_type);
    if (isSpecial) {
      return_type = apiInfoItem.api_info.method_return_type ?
        this.normalizeTypeString(apiInfoItem.api_info.method_return_type) :
        this.normalizeTypeString(apiInfoItem.api_info.api_property_type);
    }
    return return_type;
  }

  private static checkParentNameUnderSdkList(
    apiInfoItem: ApiListItem,
    sourceParentName: string,
    isSpecial?: boolean
  ): boolean {
    const parentApis = apiInfoItem.api_info.parent_api;
    const possibleNames: string[] = [];
    const primaryParentName = parentApis[0]?.api_name || '';

    if (primaryParentName) {
      possibleNames.push(primaryParentName);
      if (!!isSpecial && parentApis.length > 1) {
        const secondaryParentName = parentApis[1]?.api_name || '';
        possibleNames.push(`${secondaryParentName}.${primaryParentName}`);
      }
    }
    return possibleNames.includes(sourceParentName) || parentApis.length === 0 && !sourceParentName;
  }

  private getPropertyTypeForPropertyAssignment(
    propertyAssignment: ts.PropertyAssignment,
    contextualType: ts.Type,
    isSpecial?: boolean
  ): Map<string, string | ts.NodeArray<ts.ParameterDeclaration>> | undefined {
    const propertyName = propertyAssignment.name.getText();
    if (contextualType.isUnion()) {
      for (const type of contextualType.types) {
        const deprecatedApiCheckMap = this.getPropertyInfoByContextualType(
          type,
          propertyName,
          propertyAssignment,
          isSpecial
        );
        if (deprecatedApiCheckMap) {
          return deprecatedApiCheckMap;
        }
      }
    }
    return this.getPropertyInfoByContextualType(contextualType, propertyName, propertyAssignment, isSpecial);
  }

  private getPropertyInfoByContextualType(
    type: ts.Type,
    propertyName: string,
    node: ts.Node,
    isSpecial?: boolean
  ): Map<string, string | ts.NodeArray<ts.ParameterDeclaration>> | undefined {
    const propertySymbol = type.getProperty(propertyName);
    if (!propertySymbol) {
      return undefined;
    }
    const propertyDecl = TsUtils.getDeclaration(propertySymbol);
    let deprecatedApiCheckMap = new Map<string, string | ts.NodeArray<ts.ParameterDeclaration>>();
    if (propertyDecl && ts.isPropertySignature(propertyDecl) && propertyDecl.type) {
      deprecatedApiCheckMap = TypeScriptLinter.updateDeprecatedApiCheckMap(
        type.getSymbol()?.name + '',
        undefined,
        propertyDecl.type.getText(),
        path.basename(propertyDecl.getSourceFile().fileName + '')
      );
      if (isSpecial) {
        this.hanldeSdkCommonTypeName(node, type.getSymbol(), type.getSymbol()?.name + '', propertyDecl);
      }
    }
    return deprecatedApiCheckMap;
  }

  private static updateDeprecatedApiCheckMap(
    parentName: string,
    parames: ts.NodeArray<ts.ParameterDeclaration> | undefined,
    returnType: string | undefined,
    fileName: string
  ): Map<string, string | ts.NodeArray<ts.ParameterDeclaration>> {
    const deprecatedApiCheckMap = new Map<string, string | ts.NodeArray<ts.ParameterDeclaration>>();
    deprecatedApiCheckMap.set(DEPRECATE_CHECK_KEY.PARENT_NAME, parentName);
    if (parames) {
      deprecatedApiCheckMap.set(DEPRECATE_CHECK_KEY.PARAM_SET, parames);
    }
    if (returnType) {
      deprecatedApiCheckMap.set(DEPRECATE_CHECK_KEY.RETURN_TYPE, returnType);
    }
    deprecatedApiCheckMap.set(DEPRECATE_CHECK_KEY.FILE_NAME, fileName);
    return deprecatedApiCheckMap;
  }

  private static getReturnTypeForEnumMember(node: ts.EnumMember): string {
    const enumDecl = node.parent;
    if (!enumDecl?.members || enumDecl.members.length === 0) {
      return '';
    }
    for (let i = 0; i < enumDecl.members.length; i++) {
      if (enumDecl.members[i].name.getText() === node.name.getText()) {
        return i + '';
      }
    }
    return '';
  }

  private normalizeTypeString(
    typeStr: string | ts.NodeArray<ts.ParameterDeclaration> | undefined
  ): string | ts.NodeArray<ts.ParameterDeclaration> | undefined {
    void this;
    if (typeof typeStr === 'string') {
      return typeStr.replace(/\s+/g, '');
    }
    return typeStr;
  }

  private static areParametersEqualForDeprecated(
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
      const newtypeName = typeName?.replace(/\s+/g, '');
      const sdkArgName = sdkFuncArgs[i].type.replace(/\s+/g, '');
      if (newtypeName !== sdkArgName) {
        return false;
      }
    }
    return true;
  }

  private handleESObjectUsage(typeRef: ts.TypeReferenceNode): void {
    if (!this.options.arkts2) {
      return;
    }

    if (
      ts.isIdentifier(typeRef.typeName) && typeRef.typeName.text === ES_OBJECT ||
      ts.isQualifiedName(typeRef.typeName) && typeRef.typeName.right.text === ES_OBJECT
    ) {
      this.incrementCounters(typeRef, FaultID.NoESObjectSupport);
    }
  }

  private handleNodeForBuilderNode(node: ts.TypeReferenceNode | ts.NewExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    const identifier = ts.isTypeReferenceNode(node) ? node.typeName : node.expression;
    if (
      identifier.getText() !== CustomInterfaceName.BuilderNode ||
      !this.isDeclInTargetFile(identifier, BUILDERNODE_D_TS)
    ) {
      return;
    }

    const firstArg = node.typeArguments?.[0];
    if (firstArg && ts.isTupleTypeNode(firstArg)) {
      this.incrementCounters(node, FaultID.BuilderNodeGenericNoTuple);
    }
  }

  private isDeclInTargetFile(node: ts.Node, targetFile: string): boolean {
    const decl = this.tsUtils.getDeclarationNode(node);
    const file = decl?.getSourceFile();
    if (file !== undefined) {
      const fileName = path.basename(file.fileName);
      if (fileName !== targetFile) {
        return false;
      }
    }

    return true;
  }

  private handlePropertyAccessExprForBuilderNode(node: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    const identifier = node.name;
    if (!ts.isIdentifier(identifier) || !this.isDeclInTargetFile(identifier, BUILDERNODE_D_TS)) {
      return;
    }

    const name = identifier.getText();
    const callExpr = ts.findAncestor(node, ts.isCallExpression);
    if (!callExpr) {
      return;
    }

    switch (name) {
      case BuilderNodeFunctionName.Update:
        if (callExpr.arguments.length !== 0 && this.checkArgumentIsLiteral(callExpr.arguments[0])) {
          this.incrementCounters(callExpr, FaultID.BuilderNodeUpdateNoLiteral);
        }
        break;
      case BuilderNodeFunctionName.Build: {
        const hasTargetParam = callExpr.arguments.filter(ts.isObjectLiteralExpression).some((literal) => {
          return literal.properties.some((prop) => {
            return prop.name?.getText() === NESTING_BUILDER_SUPPORTED;
          });
        });
        if (hasTargetParam) {
          this.incrementCounters(callExpr, FaultID.BuilderNodeNoNestingBuilderSupported);
        }
        break;
      }
      default:
    }
  }

  private checkArgumentIsLiteral(node: ts.Node): boolean {
    switch (node.kind) {
      case ts.SyntaxKind.ObjectLiteralExpression:
        return true;
      case ts.SyntaxKind.Identifier:
        return this.checkIdentifierIsLiteral(node as ts.Identifier);
      case ts.SyntaxKind.ConditionalExpression:
        return this.checkConditionalExprIsLiteral(node as ts.ConditionalExpression);
      case ts.SyntaxKind.CallExpression:
        return this.checkCallExprIsLiteral(node as ts.CallExpression);
      default:
        return false;
    }
  }

  private checkIdentifierIsLiteral(node: ts.Identifier): boolean {
    const decl = this.tsUtils.getDeclarationNode(node);
    const initalizer = decl && ts.isVariableDeclaration(decl) ? decl.initializer : undefined;
    if (!initalizer) {
      return false;
    }
    return this.checkArgumentIsLiteral(initalizer);
  }

  private checkConditionalExprIsLiteral(node: ts.ConditionalExpression): boolean {
    return this.checkArgumentIsLiteral(node.whenTrue) || this.checkArgumentIsLiteral(node.whenFalse);
  }

  private checkCallExprIsLiteral(node: ts.CallExpression): boolean {
    const callExpr = node;
    let identifier: ts.Identifier | undefined;
    if (ts.isIdentifier(callExpr.expression)) {
      identifier = callExpr.expression;
    } else if (ts.isPropertyAccessExpression(callExpr.expression) && ts.isIdentifier(callExpr.expression.name)) {
      identifier = callExpr.expression.name;
    }

    if (!identifier) {
      return false;
    }
    const funcDecl = this.tsUtils.getDeclarationNode(identifier);
    const body =
      funcDecl && (ts.isFunctionDeclaration(funcDecl) || ts.isMethodDeclaration(funcDecl)) ? funcDecl.body : undefined;
    if (!body) {
      return false;
    }
    for (const stmt of body.statements) {
      if (ts.isReturnStatement(stmt) && stmt.expression) {
        return this.checkArgumentIsLiteral(stmt.expression);
      }
    }
    return false;
  }

  private handlePromiseTupleGeneric(node: ts.CallExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    if (
      ts.isPropertyAccessExpression(node.expression) &&
      ts.isIdentifier(node.expression.expression) &&
      node.expression.expression.text === PROMISE
    ) {
      const methodName = node.expression.name.text;

      if (!PROMISE_METHODS_WITH_NO_TUPLE_SUPPORT.has(methodName)) {
        return;
      }

      const typeArguments = node.typeArguments;
      if (!typeArguments || typeArguments.length === 0) {
        return;
      }

      const firstArg = typeArguments[0];
      if (ts.isTupleTypeNode(firstArg)) {
        this.incrementCounters(firstArg, FaultID.NotSupportTupleGenericValidation);
      }
    }
  }

  private static getParameterDeclarationsBySignature(signature: ts.Signature): ts.NodeArray<ts.ParameterDeclaration> {
    const validParameters = signature.parameters.
      map((paramSymbol) => {
        const declarations = paramSymbol.getDeclarations();
        const paramDeclaration = declarations?.[0];
        return paramDeclaration && ts.isParameter(paramDeclaration) ? paramDeclaration : undefined;
      }).
      filter((param): param is ts.ParameterDeclaration => {
        return param !== undefined;
      });
    return ts.factory.createNodeArray(validParameters);
  }

  private static isImportedFromOhos(importName: string, filePath: string): boolean {
    const classPaths = TypeScriptLinter.sdkCommonIndexClassSet.get(importName);
    return (
      !!classPaths &&
      classPaths.some((p) => {
        return path.basename(p) === filePath;
      })
    );
  }

  private static mergeSdkCommonApiListInfo(): Set<ApiListItem> {
    return new Set([
      ...TypeScriptLinter.sdkCommonApiInfo,
      ...TypeScriptLinter.sdkCommonSymbotIterSet,
      ...TypeScriptLinter.sdkCommonAllDeprecatedTypeNameSet
    ]);
  }

  private static getFaultIdSdkApiInfoWithConstructorDecl(extendClassName: string, apiType: string): string {
    const mergedSet = TypeScriptLinter.getApiListItemSetFromAllWhiteList(
      apiType,
      apiType === SDK_COMMON_TYPE ? true : undefined
    );
    if (!mergedSet) {
      return '';
    }
    let api_types = [''];
    if (apiType === SDK_COMMON_TYPE) {
      api_types = SDK_COMMON_CONSTRUCTORLIKE;
    } else if (apiType === BUILTIN_TYPE) {
      api_types = BUILTIN_CONSTRUCTOR_API_TYPE;
    }
    let problem = '';
    for (const item of mergedSet) {
      if (item.api_info.parent_api?.length <= 0) {
        continue;
      }
      const isCompare =
        item.api_info.parent_api[0].api_name === extendClassName && api_types.includes(item.api_info.api_type);
      if (isCompare) {
        problem = item.api_info.problem;
        break;
      }
    }
    return problem;
  }

  private static getFaultIdSdkApiInfoWithClassMember(
    decl: ts.ClassDeclaration | ts.InterfaceDeclaration,
    targetName: string,
    typeKey: string,
    mergedSet: Set<ApiListItem>
  ): string {
    const extendClassName = decl.name?.text;
    const fileName = path.basename(decl.getSourceFile().fileName);
    if (!extendClassName || !fileName) {
      return '';
    }
    const memberLike = typeKey === SDK_COMMON_TYPEKEY[0] ? SDK_COMMON_FUNCTIONLIKE : SDK_COMMON_PROPERTYLIKE;
    let problem = '';
    const apiFilePath = this.getApiFilePathsFromSdkList(mergedSet);
    for (const item of mergedSet) {
      if (item.api_info.parent_api?.length <= 0) {
        continue;
      }
      const isFunLikeCompare =
        item.api_info.parent_api[0].api_name === extendClassName &&
        memberLike.includes(item.api_info.api_type) &&
        item.api_info.api_name === targetName &&
        apiFilePath.includes(fileName);
      if (isFunLikeCompare) {
        problem = item.api_info.problem;
        break;
      }
    }
    return problem;
  }

  private static getApiFilePathsFromSdkList(mergedSet: Set<ApiListItem>): string[] {
    const apiFilePath: string[] = [];
    mergedSet.forEach((mem) => {
      apiFilePath.push(path.basename(mem.file_path));
    });
    return apiFilePath;
  }

  private static refactorSetWhitSameAsParenName(targetName: string, apiType: string): Set<ApiListItem> | undefined {
    const mergedSet = TypeScriptLinter.getApiListItemSetFromAllWhiteList(
      apiType,
      apiType === SDK_COMMON_TYPE ? true : undefined
    );
    if (!mergedSet) {
      return undefined;
    }
    const newMergedSet = new Set<ApiListItem>();
    for (const item of mergedSet) {
      if (item.api_info.parent_api?.length <= 0) {
        continue;
      }
      if (item.api_info.parent_api[0].api_name === targetName) {
        newMergedSet.add(item);
      }
    }
    return newMergedSet;
  }

  private static getApiListItemSetFromAllWhiteList(
    type: string,
    isMergeSdkCommonApi?: boolean
  ): Set<ApiListItem> | undefined {
    if (type === DEPRECATE_TYPE) {
      return TypeScriptLinter.deprecatedApiInfo;
    } else if (type === SDK_COMMON_TYPE) {
      return isMergeSdkCommonApi ? TypeScriptLinter.mergeSdkCommonApiListInfo() : TypeScriptLinter.sdkCommonApiInfo;
    } else if (type === BUILTIN_TYPE) {
      return TypeScriptLinter.builtApiInfo;
    }
    return undefined;
  }

  private checkArrayInitialization(tsNewExpr: ts.NewExpression): void {
    if (!this.options.arkts2) {
      return;
    }
    if (!tsNewExpr.arguments || tsNewExpr.arguments.length !== 1) {
      return;
    }
    const newExprType = this.tsTypeChecker.getTypeAtLocation(tsNewExpr);
    const argType = this.tsTypeChecker.getTypeAtLocation(tsNewExpr.arguments[0]);
    if (this.tsUtils.isGenericArrayType(newExprType) && this.tsUtils.isNumberLikeType(argType)) {
      this.incrementCounters(tsNewExpr, FaultID.UninitializedArrayElements);
    }
  }

  private handleBuiltinIteratorResult(propAccessExpr: ts.PropertyAccessExpression): void {
    if (!this.options.arkts2 || !TypeScriptLinter.builtApiInfo || propAccessExpr.name.getText() !== 'value') {
      return;
    }

    const type = this.tsTypeChecker.getTypeAtLocation(propAccessExpr.expression);
    const aliasSymbol = type.aliasSymbol;
    const declaration = aliasSymbol?.declarations?.[0];
    if (!declaration || !ts.isTypeAliasDeclaration(declaration)) {
      return;
    }

    const name = declaration.name.getText();
    const typeStr = declaration.type.getText();
    const fileName = declaration.getSourceFile().fileName;
    this.processApiNodeDeprecatedApi(
      name,
      propAccessExpr,
      TypeScriptLinter.updateDeprecatedApiCheckMap('', undefined, typeStr, path.basename(fileName)),
      undefined,
      BUILTIN_TYPE
    );
  }

  private handleBuiltinDisableDecorator(decorator: ts.Decorator): void {
    if (!this.options.arkts2 || !TypeScriptLinter.builtApiInfo) {
      return;
    }
    const type = this.tsTypeChecker.getTypeAtLocation(decorator.expression);
    const aliasSymbol = type.aliasSymbol;
    const declaration = aliasSymbol?.declarations?.[0];
    if (!declaration || !ts.isTypeAliasDeclaration(declaration) || !ts.isFunctionLike(declaration.type)) {
      return;
    }
    const name = declaration.name.getText();
    const params = declaration.type.parameters;
    const typeStr = declaration.type.type?.getText();
    const fileName = declaration.getSourceFile().fileName;
    this.processApiNodeDeprecatedApi(
      name,
      decorator,
      TypeScriptLinter.updateDeprecatedApiCheckMap('', params, typeStr, path.basename(fileName)),
      undefined,
      BUILTIN_TYPE
    );
  }

  private handleUnsignedShiftOnNegative(node: ts.BinaryExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    if (!TypeScriptLinter.isUnsignedShiftByZero(node)) {
      return;
    }

    if (TsUtils.isNegativeNumericLiteral(node.left)) {
      this.incrementCounters(node, FaultID.NumericUnsignedShiftBehaviorChange);
    }

    if (ts.isIdentifier(node.left)) {
      const symbol = this.tsTypeChecker.getSymbolAtLocation(node.left);
      const decl = symbol?.valueDeclaration;
      if (!decl || !ts.isVariableDeclaration(decl)) {
        return;
      }

      const init = decl.initializer;
      if (init && TsUtils.isNegativeNumericLiteral(init)) {
        this.incrementCounters(node, FaultID.NumericUnsignedShiftBehaviorChange);
      }
    }
  }

  private static isUnsignedShiftByZero(node: ts.BinaryExpression): boolean {
    return (
      node.operatorToken.kind === ts.SyntaxKind.GreaterThanGreaterThanGreaterThanToken &&
      ts.isNumericLiteral(node.right) &&
      node.right.text === '0'
    );
  }

  private handleCallExpressionForSerialization(node: ts.CallExpression): void {
    if (!this.options.arkts2) {
      return;
    }

    const propertyAccess = node.expression;
    if (!ts.isPropertyAccessExpression(propertyAccess)) {
      return;
    }

    const persistentClass = propertyAccess.expression;
    if (!ts.isIdentifier(persistentClass)) {
      return;
    }

    switch (persistentClass.getText()) {
      case StorageTypeName.PersistentStorage:
        if (this.isDeclInTargetFile(persistentClass, COMMON_TS_ETS_API_D_TS)) {
          this.handleCallExpressionForPersistentStorage(node, propertyAccess);
        }
        break;
      case StorageTypeName.PersistenceV2:
        if (this.isDeclInTargetFile(persistentClass, UI_STATE_MANAGEMENT_D_TS)) {
          this.handleCallExpressionForPersistenceV2(node, propertyAccess);
        }
        break;
      default:
    }
  }

  private handleCallExpressionForPersistentStorage(
    callExpr: ts.CallExpression,
    propertyAccess: ts.PropertyAccessExpression
  ): void {
    const funcName = propertyAccess.name.getText();

    switch (funcName) {
      case PERSIST_PROP_FUNC_NAME:
        if (!this.checkPersistPropForSerialization(callExpr)) {
          this.incrementCounters(callExpr, FaultID.PersistentPropNeedImplementMethod);
        }
        break;
      case PERSIST_PROPS_FUNC_NAME:
        if (!this.checkPersistPropsForSerialization(callExpr)) {
          this.incrementCounters(callExpr, FaultID.PersistentPropsNeedImplementMethod);
        }
        break;
      default:
    }
  }

  private checkPersistPropForSerialization(callExpr: ts.CallExpression): boolean {
    const arg = callExpr.arguments?.[1];
    return !arg || this.checkArgumentForSerialization(arg);
  }

  private checkPersistPropsForSerialization(callExpr: ts.CallExpression): boolean {
    const arg = callExpr.arguments?.[0];
    if (!arg || !ts.isArrayLiteralExpression(arg)) {
      return true;
    }

    const literals = arg.elements;
    let serializable: boolean = true;
    for (const literal of literals) {
      if (!ts.isObjectLiteralExpression(literal)) {
        continue;
      }
      const property = literal.properties?.[1];
      if (!property || !ts.isPropertyAssignment(property)) {
        continue;
      }
      if (!this.checkArgumentForSerialization(property.initializer)) {
        serializable = false;
        break;
      }
    }

    return serializable;
  }

  private checkArgumentForSerialization(arg: ts.Node): boolean {
    const type = this.tsTypeChecker.getTypeAtLocation(arg);

    if (type.isUnion()) {
      if (
        type.types.some((type) => {
          return !this.isSpecificTypeOfSerialization(type);
        })
      ) {
        return false;
      }
      return true;
    }

    return this.isSpecificTypeOfSerialization(type);
  }

  private isSpecificTypeOfSerialization(type: ts.Type): boolean {
    const typeName = this.tsTypeChecker.typeToString(type);
    return serializationTypeFlags.has(type.flags) || serializationTypeName.has(typeName);
  }

  private handleCallExpressionForPersistenceV2(
    callExpr: ts.CallExpression,
    propertyAccess: ts.PropertyAccessExpression
  ): void {
    const funcName = propertyAccess.name.getText();
    if (funcName !== GLOBAL_CONNECT_FUNC_NAME && funcName !== CONNECT_FUNC_NAME) {
      return;
    }

    const errorMsg =
      `When calling the "${funcName}" method, the parameter list of the methods needs to include ` +
      '"toJson" and "fromJson" (arkui-persistencev2-connect-serialization)';
    this.incrementCounters(callExpr, FaultID.PersistenceV2ConnectNeedAddParam, undefined, errorMsg);
  }
}
