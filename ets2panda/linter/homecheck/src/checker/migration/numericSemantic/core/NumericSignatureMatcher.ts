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
    ClassSignature,
    classSignatureCompare,
    ClassType,
    FunctionType,
    MethodSignature,
    Type,
    UnknownType,
} from 'arkanalyzer/lib';
import { NumberType, UnclearReferenceType } from 'arkanalyzer';
import { SdkUtils } from '../../../../utils/common/SDKUtils';
import {
    BUILTIN_DYN_DECL_PROJECT_NAME,
    BUILTIN_STA_DECL_PROJECT_NAME,
    NumberCategory,
    SignatureMatchOptions,
} from './NumericSemanticTypes';

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
        if (options.allowArrayLikeTypes && this.compareArrayLikeTypes(param1, param2, options)) {
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
        return this.isEts1NumberEts2IntLongSignatureMatched(sig1, sig2, options) && this.isFunctionReturnTypeMatched(sig1.getType(), sig2.getType(), options);
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
            return name === 'any' || name === 'unknown' || name === 'AnyKeyword' || name === 'UnknownKeyword';
        }
        return false;
    }

    private compareArrayLikeTypes(param1: Type, param2: Type, options: SignatureMatchOptions): boolean {
        const elementType1 = this.getArrayLikeElementType(param1);
        const elementType2 = this.getArrayLikeElementType(param2);
        if (!elementType1 || !elementType2) {
            return false;
        }
        return this.isEts1NumberEts2IntLongParamMatched(elementType1, elementType2, options);
    }

    private getArrayLikeElementType(type: Type): Type | null {
        if (type instanceof ArrayType) {
            return type.getBaseType();
        }
        if (type instanceof ClassType && this.isArrayLikeClassName(type.getClassSignature().getClassName())) {
            return type.getRealGenericTypes()?.[0] ?? null;
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
        return this.getIntLongCategoryFromType(ets2Type);
    }
}
