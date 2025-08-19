/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { Sdk } from 'arkanalyzer/lib/Config';
import fs from 'fs';
import { AnyType, ArkField, ArkFile, ArkMethod, ClassSignature, EnumValueType, Local, MethodSignature, UnionType, Value } from 'arkanalyzer';
import { Utils } from './Utils';
import { AbstractFieldRef, ArkNamespace, NamespaceSignature } from 'arkanalyzer/lib';

export class SdkUtils {
    static OhosSdkName = 'ohosSdk';
    static HmsSdkName = 'hmsSdk';

    static getEts2SdksWithSdkRelativePath(sdkMap: Map<String, Sdk>): Sdk[] | null {
        const Ets1DirName = 'ets1.1';
        const Ets2DirName = 'ets1.2';
        const ets1OhosSdk = sdkMap.get(this.OhosSdkName);
        const ets1HmsSdk = sdkMap.get(this.HmsSdkName);
        let sdks: Sdk[] = [];
        if (ets1OhosSdk !== undefined) {
            const sdkPath = ets1OhosSdk.path;
            if (sdkPath.includes(Ets1DirName)) {
                const ets2SdkPath = sdkPath.replace(Ets1DirName, Ets2DirName);
                if (fs.existsSync(ets2SdkPath)) {
                    sdks.push({ name: this.OhosSdkName, path: ets2SdkPath, moduleName: ets1OhosSdk.moduleName });
                }
            }
        }
        if (ets1HmsSdk !== undefined) {
            const sdkPath = ets1HmsSdk.path;
            if (sdkPath.includes(Ets1DirName)) {
                const ets2SdkPath = sdkPath.replace(Ets1DirName, Ets2DirName);
                if (fs.existsSync(ets2SdkPath)) {
                    sdks.push({ name: this.HmsSdkName, path: ets2SdkPath, moduleName: ets1HmsSdk.moduleName });
                }
            }
        }
        if (sdks.length > 0) {
            return sdks;
        }
        return null;
    }

    static isMethodFromSdk(method: ArkMethod): boolean {
        const projectName = method.getDeclaringArkFile().getProjectName();
        return projectName === this.OhosSdkName || projectName === this.HmsSdkName;
    }

    static isFieldFromSdk(fieldRef: AbstractFieldRef): boolean {
        const projectName = fieldRef.getFieldSignature().getDeclaringSignature().getDeclaringFileSignature().getProjectName();
        return projectName === this.OhosSdkName || projectName === this.HmsSdkName;
    }

    static getSdkMatchedSignature(ets1SDK: ArkMethod, args: Value[]): MethodSignature | null {
        const declareSigs = ets1SDK.getDeclareSignatures();
        if (declareSigs === null) {
            return null;
        }
        if (declareSigs.length === 1) {
            return declareSigs[0];
        }

        let ets1SigMatched: MethodSignature | null = null;
        for (const sig of declareSigs) {
            const params = sig.getMethodSubSignature().getParameters();
            if (params.length < args.length) {
                continue;
            }
            let matched = true;
            for (let i = 0; i < args.length; i++) {
                const argType = args[i].getType();
                const paramType = params[i].getType();
                if (argType === paramType) {
                    continue;
                }
                if (argType instanceof AnyType || argType instanceof EnumValueType) {
                    continue;
                }
                if (!(argType instanceof UnionType) || !Utils.isUnionTypeContainsType(argType, paramType)) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                ets1SigMatched = sig;
                break;
            }
        }
        return ets1SigMatched;
    }

    static getSdkField(etsFile: ArkFile, fieldRef: AbstractFieldRef): ArkField | Local | null {
        const declaringSig = fieldRef.getFieldSignature().getDeclaringSignature();
        if (declaringSig instanceof ClassSignature) {
            const declaringNS = declaringSig.getDeclaringNamespaceSignature();
            if (!declaringNS) {
                return etsFile?.getClassWithName(declaringSig.getClassName())?.getFieldWithName(fieldRef.getFieldName()) ?? null;
            }
            const namespace = this.getSdkNamespace(etsFile, declaringNS);
            if (!namespace) {
                return null;
            }
            return namespace.getClassWithName(declaringSig.getClassName())?.getFieldWithName(fieldRef.getFieldName()) ?? null;
        }
        const namespace = this.getSdkNamespace(etsFile, declaringSig);
        if (!namespace) {
            return null;
        }
        return namespace.getDefaultClass().getDefaultArkMethod()?.getBody()?.getLocals().get(fieldRef.getFieldName()) ?? null;
    }

    static getSdkNamespace(etsFile: ArkFile, namespaceSig: NamespaceSignature): ArkNamespace | null {
        const declaringNsSig = namespaceSig.getDeclaringNamespaceSignature();
        if (!declaringNsSig) {
            return etsFile.getNamespace(namespaceSig);
        }
        const declaringNS = this.getSdkNamespace(etsFile, declaringNsSig);
        if (!declaringNS) {
            return null;
        }
        return declaringNS.getNamespace(namespaceSig);
    }
}
