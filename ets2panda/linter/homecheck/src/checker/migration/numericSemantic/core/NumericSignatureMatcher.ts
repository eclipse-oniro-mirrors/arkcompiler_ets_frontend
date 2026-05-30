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
    AnyType,
    ArrayType,
    BigIntType,
    BooleanType,
    ClassSignature,
    classSignatureCompare,
    ClassType,
    FunctionType,
    GenericType,
    MethodSignature,
    NumberType,
    StringType,
    Type,
    UnclearReferenceType,
    UnknownType,
    UnionType,
    VoidType,
} from 'arkanalyzer/lib';
import type { MethodParameter } from 'arkanalyzer/lib/core/model/builder/ArkMethodBuilder';
import { SdkUtils } from '../../../../utils/common/SDKUtils';
import {
    BUILTIN_DYN_DECL_PROJECT_NAME,
    BUILTIN_STA_DECL_PROJECT_NAME,
    INTERNAL_SDK_PROJECT_NAME,
    NumberCategory,
    SignatureMatchOptions,
} from './NumericSemanticTypes';

const WELL_KNOWN_SYMBOL_DECLARATION_FILE: string = 'lib.es2015.symbol.wellknown.d.ts';
const REGEXP_TYPE_NAME: string = 'RegExp';

interface NumericSignatureMatcherOptions {
    isIntType(type: Type): boolean;
    isLongType(type: Type): boolean;
}

export class NumericSignatureMatcher {
    constructor(private options: NumericSignatureMatcherOptions) {}

    public matchEts1NumberEts2IntLongReturnSig(ets2Sigs: MethodSignature[], ets1Sig: MethodSignature): MethodSignature | null {
        for (const ets2Sig of ets2Sigs) {
            if (!this.isEts1NumberEts2IntLongSignatureMatched(ets1Sig, ets2Sig)) {
                continue;
            }
            const returnType = ets2Sig.getType();
            if (this.options.isLongType(returnType) || this.options.isIntType(returnType)) {
                return ets2Sig;
            }
        }
        return null;
    }

    public matchEts1NumberEts2IntLongMethodSig(ets2Sigs: MethodSignature[], ets1Sig: MethodSignature): MethodSignature | null {
        let intSDKMatched: MethodSignature | null = null;
        for (const ets2Sig of ets2Sigs) {
            if (!this.isEts1NumberEts2IntLongSignatureMatched(ets1Sig, ets2Sig)) {
                continue;
            }
            const changedCategories = this.getEts1NumberEts2IntLongChangedParamCategories(ets1Sig, ets2Sig);
            if (changedCategories.some(category => category === NumberCategory.long)) {
                return ets2Sig;
            }
            if (changedCategories.some(category => category === NumberCategory.int)) {
                intSDKMatched = ets2Sig;
            }
        }
        return intSDKMatched;
    }

    public isEts1NumberEts2IntLongSignatureMatched(ets1Sig: MethodSignature, ets2Sig: MethodSignature, options: SignatureMatchOptions = {}): boolean {
        const ets1Params = ets1Sig.getMethodSubSignature().getParameters();
        const ets2Params = ets2Sig.getMethodSubSignature().getParameters();
        const countMatched = options.allowTrailingOptionalParams ?
            this.isSignatureParameterCountMatched(ets1Params, ets2Params) :
            ets2Params.length === ets1Params.length;
        if (!countMatched) {
            return false;
        }
        const comparableLength = Math.min(ets1Params.length, ets2Params.length);
        for (let i = 0; i < comparableLength; i++) {
            if (!this.isEts1NumberEts2IntLongParamMatched(ets1Params[i].getType(), ets2Params[i].getType(), options)) {
                return false;
            }
        }
        return true;
    }

    public getIntLongCategoryFromType(type: Type): NumberCategory.int | NumberCategory.long | null {
        if (this.options.isIntType(type)) {
            return NumberCategory.int;
        }
        if (this.options.isLongType(type)) {
            return NumberCategory.long;
        }
        return null;
    }

