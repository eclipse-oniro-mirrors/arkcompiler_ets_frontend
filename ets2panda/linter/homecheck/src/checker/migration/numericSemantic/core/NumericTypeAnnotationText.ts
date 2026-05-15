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

import { NumberCategory } from './NumericSemanticTypes';

export class NumericTypeAnnotationText {
    public static getNumberCategory(typeText: string): NumberCategory | null {
        if (this.containsTypeToken(typeText, NumberCategory.int)) {
            return NumberCategory.int;
        }
        if (this.containsTypeToken(typeText, NumberCategory.long)) {
            return NumberCategory.long;
        }
        if (this.containsTypeToken(typeText, NumberCategory.number)) {
            return NumberCategory.number;
        }
        return null;
    }

    public static containsTypeToken(typeText: string, typeName: NumberCategory): boolean {
        return new RegExp(`(^|[^A-Za-z0-9_$])${typeName}([^A-Za-z0-9_$]|$)`, 'u').test(typeText);
    }

    public static replaceTypeToken(typeText: string, fromType: NumberCategory, toType: NumberCategory): string {
        return typeText.replace(new RegExp(`(^|[^A-Za-z0-9_$])${fromType}([^A-Za-z0-9_$]|$)`, 'gu'), `$1${toType}$2`);
    }
}
