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
    ArkAssignStmt,
    ArkCastExpr,
    ArkInstanceFieldRef,
    ClassSignature,
    ClosureFieldRef,
    Local,
    TEMP_LOCAL_PREFIX,
} from 'arkanalyzer/lib';

const ITERATOR_RESULT_CLASS_NAMES = new Set<string>([
    'IteratorResult',
    'IteratorYieldResult',
    'IteratorReturnResult',
]);

export class NumericLocalIgnorePolicy {
    public shouldIgnoreLocal(local: Local): boolean {
        if (local.getName().startsWith(TEMP_LOCAL_PREFIX)) {
            return true;
        }
        const declaringStmt = local.getDeclaringStmt();
        if (declaringStmt instanceof ArkAssignStmt && declaringStmt.getRightOp() instanceof ClosureFieldRef) {
            return true;
        }
        return declaringStmt instanceof ArkAssignStmt && this.isIteratorResultValueLocal(declaringStmt);
    }

    private isIteratorResultValueLocal(declaringStmt: ArkAssignStmt): boolean {
        const rightOp = declaringStmt.getRightOp();
        if (!(rightOp instanceof ArkCastExpr)) {
            return false;
        }
        const castOp = rightOp.getOp();
        if (!(castOp instanceof Local)) {
            return false;
        }
        const castOpDeclaring = castOp.getDeclaringStmt();
        if (!(castOpDeclaring instanceof ArkAssignStmt)) {
            return false;
        }
        const castOpRight = castOpDeclaring.getRightOp();
        if (!(castOpRight instanceof ArkInstanceFieldRef)) {
            return false;
        }
        const fieldSig = castOpRight.getFieldSignature();
        if (fieldSig.getFieldName() !== 'value') {
            return false;
        }
        const declaringSig = fieldSig.getDeclaringSignature();
        return declaringSig instanceof ClassSignature && ITERATOR_RESULT_CLASS_NAMES.has(declaringSig.getClassName());
    }
}
