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

import fs from 'fs';
import path from 'path';
import {
    CONSTRUCTOR_NAME,
    INSTANCE_INIT_METHOD_NAME,
    MethodSignature,
    Scene,
    SceneConfig,
    STATIC_INIT_METHOD_NAME,
    TEMP_LOCAL_PREFIX,
    Type,
} from 'arkanalyzer/lib';
import { ArkClass, NumberType } from 'arkanalyzer';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import {
    BUILTIN_CONSTRUCT_SIGNATURE_METHOD_NAME,
    BUILTIN_DYN_DECL_PROJECT_NAME,
    BUILTIN_STA_DECL_PROJECT_NAME,
    BuiltinNumberChange,
    BuiltinDeclarationRules,
    BuiltinSignatureField,
    BuiltinSignatureInfo,
    BuiltinSignatureMethod,
    NumberCategory,
} from '../../../core/NumericSemanticTypes';
import { BuiltinTypeChangePathCollector } from './BuiltinTypeChangePathCollector';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'BuiltinDeclarationRuleBuilder');

interface BuiltinDeclarationRuleBuilderOptions {
    normalizeClassName(className: string): string;
    isSignatureMatched(dynSignature: MethodSignature, staSignature: MethodSignature): boolean;
    getIntLongCategoryFromType(type: Type): NumberCategory.int | NumberCategory.long | null;
}

export class BuiltinDeclarationRuleBuilder {
    constructor(private options: BuiltinDeclarationRuleBuilderOptions) {}

    public getBuiltinSignatureDeclarationRules(dynFiles: string[], staFiles: string[]): BuiltinDeclarationRules | null {
        const dynScene = this.buildBuiltinDeclarationScene(BUILTIN_DYN_DECL_PROJECT_NAME, dynFiles);
        const staScene = this.buildBuiltinDeclarationScene(BUILTIN_STA_DECL_PROJECT_NAME, staFiles);
        if (!dynScene || !staScene) {
            return null;
        }
        return this.diffBuiltinSignatureInfo(this.collectBuiltinSignatureInfo(dynScene), this.collectBuiltinSignatureInfo(staScene));
    }

    private buildBuiltinDeclarationScene(projectName: string, filePaths: string[]): Scene | null {
        const files = [...new Set(filePaths)].filter(filePath => fs.existsSync(filePath) && fs.statSync(filePath).isFile());
        if (files.length === 0) {
            return null;
        }

        const projectDir = this.getCommonParentDir(files);
        const projectFiles = files.map(filePath => path.relative(projectDir, filePath));
        const sceneConfig = new SceneConfig();
        sceneConfig.buildFromProjectFiles(projectName, projectDir, projectFiles, []);
        sceneConfig.getOptions().enableBuiltIn = false;

        try {
            const scene = new Scene();
            scene.buildSceneFromFiles(sceneConfig);
            scene.inferTypes();
            return scene;
        } catch (e) {
            logger.debug(`Failed to build builtin declaration scene ${projectName}: ${e}`);
            return null;
        }
    }

    private getCommonParentDir(filePaths: string[]): string {
        const dirs = filePaths.map(filePath => path.dirname(path.resolve(filePath)).split(path.sep));
        const commonParts: string[] = [];
        const minLength = Math.min(...dirs.map(dir => dir.length));
        for (let i = 0; i < minLength; i++) {
            const part = dirs[0][i];
            if (!dirs.every(dir => dir[i] === part)) {
                break;
            }
            commonParts.push(part);
        }
        return commonParts.length === 0 ? path.parse(filePaths[0]).root : commonParts.join(path.sep) || path.sep;
    }

    private collectBuiltinSignatureInfo(scene: Scene): BuiltinSignatureInfo {
        const info: BuiltinSignatureInfo = {
            methods: new Map<string, BuiltinSignatureMethod[]>(),
            fields: new Map<string, BuiltinSignatureField[]>(),
        };
        const arkFiles = scene.getFiles().concat(scene.getSdkArkFiles());
        for (const arkFile of arkFiles) {
            for (const arkClass of arkFile.getClasses()) {
                this.collectBuiltinSignatureClassInfo(arkClass, info);
            }
            for (const namespace of arkFile.getAllNamespacesUnderThisFile()) {
                for (const arkClass of namespace.getClasses()) {
                    this.collectBuiltinSignatureClassInfo(arkClass, info);
                }
            }
        }
        return info;
    }

    private collectBuiltinSignatureClassInfo(arkClass: ArkClass, info: BuiltinSignatureInfo): void {
        if (arkClass.isDefaultArkClass() || arkClass.isAnonymousClass()) {
            return;
        }

        const className = this.options.normalizeClassName(arkClass.getName());
        for (const method of arkClass.getMethods(true)) {
            const methodName = this.getBuiltinSignatureMethodName(method.getName());
            if (!methodName) {
                continue;
            }
            const signatures = method.getDeclareSignatures() ?? [method.getSignature()];
            signatures.forEach(signature => this.addBuiltinSignatureMethod(info, { className, methodName, signature }));
        }
        for (const field of arkClass.getFields()) {
            this.addBuiltinSignatureField(info, { className, fieldName: field.getName(), type: field.getType() });
        }
    }

