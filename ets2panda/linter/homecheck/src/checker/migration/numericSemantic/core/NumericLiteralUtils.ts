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

import { NumberConstant } from 'arkanalyzer/lib/core/base/Constant';
import { INT32_BOUNDARY } from './NumericSemanticTypes';

export class NumericLiteralUtils {
    public static isNumberConstantActuallyFloat(constant: NumberConstant): boolean {
        const valueStr = constant.getValue().toLowerCase();
        if (this.isScientificStr(valueStr)) {
            return true;
        }
        if (valueStr.includes('.') && !valueStr.includes('e')) {
            return true;
        }
        const num = Number(constant.getValue());
        if (isNaN(num) || num > INT32_BOUNDARY) {
            return true;
        }
        return !Number.isInteger(num);
    }

    public static isFloatActuallyInt(constant: NumberConstant): boolean {
        const parts = constant.getValue().split('.');
        if (parts.length !== 2) {
            return false;
        }
        return /^0+$/.test(parts[1]);
    }

    public static createFixTextForIntLiteral(valueStr: string): string {
        if (!this.isNotDecimalNumber(valueStr)) {
            return valueStr + '.0';
        }
        return valueStr + '.toDouble()';
    }

    public static createFixTextForEnumValue(valueStr: string): string {
        return valueStr + '.valueOf().toDouble()';
    }

    private static isScientificStr(str: string): boolean {
        const scientificRegex = /^[+-]?(?:\d+\.?\d*|\.\d+)[eE][+-]?\d+$/;
        return scientificRegex.test(str);
    }

    private static isNotDecimalNumber(value: string): boolean {
        const loweredValue = value.toLowerCase();
        return loweredValue.startsWith('0b') || loweredValue.startsWith('0x') || loweredValue.startsWith('0o') || loweredValue.includes('e');
    }
}
