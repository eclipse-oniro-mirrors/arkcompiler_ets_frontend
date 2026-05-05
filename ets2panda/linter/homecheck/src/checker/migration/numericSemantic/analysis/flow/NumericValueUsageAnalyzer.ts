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
    AbstractExpr,
    AbstractRef,
    Local,
    Stmt,
    Value,
} from 'arkanalyzer/lib';
import {
    NullConstant,
    NumberConstant,
    StringConstant,
    UndefinedConstant,
} from 'arkanalyzer/lib/core/base/Constant';
import { Language } from 'arkanalyzer/lib/core/model/ArkFile';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import {
    IssueInfo,
    IssueReason,
    NumberCategory,
} from '../../core/NumericSemanticTypes';
import { NumericLiteralUtils } from '../../core/NumericLiteralUtils';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericValueUsageAnalyzer');

interface NumericValueUsageAnalyzerOptions {
    isLocalOnlyUsedAsIntLong(stmt: Stmt, local: Local, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    isAbstractExprOnlyUsedAsIntLong(stmt: Stmt, expr: AbstractExpr, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
    isAbstractRefOnlyUsedAsIntLong(stmt: Stmt, ref: AbstractRef, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason;
}

export class NumericValueUsageAnalyzer {
    constructor(private options: NumericValueUsageAnalyzerOptions) {}

    // 此处value作为函数入参、数组下标、a/b，因为三地址码原则的限制，只可能是Local和NumberConstant类型，其他value的类型均不可能存在
    public checkValueOnlyUsedAsIntLong(stmt: Stmt, value: Value, hasChecked: Map<Local, IssueInfo>, numberCategory: NumberCategory): IssueReason {
        if (stmt.getCfg().getDeclaringMethod().getLanguage() !== Language.ARKTS1_2) {
            return IssueReason.RelatedWithNonETS2;
        }
        if (value instanceof NumberConstant) {
            if (NumericLiteralUtils.isNumberConstantActuallyFloat(value)) {
                return IssueReason.UsedWithOtherType;
            }
            return IssueReason.OnlyUsedAsIntLong;
        }
        if (value instanceof UndefinedConstant || value instanceof NullConstant) {
            // 对于用null或undefined赋值的场景，认为未进行初始化，还需其他赋值语句进行检查
            return IssueReason.OnlyUsedAsIntLong;
        }
        if (value instanceof StringConstant) {
            // 存在将‘100%’，‘auto’等赋值给numberType的情况，可能是ArkAnalyzer对左值的推导有错误，左值应该是联合类型
            // TODO: arr[await foo()]语句ArkIR将index表示成‘await %2’,应该表示成ArkAwaitExpr，但都认为是number，仍旧是正确结果
            return IssueReason.UsedWithOtherType;
        }
        if (value instanceof Local) {
            return this.options.isLocalOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory);
        }
        if (value instanceof AbstractExpr) {
            return this.options.isAbstractExprOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory);
        }
        if (value instanceof AbstractRef) {
            return this.options.isAbstractRefOnlyUsedAsIntLong(stmt, value, hasChecked, numberCategory);
        }
        logger.error(`Need to handle new value type: ${value.getType().getTypeString()}`);
        return IssueReason.Other;
    }
}
