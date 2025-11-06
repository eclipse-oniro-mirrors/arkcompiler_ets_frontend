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

// destructuring declaration + untyped object literal
let { a, b } = { a: 1, b: 2 };

// destructuring declaration + untyped object literal + type literal
let { a2, b2 } = {
    a2: 1,
    b2: {
        c2: 1,
        d2: '2'
    } as { c2: number, d2: string }
};

// untyped object literal + 'in' operator
console.log('a' in { a: 1, b: 2 });

// untyped object literal + var declaration + literal as property name + function expression
var fun = function() { 
    var o = {
        'a': 1,
        'b': 2
    };

    var o2 = {
        'c': 3,
        'd': 4,
        5: {
            x1: 10,
            x2: 20
        } 
    };
};

// private identifier + definite assignment
class A {
    #a!: number;
}

// type assertion + as 'const'
const t = <const> 'hello';