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

function fun1() : string {
    let input = true;
    if (input instanceof Boolean) {
        return "Boolean";
    } else {
        // Unused variables, for testing the fix 
        let asddaf = input;
        return "No Boolean";
    }
}

function fun2() : string {
    let input = 1.2;
    if (input instanceof Double) {
        return "Double";
    } else {
        // Unused variables, for testing the fix 
        let asddaf = input;
        return "No Double";
    }
}

function fun3() : string {
    let input = 1;
    if (input instanceof Int) {
        return "Int";
    } else {
        // Unused variables, for testing the fix 
        let asddaf = input;
        return "No Int";
    }
}

function fun4() : string {
    let input = "str";
    if (input instanceof String) {
        return "String";
    } else {
        // Unused variables, for testing the fix 
        let asddaf = input;
        return "No String";
    }
}

function fun5() : string {
    let input = "str";
    if (input instanceof Boolean) {
        return "Boolean";
    } else {
        return "String";
    }
}

function fun6() : string {
    let input = "str";
    if (input instanceof Double) {
        return "Double";
    } else {
        return "String";
    }
}

function fun7() : string {
    let input = "str";
    if (input instanceof Int) {
        return "Int";
    } else {
        return "String";
    }
}

function main() {
    arktest.assertEQ(fun1(), "Boolean");
    arktest.assertEQ(fun2(), "Double");
    arktest.assertEQ(fun3(), "Int");
    arktest.assertEQ(fun4(), "String");
    arktest.assertEQ(fun5(), "String");
    arktest.assertEQ(fun6(), "String");
    arktest.assertEQ(fun7(), "String");
}
