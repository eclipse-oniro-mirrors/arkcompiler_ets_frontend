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
    ClassType,
    MethodSignature,
    Type,
} from 'arkanalyzer/lib';
import {
    AliasType,
    UnclearReferenceType,
} from 'arkanalyzer';
import {
    NumberCategory,
    SignatureMatchOptions,
} from './NumericSemanticTypes';
import { NumericSignatureMatcher } from './NumericSignatureMatcher';

export class NumericTypeClassifier {
    public isIntType(checkType: Type): boolean {
        if (checkType instanceof AliasType || checkType instanceof UnclearReferenceType) {
            if (checkType.getName() === NumberCategory.int) {
                return true;
            }
        }
        return this.hasPromiseGenericType(checkType, type => this.isIntType(type));
    }

    public isLongType(checkType: Type): boolean {
        if (checkType instanceof AliasType || checkType instanceof UnclearReferenceType) {
            if (checkType.getName() === NumberCategory.long) {
                return true;
            }
        }
        return this.hasPromiseGenericType(checkType, type => this.isLongType(type));
    }

    public isEts1NumberEts2IntLongSignatureMatched(
        ets1Sig: MethodSignature,
        ets2Sig: MethodSignature,
        options: SignatureMatchOptions = {}
    ): boolean {
        return this.getSignatureMatcher().isEts1NumberEts2IntLongSignatureMatched(ets1Sig, ets2Sig, options);
    }

    public getIntLongCategoryFromType(type: Type): NumberCategory.int | NumberCategory.long | null {
        return this.getSignatureMatcher().getIntLongCategoryFromType(type);
    }

    private getSignatureMatcher(): NumericSignatureMatcher {
        return new NumericSignatureMatcher({
            isIntType: type => this.isIntType(type),
            isLongType: type => this.isLongType(type),
        });
    }

    private hasPromiseGenericType(checkType: Type, predicate: (type: Type) => boolean): boolean {
        if (checkType instanceof UnclearReferenceType && checkType.getName() === 'Promise') {
            const gTypes = checkType.getGenericTypes();
            for (const gType of gTypes) {
                if (predicate(gType)) {
                    return true;
                }
            }
        }
        if (checkType instanceof ClassType && checkType.getClassSignature().getClassName() === 'Promise') {
            const gTypes = checkType.getRealGenericTypes();
            if (gTypes === undefined) {
                return false;
            }
            for (const gType of gTypes) {
                if (predicate(gType)) {
                    return true;
                }
            }
        }
        return false;
    }
}
