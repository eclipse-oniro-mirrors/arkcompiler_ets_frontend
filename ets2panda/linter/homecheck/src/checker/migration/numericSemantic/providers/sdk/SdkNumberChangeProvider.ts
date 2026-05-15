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
} from 'arkanalyzer/lib';
import {
    ApiNumberChangeProvider,
    ChangedArgCategories,
    ChangedResultCategory,
    RuleCategory,
} from '../../core/NumericSemanticTypes';
import { SdkApiChangeDetector } from './SdkApiChangeDetector';

export class SdkNumberChangeProvider implements ApiNumberChangeProvider {
    constructor(private detector: SdkApiChangeDetector) {}

    public getChangedArgCategories(invokeExpr: AbstractInvokeExpr): ChangedArgCategories {
        return {
            ruleCategory: RuleCategory.SDKIntType,
            args: this.detector.getIntLongArgsFromInvokeExpr(invokeExpr),
        };
    }

    public getChangedReturnCategory(_stmt: ArkAssignStmt, rightInvokeExpr: AbstractInvokeExpr | null): ChangedResultCategory {
        return {
            ruleCategory: RuleCategory.SDKIntType,
            category: rightInvokeExpr ? this.detector.checkReturnType(rightInvokeExpr) : null,
        };
    }

    public getChangedFieldCategory(fieldRef: AbstractFieldRef): ChangedResultCategory {
        return {
            ruleCategory: RuleCategory.SDKIntType,
            category: this.detector.checkFieldType(fieldRef),
            requireNumberLikeLeft: true,
        };
    }
}
