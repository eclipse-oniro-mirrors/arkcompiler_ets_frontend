/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import {
    AbstractFieldRef,
    AbstractInvokeExpr,
    ArkAssignStmt,
    MethodSignature,
    Stmt,
    Type,
    Value,
} from 'arkanalyzer/lib';
import { Sdk } from 'arkanalyzer/lib/Config';

export const INT32_BOUNDARY: number = 0x80000000;
export const PROMISE_CLASS_NAME: string = 'Promise';
export const THEN_METHOD_NAME: string = 'then';
export const LENGTH_FIELD_NAME: string = 'length';
export const DEFAULT_BUILTIN_TARGET_ES_VERSION: string = 'ES2017';
export const BUILD_PROFILE_JSON5: string = 'build-profile.json5';
export const BUILTIN_DYN_DECL_PROJECT_NAME: string = 'builtin-dyn-declaration';
export const BUILTIN_STA_DECL_PROJECT_NAME: string = 'builtin-sta-declaration';
export const BUILTIN_CONSTRUCT_SIGNATURE_METHOD_NAME: string = 'construct-signature';
export const INTERNAL_SDK_PROJECT_NAME: string = 'internalSdk';
export const INTERNAL_BUILTIN_DECLARATION_PREFIX: string = '@internal/lib.';
export const BUILTIN_ES_VERSION_ENTRY_FILES: Map<string, string> = new Map<string, string>([
    ['ES2017', 'lib.es2017.d.ts'],
    ['ES2018', 'lib.es2018.d.ts'],
    ['ES2019', 'lib.es2019.d.ts'],
    ['ES2020', 'lib.es2020.d.ts'],
    ['ES2021', 'lib.es2021.d.ts'],
]);

export interface SignatureMatchOptions {
    allowTrailingOptionalParams?: boolean;
    allowLooseSourceTypes?: boolean;
    allowArrayLikeTypes?: boolean;
    allowWellKnownSymbolRegExpProtocolTypes?: boolean;
}

export const BUILTIN_DECLARATION_SIGNATURE_MATCH_OPTIONS: SignatureMatchOptions = {
    allowTrailingOptionalParams: true,
    allowLooseSourceTypes: true,
    allowArrayLikeTypes: true,
    allowWellKnownSymbolRegExpProtocolTypes: true,
};

export enum NumberCategory {
    int = 'int',
    long = 'long',
    number = 'number',
}

export enum RuleCategory {
    SDKIntType = 'sdk-api-num2int',
    BuiltinIntType = 'arkts-builtin-api-num2int',
    NumericLiteral = 'arkts-numeric-semantic',
    ArrayIndex = 'arkts-array-index-expr-type',
}

export enum IssueReason {
    OnlyUsedAsIntLong = 'only-used-as-int-or-long',
    UsedWithOtherType = 'not-only-used-as-int-or-long',
    CannotFindAll = 'cannot-find-all',
    RelatedWithNonETS2 = 'related-with-non-ets2',
    ActuallyIntConstant = 'actually-int-constant',
    AmbiguousIntLong = 'ambiguous-int-long',
    Other = 'other',
}

export interface IssueInfo {
    issueReason: IssueReason;
    numberCategory: NumberCategory;
}

export interface ChangedArgCategories {
    ruleCategory: RuleCategory;
    args: Map<Value, NumberCategory> | null;
}

export interface ChangedFunctionReturnCategories {
    ruleCategory: RuleCategory;
    callbacks: Map<Value, NumberCategory> | null;
}

export interface ChangedFunctionParamCategory {
    callback: Value;
    paramIndex: number;
    category: NumberCategory;
}

export interface ChangedFunctionParamCategories {
    ruleCategory: RuleCategory;
    params: ChangedFunctionParamCategory[] | null;
}

export interface ChangedResultCategory {
    ruleCategory: RuleCategory;
    category: NumberCategory | null;
    requireNumberLikeLeft?: boolean;
}

export interface ApiNumberChangeProvider {
    getChangedArgCategories(invokeExpr: AbstractInvokeExpr): ChangedArgCategories;
    getChangedFunctionParamCategories?(invokeExpr: AbstractInvokeExpr): ChangedFunctionParamCategories;
    getChangedFunctionReturnCategories?(invokeExpr: AbstractInvokeExpr): ChangedFunctionReturnCategories;
    beforeArgCheck?(stmt: Stmt, invokeExpr: AbstractInvokeExpr): void;
    getChangedReturnCategory(stmt: ArkAssignStmt, rightInvokeExpr: AbstractInvokeExpr | null): ChangedResultCategory;
    getChangedReturnedValueCategory?(value: Value): ChangedResultCategory;
    getChangedFieldCategory(fieldRef: AbstractFieldRef): ChangedResultCategory;
}

export type BuiltinNumberChangePathRoot = 'arg' | 'return';

export type BuiltinNumberChangePathStep =
    { kind: 'functionParam'; index: number } |
    { kind: 'functionReturn' } |
    { kind: 'generic'; index: number } |
    { kind: 'tuple'; index: number } |
    { kind: 'arrayElement' } |
    { kind: 'union'; index: number };

export interface BuiltinNumberChangePath {
    root: BuiltinNumberChangePathRoot;
    argIndex?: number;
    steps: BuiltinNumberChangePathStep[];
}

export interface BuiltinNumberChange {
    path: BuiltinNumberChangePath;
    category: NumberCategory;
}

export interface BuiltinApiRule {
    className: string | string[];
    methodName: string;
    args?: Record<number, NumberCategory | string>;
    returnType?: NumberCategory | string;
    paramCount?: number;
    hasRest?: boolean;
    signature: MethodSignature;
    changes?: BuiltinNumberChange[];
}

export interface BuiltinFieldRule {
    className: string | string[];
    fieldName: string;
    type: NumberCategory | string;
}

export interface BuiltinDeclarationRules {
    apiRules: BuiltinApiRule[];
    fieldRules: BuiltinFieldRule[];
}

export interface BuiltinSignatureMethod {
    className: string;
    methodName: string;
    signature: MethodSignature;
}

export interface BuiltinSignatureField {
    className: string;
    fieldName: string;
    type: Type;
}

export interface BuiltinSignatureInfo {
    methods: Map<string, BuiltinSignatureMethod[]>;
    fields: Map<string, BuiltinSignatureField[]>;
}

export interface BuiltinNewArrayArgInfo {
    stmt: Stmt;
    value: Value;
}

export interface RuleOptions {
    ets2Sdks?: Sdk[];
    disableDefaultBuiltinApis?: boolean;
    targetESVersion?: string;
}
