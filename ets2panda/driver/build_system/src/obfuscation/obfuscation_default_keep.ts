
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

export enum KeepRuleType {
    KEEP = 'keep',
    KEEP_CLASS_WITH_MEMBERS = 'keepclasswithmembers',
    KEEP_CLASS_MEMBERS = 'keepclassmembers',
    APPEND_ALL_FILE = 'appendAllFile'
}

export enum RuleTargetType {
    CLASS = 'class',
    INTERFACE = 'interface',
    PACKAGE = 'package',
}
export interface KeepRule {
    value: string;  
    type: RuleTargetType;
}

export type ObfuscationKeepConfig = {
    [K in KeepRuleType]: KeepRule[];
};
export function forEachKeepRule(config: ObfuscationKeepConfig, callback: (rules: KeepRule[], type: KeepRuleType) => void): void {
    Object.entries(config).forEach(([type, rules]) => {
        callback(rules, type as unknown as KeepRuleType);
    });
}
export const obfuscation_default_keep: ObfuscationKeepConfig = {
    [KeepRuleType.KEEP]: [
        { value: 'std.core.* { *; }', type: RuleTargetType.CLASS},
        { value: '*\\$ {*;} { *; }', type: RuleTargetType.PACKAGE},
    ],
    [KeepRuleType.KEEP_CLASS_WITH_MEMBERS]: [],
    [KeepRuleType.KEEP_CLASS_MEMBERS]: [],
    [KeepRuleType.APPEND_ALL_FILE]: [
        { value: '**.*__Options* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__options* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__Options* { *; }', type: RuleTargetType.INTERFACE},
        { value: '**.*__initializeStruct* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__updateStruct* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__build* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__constructor* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__buildCompatibleNode* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__computed* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__monitor* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__builder* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__setDialogController* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__reuseId* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__content* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__elmtId* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__paramsLambda* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__parent* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__instance* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__EntryWrapper* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__EntryPoint* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__EntryComponent* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*__LocalStorage* { *; }', type: RuleTargetType.CLASS},
        { value: '**.*_invoke* { *; }', type: RuleTargetType.INTERFACE},
        { value: '**.*PageLifeCycle* { *; }', type: RuleTargetType.INTERFACE},
        { value: '**.*LayoutCallbacks* { *; }', type: RuleTargetType.INTERFACE},
        { value: '**.*CustomBuilder* { *; }', type: RuleTargetType.INTERFACE},
    ]
};
