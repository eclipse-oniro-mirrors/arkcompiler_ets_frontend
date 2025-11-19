/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class CombinedArithmeticIndex {
    static arrayA = [1, 2, 3, 4];

    CombinedArithmeticIndex() {}

    static arith1() {
        // Subtraction (1)
        return CombinedArithmeticIndex.arrayA[3 + (-2)];
    }

    static arith2() {
        // Multiplication (2)
        return CombinedArithmeticIndex.arrayA[2 * -1 * -1];
    }

    static arith3() {
        // Precedence (3)
        return CombinedArithmeticIndex.arrayA[-6 / 2 * -1];
    }

    static arith4() {
        // Octal (3)
        return CombinedArithmeticIndex.arrayA[13 + -0o12];
    }
}

class UnaryAndBitwiseIndex {
    static arrayU = [1, 2, 3, 4];

    UnaryAndBitwiseIndex() {}

    static unary1() {
        // Prefix (0)
        let x: number = -1;
        return UnaryAndBitwiseIndex.arrayU[++x];
    }

    static unary2() {
        // Postfix (0)
        let y: number = 0;
        return UnaryAndBitwiseIndex.arrayU[y--];
    }

    static bitwise1() {
        // AND (1)
        return UnaryAndBitwiseIndex.arrayU[1 & -1];
    }
}

class ConditionalIndex {
    static arrayC = [1, 2, 3, 4];

    ConditionalIndex() {}

    static cond1() {
        // Literal (1)
        let x: number = -1;
        return ConditionalIndex.arrayC[true ? 1 : x];
    }

    static cond2() {
        // Comparison (1)
        let x: number = -1;
        return ConditionalIndex.arrayC[x < 0 ? 1 : -2];
    }
}
