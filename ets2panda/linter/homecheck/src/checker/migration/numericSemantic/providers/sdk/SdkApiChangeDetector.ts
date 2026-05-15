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
    ArkMethod,
    FileSignature,
    MethodSignature,
    Scene,
    Type,
    Value,
} from 'arkanalyzer/lib';
import { ArkFile } from 'arkanalyzer';
import { Sdk } from 'arkanalyzer/lib/Config';
import { SdkUtils } from '../../../../../utils/common/SDKUtils';
import { NumberCategory } from '../../core/NumericSemanticTypes';
import { NumericSignatureMatcher } from '../../core/NumericSignatureMatcher';

interface SdkApiChangeDetectorOptions {
    scene: Scene;
    ets2SdkScene?: Scene;
    ets2Sdks?: Sdk[];
    isIntType(type: Type): boolean;
    isLongType(type: Type): boolean;
    isNumberLikeType(type: Type): boolean;
}

export class SdkApiChangeDetector {
    constructor(private options: SdkApiChangeDetectorOptions) {}

    // 语句为sdk的调用且形参有int或long类型，找出所有int类型形参的实参
    public getIntLongArgsFromInvokeExpr(invokeExpr: AbstractInvokeExpr): Map<Value, NumberCategory> | null {
        const callMethod = this.options.scene.getMethod(invokeExpr.getMethodSignature());
        if (callMethod === null || !SdkUtils.isMethodFromSdk(callMethod)) {
            return null;
        }

        const args = invokeExpr.getArgs();
        // 根据找到的对应arkts1.1中的SDK接口匹配到对应在arkts1.2中的SDK接口
        const ets2SdkSignature = this.getEts2SdkSignatureWithEts1Method(callMethod, args, true);
        if (ets2SdkSignature === null) {
            return null;
        }
        const params = ets2SdkSignature.getMethodSubSignature().getParameters();
        if (params.length < args.length) {
            return null;
        }

        const res: Map<Value, NumberCategory> = new Map<Value, NumberCategory>();
        args.forEach((arg, index) => {
            if (this.options.isIntType(params[index].getType()) && !this.options.isIntType(arg.getType())) {
                res.set(arg, NumberCategory.int);
            } else if (this.options.isLongType(params[index].getType()) && !this.options.isLongType(arg.getType())) {
                res.set(arg, NumberCategory.long);
            }
        });
        return res.size === 0 ? null : res;
    }

    public checkReturnType(invokeExpr: AbstractInvokeExpr): NumberCategory | null {
        const callMethod = this.options.scene.getMethod(invokeExpr.getMethodSignature());
        if (callMethod === null || !SdkUtils.isMethodFromSdk(callMethod)) {
            return null;
        }

        // 根据找到的对应arkts1.1中的SDK接口匹配到对应在arkts1.2中的SDK接口
        const ets2SdkSignature = this.getEts2SdkSignatureWithEts1Method(callMethod, invokeExpr.getArgs(), false);
        if (ets2SdkSignature === null) {
            return null;
        }
        const returnType = ets2SdkSignature.getType();
        if (this.options.isLongType(returnType)) {
            return NumberCategory.long;
        }
        if (this.options.isIntType(returnType)) {
            return NumberCategory.int;
        }
        return null;
    }

    public checkFieldType(fieldRef: AbstractFieldRef): NumberCategory | null {
        if (!SdkUtils.isFieldFromSdk(fieldRef) || !this.options.isNumberLikeType(fieldRef.getType())) {
            return null;
        }
        const ets1SdkFileSig = fieldRef.getFieldSignature().getDeclaringSignature().getDeclaringFileSignature();
        const ets2SdkFileSig = new FileSignature(ets1SdkFileSig.getProjectName(), ets1SdkFileSig.getFileName().replace('.d.ts', '.d.ets'));
        const ets2SdkFileSigBak = new FileSignature(ets1SdkFileSig.getProjectName(), ets1SdkFileSig.getFileName());
        const ets2SdkFile = this.options.ets2SdkScene?.getFile(ets2SdkFileSig) ?? this.options.ets2SdkScene?.getFile(ets2SdkFileSigBak);
        if (!ets2SdkFile) {
            return null;
        }
        const ets2Field = SdkUtils.getSdkField(ets2SdkFile, fieldRef);
        if (!ets2Field) {
            return null;
        }
        if (this.options.isIntType(ets2Field.getType())) {
            return NumberCategory.int;
        }
        if (this.options.isLongType(ets2Field.getType())) {
            return NumberCategory.long;
        }
        return null;
    }

    // checkArg = true is for checking SDK arg with int or long; otherwise is for checking SDK return with int or long
    public getEts2SdkSignatureWithEts1Method(ets1SDK: ArkMethod, args: Value[], checkArg: boolean, exactMatch: boolean = true): MethodSignature | null {
        const ets2Sdks = this.options.ets2Sdks;
        if (ets2Sdks === undefined || ets2Sdks.length === 0) {
            return null;
        }

        const ets1SigMatched = SdkUtils.getSdkMatchedSignature(ets1SDK, args);
        if (ets1SigMatched === null) {
            return null;
        }

        const ets1SdkFileSig = ets1SDK.getDeclaringArkFile().getFileSignature();
        const ets2SdkFileSig = new FileSignature(ets1SdkFileSig.getProjectName(), ets1SdkFileSig.getFileName().replace('.d.ts', '.d.ets'));
        const ets2SdkFileSigBak = new FileSignature(ets1SdkFileSig.getProjectName(), ets1SdkFileSig.getFileName());
        const ets2SdkFile = this.options.ets2SdkScene?.getFile(ets2SdkFileSig) ?? this.options.ets2SdkScene?.getFile(ets2SdkFileSigBak);
        if (!ets2SdkFile) {
            return null;
        }
        const ets2SdkMethod = this.getEts2SdkWithEts1SdkInfo(ets2SdkFile, ets1SDK);
        if (ets2SdkMethod === null) {
            return null;
        }
        const declareSigs = ets2SdkMethod.getDeclareSignatures();
        if (declareSigs === null) {
            return null;
        }
        if (!exactMatch && declareSigs.length === 1) {
            return declareSigs[0];
        }
        if (checkArg) {
            return this.getNumericSignatureMatcher().matchEts1NumberEts2IntLongMethodSig(declareSigs, ets1SigMatched);
        }
        return this.getNumericSignatureMatcher().matchEts1NumberEts2IntLongReturnSig(declareSigs, ets1SigMatched);
    }

    private getEts2SdkWithEts1SdkInfo(ets2File: ArkFile, ets1SDK: ArkMethod): ArkMethod | null {
        const ets1Class = ets1SDK.getDeclaringArkClass();
        const ets1Namespace = ets1Class.getDeclaringArkNamespace();
        if (ets1Namespace === undefined) {
            const ets2Class = ets2File.getClassWithName(ets1Class.getName());
            return ets2Class?.getMethodWithName(ets1SDK.getName()) ?? ets2Class?.getStaticMethodWithName(ets1SDK.getName()) ?? null;
        }
        const ets2Class = ets2File.getNamespaceWithName(ets1Namespace.getName())?.getClassWithName(ets1Class.getName());
        return ets2Class?.getMethodWithName(ets1SDK.getName()) ?? ets2Class?.getStaticMethodWithName(ets1SDK.getName()) ?? null;
    }

    private getNumericSignatureMatcher(): NumericSignatureMatcher {
        return new NumericSignatureMatcher({
            isIntType: type => this.options.isIntType(type),
            isLongType: type => this.options.isLongType(type),
        });
    }
}