    private getBuiltinSignatureMethodName(methodName: string): string | null {
        if (
            methodName === INSTANCE_INIT_METHOD_NAME ||
            methodName === STATIC_INIT_METHOD_NAME ||
            methodName.startsWith(TEMP_LOCAL_PREFIX) ||
            methodName.startsWith('Get-')
        ) {
            return null;
        }
        if (methodName === BUILTIN_CONSTRUCT_SIGNATURE_METHOD_NAME) {
            return CONSTRUCTOR_NAME;
        }
        return methodName;
    }

    private addBuiltinSignatureMethod(info: BuiltinSignatureInfo, method: BuiltinSignatureMethod): void {
        const key = this.getBuiltinMethodKey(method.className, method.methodName);
        const methods = info.methods.get(key) ?? [];
        methods.push(method);
        info.methods.set(key, methods);
    }

    private addBuiltinSignatureField(info: BuiltinSignatureInfo, field: BuiltinSignatureField): void {
        const key = this.getBuiltinFieldKey(field.className, field.fieldName);
        const fields = info.fields.get(key) ?? [];
        fields.push(field);
        info.fields.set(key, fields);
    }

    private diffBuiltinSignatureInfo(dynInfo: BuiltinSignatureInfo, staInfo: BuiltinSignatureInfo): BuiltinDeclarationRules {
        const rules: BuiltinDeclarationRules = { apiRules: [], fieldRules: [] };
        staInfo.methods.forEach((staMethods, key) => {
            const dynMethods = dynInfo.methods.get(key);
            if (!dynMethods || dynMethods.length === 0) {
                return;
            }
            staMethods.forEach(staMethod => {
                const matchedDynMethods = dynMethods.filter(dynMethod =>
                    this.options.isSignatureMatched(dynMethod.signature, staMethod.signature)
                );
                if (matchedDynMethods.length === 0) {
                    return;
                }
                const changes = this.getBuiltinSignatureChanges(matchedDynMethods, staMethod.signature);
                const args = this.getBuiltinSignatureChangedArgs(changes);
                const returnType = this.getBuiltinSignatureChangedReturnType(changes);
                if (changes.length === 0) {
                    return;
                }
                const rule = {
                    className: staMethod.className,
                    methodName: staMethod.methodName,
                    paramCount: staMethod.signature.getMethodSubSignature().getParameters().length,
                    hasRest: this.hasRestParameter(staMethod.signature),
                    signature: staMethod.signature,
                    args: Object.keys(args).length > 0 ? args : undefined,
                    returnType: returnType ?? undefined,
                    changes,
                };
                rules.apiRules.push(rule);
            });
        });

        staInfo.fields.forEach((staFields, key) => {
            const dynFields = dynInfo.fields.get(key);
            if (!dynFields || dynFields.length === 0 || !dynFields.some(field => field.type instanceof NumberType)) {
                return;
            }
            staFields.forEach(staField => {
                const category = this.options.getIntLongCategoryFromType(staField.type);
                if (!category) {
                    return;
                }
                rules.fieldRules.push({ className: staField.className, fieldName: staField.fieldName, type: category });
            });
        });
        return rules;
    }

    private getBuiltinSignatureChanges(matchedDynMethods: BuiltinSignatureMethod[], staSignature: MethodSignature): BuiltinNumberChange[] {
        const changes = new Map<string, BuiltinNumberChange>();
        const collector = new BuiltinTypeChangePathCollector({
            getIntLongCategoryFromType: type => this.options.getIntLongCategoryFromType(type),
        });
        const staParams = staSignature.getMethodSubSignature().getParameters();
        for (const matchedDynMethod of matchedDynMethods) {
            const dynParams = matchedDynMethod.signature.getMethodSubSignature().getParameters();
            const paramLength = Math.min(dynParams.length, staParams.length);
            for (let index = 0; index < paramLength; index++) {
                collector.collect(dynParams[index].getType(), staParams[index].getType(), { root: 'arg', argIndex: index, steps: [] })
                    .forEach(change => changes.set(this.getChangeKey(change), change));
            }
            collector.collect(matchedDynMethod.signature.getType(), staSignature.getType(), { root: 'return', steps: [] })
                .forEach(change => changes.set(this.getChangeKey(change), change));
        }
        return [...changes.values()];
    }

    private getBuiltinSignatureChangedArgs(changes: BuiltinNumberChange[]): Record<number, NumberCategory> {
        const args: Record<number, NumberCategory> = {};
        changes.forEach(change => {
            if (change.path.root === 'arg' && change.path.argIndex !== undefined && change.path.steps.length === 0) {
                args[change.path.argIndex] = change.category;
            }
        });
        return args;
    }

    private getBuiltinSignatureChangedReturnType(changes: BuiltinNumberChange[]): NumberCategory | null {
        return changes.find(change => change.path.root === 'return' && change.path.steps.length === 0)?.category ?? null;
    }

    private getChangeKey(change: BuiltinNumberChange): string {
        return `${change.path.root}#${change.path.argIndex ?? ''}#${JSON.stringify(change.path.steps)}#${change.category}`;
    }

    private hasRestParameter(signature: MethodSignature): boolean {
        return signature.getMethodSubSignature().getParameters().some(param => param.hasDotDotDotToken());
    }

    private getBuiltinMethodKey(className: string, methodName: string): string {
        return `${this.options.normalizeClassName(className)}#${methodName}`;
    }

    private getBuiltinFieldKey(className: string, fieldName: string): string {
        return `${this.options.normalizeClassName(className)}#${fieldName}`;
    }
}
