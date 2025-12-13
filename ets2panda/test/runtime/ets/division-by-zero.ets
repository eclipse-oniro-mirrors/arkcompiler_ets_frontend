/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function main() : void {
    let caught_counter = 0;

    let INT_ONE : int = 1;
    let INT_ZERO : int = 0;
    let LONG_ONE : long = 1;
    let LONG_ZERO : long = 0;

    try {
        let result : int = INT_ONE / INT_ZERO;
    } catch (error) {
        if (error instanceof ArithmeticError) {
            caught_counter++;
        }
    }
    arktest.assertEQ(caught_counter, 1)

    try {
        let result : long = LONG_ONE / LONG_ZERO;
    } catch (error) {
        if (error instanceof ArithmeticError) {
            caught_counter++;
        } else {
            console.log(error);
        }
    }
    arktest.assertEQ(caught_counter, 2)

    try {
        let result : int = INT_ONE % INT_ZERO;
    } catch (error) {
        if (error instanceof ArithmeticError) {
            caught_counter++;
        } else {
            console.log(error);
        }
    }
    arktest.assertEQ(caught_counter, 3)

    try {
        let result : long = LONG_ONE % LONG_ZERO;
    } catch (error) {
        if (error instanceof ArithmeticError) {
            caught_counter++;
        } else {
            console.log(error);
        }
    }
    arktest.assertEQ(caught_counter, 4)

    let DOUBLE_ONE : double = 1.0;
    let DOUBLE_ZERO : double = 0.0;

    try {
        let result : double = DOUBLE_ONE / DOUBLE_ZERO;
    } catch (error) {
        if (error instanceof ArithmeticError) {
            arktest.assertTrue(false)
        } else {
            console.log(error);
        }
    }

    try {
        let result = INT_ONE / DOUBLE_ZERO;
    } catch (error) {
        if (error instanceof ArithmeticError) {
            arktest.assertTrue(false)
        } else {
            console.log(error);
        }
    }

    try {
        let result = DOUBLE_ONE / INT_ZERO;
    } catch (error) {
        if (error instanceof ArithmeticError) {
            arktest.assertTrue(false)
        } else {
            console.log(error);
        }
    }

    try {
        let result : double = DOUBLE_ONE % DOUBLE_ZERO;
    } catch (error) {
        if (error instanceof ArithmeticError) {
            arktest.assertTrue(false)
        } else {
            console.log(error);
        }
    }

    try {
        let result = INT_ONE % DOUBLE_ZERO;
    } catch (error) {
        if (error instanceof ArithmeticError) {
            arktest.assertTrue(false)
        } else {
            console.log(error);
        }
    }

    try {
        let result = DOUBLE_ONE % INT_ZERO;
    } catch (error) {
        if (error instanceof ArithmeticError) {
            arktest.assertTrue(false)
        } else {
            console.log(error);
        }
    }
}