    private compareTypes(param1: Type, param2: Type, options: SignatureMatchOptions = {}): boolean {
        if (param1 === param2) {
            return true;
        }
        if (options.allowLooseSourceTypes && this.isLooseDeclarationSourceType(param1)) {
            return true;
        }
        if (param1 instanceof FunctionType && param2 instanceof FunctionType) {
            return this.compareFunctionTypes(param1, param2, options);
        }
        if (this.compareUnionTypes(param1, param2, options)) {
            return true;
        }
        if (options.allowArrayLikeTypes && this.compareArrayLikeTypes(param1, param2, options)) {
            return true;
        }
        if (this.compareWellKnownSymbolRegExpProtocolTypes(param1, param2, options)) {
            return true;
        }
        if (param1 instanceof GenericType && param2 instanceof GenericType) {
            return this.compareGenericTypes(param1, param2, options);
        }
        if (this.compareGenericReferenceTypes(param1, param2)) {
            return true;
        }
        if (this.compareNamedTypes(param1, param2)) {
            return true;
        }
        if (param1 instanceof ClassType && param2 instanceof ClassType) {
            const classSign1 = param1.getClassSignature();
            const classSign2 = param2.getClassSignature();
            if (
                (SdkUtils.isClassFromSdk(classSign1) && SdkUtils.isClassFromSdk(classSign2)) ||
                (this.isBuiltinDeclarationClassSignature(classSign1) && this.isBuiltinDeclarationClassSignature(classSign2))
            ) {
                return classSign1.getClassName() === classSign2.getClassName();
            }
            return classSignatureCompare(classSign1, classSign2);
        }
        return param1.constructor === param2.constructor && param1.toString() === param2.toString();
    }

    private compareFunctionTypes(param1: FunctionType, param2: FunctionType, options: SignatureMatchOptions = {}): boolean {
        const sig1 = param1.getMethodSignature();
        const sig2 = param2.getMethodSignature();
        return this.isFunctionParameterTypesMatched(sig1, sig2, options) && this.isFunctionReturnTypeMatched(sig1.getType(), sig2.getType(), options);
    }

    private isFunctionParameterTypesMatched(ets1Sig: MethodSignature, ets2Sig: MethodSignature, options: SignatureMatchOptions): boolean {
        const ets1Params = this.getComparableFunctionParameters(ets1Sig);
        const ets2Params = this.getComparableFunctionParameters(ets2Sig);
        const countMatched = options.allowTrailingOptionalParams ?
            this.isSignatureParameterCountMatched(ets1Params, ets2Params) :
            ets2Params.length === ets1Params.length;
        if (!countMatched) {
            return false;
        }
        const comparableLength = Math.min(ets1Params.length, ets2Params.length);
        for (let i = 0; i < comparableLength; i++) {
            if (!this.isEts1NumberEts2IntLongParamMatched(ets1Params[i].getType(), ets2Params[i].getType(), options)) {
                return false;
            }
        }
        return true;
    }

    private getComparableFunctionParameters(signature: MethodSignature): MethodParameter[] {
        const params = signature.getMethodSubSignature().getParameters();
        if (params.length > 0 && this.isThisVoidParameter(params[0])) {
            return params.slice(1);
        }
        return params;
    }

    private isThisVoidParameter(param: MethodParameter): boolean {
        return param.getName() === 'this' && param.getType() instanceof VoidType;
    }

    private isFunctionReturnTypeMatched(ets1Type: Type, ets2Type: Type, options: SignatureMatchOptions = {}): boolean {
        return this.isEts1NumberEts2IntLongParamMatched(ets1Type, ets2Type, options) ||
            (options.allowLooseSourceTypes === true && this.isLooseDeclarationSourceType(ets1Type));
    }

    private isLooseDeclarationSourceType(type: Type): boolean {
        if (type instanceof AnyType || type instanceof UnknownType) {
            return true;
        }
        if (type instanceof UnclearReferenceType) {
            const name = type.getName();
            return name === 'any' ||
                name === 'unknown' ||
                name === 'AnyKeyword' ||
                name === 'UnknownKeyword';
        }
        return false;
    }

