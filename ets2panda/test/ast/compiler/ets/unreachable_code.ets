/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

function unreachableInIfFalse(): void {
    if (false) {
        let unreach: number = 10;
    }
}

function unreachableInResolvedVariableCondition(): void {
    let num: number = 0;
    if (num > 0) {
        let unreach: number = 10;
    }
}

function unreachableAfterNestedSameConditionalReturn(flag: boolean): void {
    if (flag) {
        if (flag) {
            return;
        }
        let unreach: number = 10;
    }
}

function unreachableInShadowedBooleanCondition(): void {
    let flag: boolean = true;
    if (flag) {
        let flag: boolean = false;
        if (flag) {
            let unreach: number = 10;
        }
    }
}

function noUnreachableInShadowedNumericCondition(): void {
    let num: number = 0;
    {
        let num: number = 1;
    }
    if (num == 0) {
        let reachable: number = 10;
    }
}

function noUnreachableAfterIncrementedNumericCondition(): void {
    let num: number = 0;
    num++;
    if (num == 0) {
        return;
    }
    let reachable: number = 10;
}

function noUnreachableAfterDecrementedNumericCondition(): void {
    let num: number = 0;
    --num;
    if (num != 0) {
        let reachable: number = 10;
    }
}

function noUnreachableAfterAssignmentInTrueCondition(flag: boolean): void {
    if (flag) {
        flag = false;
        if (flag) {
            return;
        }
        let reachable: number = 10;
    }
}

function noUnreachableAfterLoopUpdate(flag: boolean): void {
    let num: number = 0;
    while (flag) {
        num++;
    }
    if (num == 0) {
        let reachable: number = 10;
    }
}

function noUnreachableAfterCallMutatesCapturedBoolean(): void {
    let flag: boolean = false;
    let setFlag = (): void => {
        flag = true;
    };
    setFlag();
    if (flag) {
        let reachable: number = 10;
    }
}

function noUnreachableAfterCallMutatesCapturedNumber(): void {
    let num: number = 0;
    let setNum = (): void => {
        num = 1;
    };
    setNum();
    if (num != 0) {
        let reachable: number = 10;
    }
}

class NonNullishUnreachableGuard {
    field: int;

    static unreachableAfterDefinitelyNullishAccess(): void {
        let obj: NonNullishUnreachableGuard | null = null;
        let num: number = 0;
        try {
            obj!.field += ++num;
        } catch (e) {
            if (num == 0) {
                return;
            }
            let unreach: number = 10;
        }
    }
}

/* @@? 18:9 Warning Warning W0026: Unreachable statement. */
/* @@? 25:9 Warning Warning W0026: Unreachable statement. */
/* @@? 34:9 Warning Warning W0026: Unreachable statement. */
/* @@? 43:13 Warning Warning W0026: Unreachable statement. */
/* @@? 124:13 Warning Warning W0014: Operand of the ensure-not-nullish expression is 'null' or 'undefined'. */
/* @@? 129:13 Warning Warning W0026: Unreachable statement. */
