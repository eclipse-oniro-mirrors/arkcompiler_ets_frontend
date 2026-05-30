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

import { MethodSignature, Scene, Type } from 'arkanalyzer/lib';
import {
    BuiltinApiRule,
    BuiltinDeclarationRules,
    BuiltinFieldRule,
    NumberCategory,
    RuleOptions,
} from '../../../core/NumericSemanticTypes';
import { BuiltinApiChangeDetector } from '../runtime/BuiltinApiChangeDetector';
import { BuiltinDeclarationFileResolver } from './BuiltinDeclarationFileResolver';
import { BuiltinDeclarationRuleBuilder } from './BuiltinDeclarationRuleBuilder';

interface BuiltinDeclarationRuleProviderOptions {
    scene: Scene;
    ruleOptions?: RuleOptions;
    isSignatureMatched(dynSignature: MethodSignature, staSignature: MethodSignature): boolean;
    getIntLongCategoryFromType(type: Type): NumberCategory.int | NumberCategory.long | null;
}

export class BuiltinDeclarationRuleProvider {
    constructor(private options: BuiltinDeclarationRuleProviderOptions) {}

    public getDeduplicatedDeclarationRules(): BuiltinDeclarationRules {
        const rules = this.getDeclarationRules();
        return {
            apiRules: this.deduplicateApiRules(rules.apiRules),
            fieldRules: this.deduplicateFieldRules(rules.fieldRules),
        };
    }

    public getDeclarationRules(): BuiltinDeclarationRules {
        const staFiles = this.getStaBuiltinDeclarationFiles();
        const dynFiles = this.getDynBuiltinDeclarationFiles(staFiles);
        const builder = new BuiltinDeclarationRuleBuilder({
            normalizeClassName: className => BuiltinApiChangeDetector.normalizeClassName(className),
            isSignatureMatched: (dynSignature, staSignature) => this.options.isSignatureMatched(dynSignature, staSignature),
            getIntLongCategoryFromType: type => this.options.getIntLongCategoryFromType(type),
        });
        const signatureRules = builder.getBuiltinSignatureDeclarationRules(dynFiles, staFiles);
        return signatureRules ?? { apiRules: [], fieldRules: [] };
    }

    public getDynBuiltinDeclarationFiles(staticBuiltinDeclarationFiles: string[] = []): string[] {
        return new BuiltinDeclarationFileResolver(this.options.scene, this.options.ruleOptions)
            .getDynBuiltinDeclarationFiles(staticBuiltinDeclarationFiles);
    }

    public getDynBuiltinDeclarationFilesFromSdkPath(sdkPath: string, targetESVersion?: string): string[] {
        return new BuiltinDeclarationFileResolver(this.options.scene).getDynBuiltinDeclarationFilesFromSdkPath(sdkPath, targetESVersion);
    }

    private getStaBuiltinDeclarationFiles(): string[] {
        return new BuiltinDeclarationFileResolver(this.options.scene, this.options.ruleOptions).getStaBuiltinDeclarationFiles();
    }

    private deduplicateApiRules(rules: BuiltinApiRule[]): BuiltinApiRule[] {
        const res: BuiltinApiRule[] = [];
        const seen = new Set<string>();
        for (const rule of rules) {
            const key = `${BuiltinApiChangeDetector.getClassNameKey(rule.className)}#${rule.methodName}#${rule.paramCount ?? ''}#${rule.hasRest ?? false}#${JSON.stringify(rule.args ?? {})}#${rule.returnType ?? ''}#${rule.signature?.toString() ?? ''}`;
            if (seen.has(key)) {
                continue;
            }
            seen.add(key);
            res.push(rule);
        }
        return res;
    }

    private deduplicateFieldRules(rules: BuiltinFieldRule[]): BuiltinFieldRule[] {
        const res: BuiltinFieldRule[] = [];
        const seen = new Set<string>();
        for (const rule of rules) {
            const key = `${BuiltinApiChangeDetector.getClassNameKey(rule.className)}#${rule.fieldName}#${rule.type}`;
            if (seen.has(key)) {
                continue;
            }
            seen.add(key);
            res.push(rule);
        }
        return res;
    }
}