    private compareArrayLikeTypes(param1: Type, param2: Type, options: SignatureMatchOptions): boolean {
        const arrayLikeType1 = this.getArrayLikeTypeInfo(param1);
        const arrayLikeType2 = this.getArrayLikeTypeInfo(param2);
        if (!arrayLikeType1 || !arrayLikeType2) {
            return false;
        }
        if (!arrayLikeType1.elementType || !arrayLikeType2.elementType) {
            return true;
        }
        if (this.isRawArrayElementWildcard(arrayLikeType1.elementType) || this.isRawArrayElementWildcard(arrayLikeType2.elementType)) {
            return true;
        }
        return this.isEts1NumberEts2IntLongParamMatched(arrayLikeType1.elementType, arrayLikeType2.elementType, options);
    }

    private isRawArrayElementWildcard(type: Type): boolean {
        return type instanceof AnyType;
    }

    private getArrayLikeTypeInfo(type: Type): { elementType: Type | null } | null {
        if (type instanceof ArrayType) {
            return { elementType: type.getBaseType() };
        }
        if (type instanceof ClassType && this.isArrayLikeClassName(type.getClassSignature().getClassName())) {
            return { elementType: type.getRealGenericTypes()?.[0] ?? null };
        }
        return null;
    }

    private isArrayLikeClassName(className: string): boolean {
        return className === 'Array' || className === 'ReadonlyArray';
    }

    private isBuiltinDeclarationClassSignature(classSignature: ClassSignature): boolean {
        const projectName = classSignature.getDeclaringFileSignature().getProjectName();
        return projectName === BUILTIN_DYN_DECL_PROJECT_NAME || projectName === BUILTIN_STA_DECL_PROJECT_NAME;
    }

    private isSignatureParameterCountMatched(
        ets1Params: { isOptional(): boolean; hasDotDotDotToken(): boolean }[],
        ets2Params: { isOptional(): boolean; hasDotDotDotToken(): boolean }[]
    ): boolean {
        if (ets1Params.length === ets2Params.length) {
            return true;
        }
        if (ets1Params.length > ets2Params.length) {
            return this.areTrailingParametersOptional(ets1Params, ets2Params.length);
        }
        return this.areTrailingParametersOptional(ets2Params, ets1Params.length);
    }

    private areTrailingParametersOptional(params: { isOptional(): boolean; hasDotDotDotToken(): boolean }[], startIndex: number): boolean {
        return params.slice(startIndex).every(param => param.isOptional() || param.hasDotDotDotToken());
    }

    private isEts1NumberEts2IntLongParamMatched(ets1Type: Type, ets2Type: Type, options: SignatureMatchOptions = {}): boolean {
        return this.compareTypes(ets1Type, ets2Type, options) || this.getEts1NumberEts2IntLongChangedCategory(ets1Type, ets2Type) !== null;
    }

    private compareUnionTypes(param1: Type, param2: Type, options: SignatureMatchOptions): boolean {
        if (param1 instanceof UnionType && param2 instanceof UnionType) {
            return this.compareUnionTypeMembers(param1, param2, options);
        }

        if (param1 instanceof UnionType) {
            return param1.getTypes().some(type => this.compareTypes(type, param2, options));
        }

        if (param2 instanceof UnionType) {
            return param2.getTypes().some(type => this.compareTypes(param1, type, options));
        }

        return false;
    }

    private compareUnionTypeMembers(param1: UnionType, param2: UnionType, options: SignatureMatchOptions): boolean {
        const remainingTypes = [...param2.getTypes()];
        for (const type1 of param1.getTypes()) {
            const matchedIndex = remainingTypes.findIndex(type2 => this.compareTypes(type1, type2, options));
            if (matchedIndex < 0) {
                return false;
            }
            remainingTypes.splice(matchedIndex, 1);
        }
        return remainingTypes.length === 0;
    }

    private compareNamedTypes(param1: Type, param2: Type): boolean {
        const typeName1 = this.getComparableTypeName(param1);
        const typeName2 = this.getComparableTypeName(param2);
        return typeName1 !== null && typeName2 !== null && typeName1 === typeName2;
    }

    private compareGenericTypes(param1: GenericType, param2: GenericType, options: SignatureMatchOptions): boolean {
        const defaultType1 = param1.getDefaultType();
        const defaultType2 = param2.getDefaultType();
        if (defaultType1 && defaultType2) {
            return this.isEts1NumberEts2IntLongParamMatched(defaultType1, defaultType2, options);
        }
        return true;
    }

