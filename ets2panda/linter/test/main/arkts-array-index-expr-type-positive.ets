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

class ArithmeticIndex {
    static arrayA = [1, 2, 3, 4, 5, 6, 7];

    ArithmeticIndex(){}

    static arith1() {
        // Subtraction (2)
        ArithmeticIndex.arrayA[5 - 3];
    }

    static arith2() {
        // Multiplication (4)
        ArithmeticIndex.arrayA[2 * 2];
    }

    static arith3() {
        // Division (5)
        ArithmeticIndex.arrayA[10 / 2];
    }
    
    static arith4() {
        // Sequence of additions (6)
        ArithmeticIndex.arrayA[1 + 2 + 3];
    }

    static arith5() {
        // Sequence of additions with multiplication (1)
        ArithmeticIndex.arrayA[1 + 2 + (-1) * 2];
    }

    static literal() {
        // Literal check with Octal Literal
        ArithmeticIndex.arrayA[0o1];
    }
}

class BitwiseAndUnaryIndex {
    static arrayU = [1, 2, 3, 4, 5, 6, 7];

    BitwiseAndUnaryIndex(){}

    static unary1() {
        // Bitwise NOT (~-1 = 0)
        BitwiseAndUnaryIndex.arrayU[~-1];
    }

    static unary2() {
        // Prefix Increment
        let j: int = 1; 
        BitwiseAndUnaryIndex.arrayU[++j]; // Index is 2
    }

    static unary3() {
        // Postfix Decrement (index is used before decrement)
        let k: int = 5; 
        BitwiseAndUnaryIndex.arrayU[k--]; // Index is 5
    }

    static bitwise1() {
        // Right shift (1000 >> 8 = 3)
        BitwiseAndUnaryIndex.arrayU[1000 >> 8];
    }

    static bitwise2() {
        // Bitwise OR (4 | 1 = 5)
        BitwiseAndUnaryIndex.arrayU[4 | 1];
    }

    static bitwise3() {
        // Bitwise AND (7 & 6 = 6)
        BitwiseAndUnaryIndex.arrayU[7 & 6];
    }
}

class ConditionalIndex {
    static arrayC = [1,2,3,4,5];

    ConditionalIndex(){}

    static cond1() {
        // Condition (2)
        ConditionalIndex.arrayC[true ? 2 : 1];
    }
    static cond2() {
        let z = 5;
        // Condition (0)
        ConditionalIndex.arrayC[z > 0 ? 0 : 1];
    }
}
