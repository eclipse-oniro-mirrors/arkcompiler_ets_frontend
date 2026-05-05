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

import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { IssueReason, NumberCategory, RuleCategory } from './NumericSemanticTypes';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericSemanticIssueText');

export class NumericSemanticIssueText {
    public static getProblem(ruleCategory: RuleCategory, reason: IssueReason): string | null {
        if (ruleCategory === RuleCategory.SDKIntType) {
            return 'SDKIntType-' + reason;
        }
        if (ruleCategory === RuleCategory.BuiltinIntType) {
            return 'BuiltinIntType-' + reason;
        }
        if (ruleCategory === RuleCategory.NumericLiteral) {
            return 'NumericLiteral-' + reason;
        }
        if (ruleCategory === RuleCategory.ArrayIndex) {
            return 'IndexIntType-' + reason;
        }
        logger.error(`Have not support rule ${ruleCategory} yet.`);
        return null;
    }

    public static getApiSourceDesc(ruleCategory: RuleCategory): string {
        if (ruleCategory === RuleCategory.BuiltinIntType) {
            return 'builtin API';
        }
        return 'SDK API';
    }

    public static getDesc(ruleCategory: RuleCategory, reason: IssueReason, couldAutofix: boolean): string | null {
        if (ruleCategory === RuleCategory.NumericLiteral) {
            if (reason === IssueReason.OnlyUsedAsIntLong) {
                return `It is used as ${NumberCategory.int} (${ruleCategory})`;
            }
            return `It is used as ${NumberCategory.number} (${ruleCategory})`;
        }
        if (ruleCategory === RuleCategory.ArrayIndex) {
            if (reason === IssueReason.OnlyUsedAsIntLong) {
                return `It is used as ${NumberCategory.int} (${ruleCategory})`;
            }
            if (reason === IssueReason.ActuallyIntConstant) {
                return `The number constant could be changed to int constant (${ruleCategory})`;
            }
            if (couldAutofix) {
                return `It is used as ${NumberCategory.number} (${ruleCategory})`;
            }
            return `The array index is used as ${NumberCategory.number}, please check if it's ok (${ruleCategory})`;
        }
        logger.error(`Have not support rule ${ruleCategory} yet.`);
        return null;
    }

    public static getRuleCategoryPriority(ruleCategory: RuleCategory | null): number {
        switch (ruleCategory) {
            case RuleCategory.BuiltinIntType:
                return 2;
            case RuleCategory.SDKIntType:
            case RuleCategory.ArrayIndex:
            case RuleCategory.NumericLiteral:
                return 1;
            default:
                return 0;
        }
    }

    public static getRuleCategoryFromProblem(problem: string): RuleCategory | null {
        if (problem.startsWith('SDKIntType-')) {
            return RuleCategory.SDKIntType;
        }
        if (problem.startsWith('BuiltinIntType-')) {
            return RuleCategory.BuiltinIntType;
        }
        if (problem.startsWith('IndexIntType-')) {
            return RuleCategory.ArrayIndex;
        }
        if (problem.startsWith('NumericLiteral-')) {
            return RuleCategory.NumericLiteral;
        }
        return null;
    }
}