    private compareGenericReferenceTypes(param1: Type, param2: Type): boolean {
        return (param1 instanceof GenericType && this.isGenericTypeParameterReference(param2)) ||
            (param2 instanceof GenericType && this.isGenericTypeParameterReference(param1));
    }

    private isGenericTypeParameterReference(type: Type): boolean {
        return type instanceof UnclearReferenceType &&
            type.getGenericTypes().length === 0 &&
            /^[A-Z]$/u.test(type.getName());
    }

    private compareWellKnownSymbolRegExpProtocolTypes(param1: Type, param2: Type, options: SignatureMatchOptions): boolean {
        if (!options.allowWellKnownSymbolRegExpProtocolTypes) {
            return false;
        }
        return (this.isWellKnownSymbolProtocolObjectType(param1) && this.isRegExpType(param2)) ||
            (this.isWellKnownSymbolProtocolObjectType(param2) && this.isRegExpType(param1));
    }

    private isWellKnownSymbolProtocolObjectType(type: Type): boolean {
        if (!(type instanceof ClassType)) {
            return false;
        }
        const signature = type.getClassSignature();
        const fileSignature = signature.getDeclaringFileSignature();
        const projectName = this.normalizeProjectName(fileSignature.getProjectName());
        return projectName === INTERNAL_SDK_PROJECT_NAME &&
            !this.isRegExpType(type) &&
            fileSignature.getFileName().endsWith(WELL_KNOWN_SYMBOL_DECLARATION_FILE);
    }

    private isRegExpType(type: Type): boolean {
        const typeName = this.getComparableTypeName(type);
        if (!typeName) {
            return false;
        }
        const lastSegment = typeName.split('.').pop();
        return lastSegment === REGEXP_TYPE_NAME;
    }

    private normalizeProjectName(projectName: string): string {
        return projectName.startsWith('@') ? projectName.substring(1) : projectName;
    }

    private getComparableTypeName(type: Type): string | null {
        if (type instanceof StringType) {
            return 'String';
        }
        if (type instanceof BooleanType) {
            return 'Boolean';
        }
        if (type instanceof BigIntType) {
            return 'BigInt';
        }
        if (type instanceof ClassType) {
            return this.getClassTypeName(type);
        }
        if (type instanceof UnclearReferenceType) {
            return type.getName();
        }
        return null;
    }

    private getClassTypeName(type: ClassType): string {
        const signature = type.getClassSignature();
        const names: string[] = [signature.getClassName()];
        let namespace = signature.getDeclaringNamespaceSignature();
        while (namespace) {
            names.unshift(namespace.getNamespaceName());
            namespace = namespace.getDeclaringNamespaceSignature();
        }
        return names.join('.');
    }

    private getEts1NumberEts2IntLongChangedParamCategories(ets1Sig: MethodSignature, ets2Sig: MethodSignature): NumberCategory[] {
        const ets1Params = ets1Sig.getMethodSubSignature().getParameters();
        const ets2Params = ets2Sig.getMethodSubSignature().getParameters();
        const categories: NumberCategory[] = [];
        const comparableLength = Math.min(ets1Params.length, ets2Params.length);
        for (let i = 0; i < comparableLength; i++) {
            const category = this.getEts1NumberEts2IntLongChangedCategory(ets1Params[i].getType(), ets2Params[i].getType());
            if (category) {
                categories.push(category);
            }
        }
        return categories;
    }

    private getEts1NumberEts2IntLongChangedCategory(ets1Type: Type, ets2Type: Type): NumberCategory | null {
        if (!(ets1Type instanceof NumberType)) {
            return null;
        }
        return this.getIntLongCategoryFromType(ets2Type) ?? this.getSingleIntLongCategoryFromUnionType(ets2Type);
    }

    private getSingleIntLongCategoryFromUnionType(type: Type): NumberCategory.int | NumberCategory.long | null {
        if (!(type instanceof UnionType)) {
            return null;
        }
        const categories = new Set<NumberCategory.int | NumberCategory.long>();
        for (const unionType of type.getTypes()) {
            const category = this.getIntLongCategoryFromType(unionType);
            if (category) {
                categories.add(category);
            }
        }
        return categories.size === 1 ? [...categories][0] : null;
    }
}
