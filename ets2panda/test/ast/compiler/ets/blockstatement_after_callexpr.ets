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

function foo(callback: (str: String) => void): void {
    callback("Hello");
}

console.log("hello")
{
    if (2 < 4) {
        return; // CTE
    }

    for (let i = 0; i < 10; i++) {
       return; // CTE;
    }

    console.log("world")
    {
        return; // CTE
    }

    foo((str: String): void => {
       return; // Do not throw error in arrow function
    });

    return; // CTE

}

console.log("I am a top level statement 1");
{
    return
}
console.log("I am a top level statement 2, but I am not executed");


function main() {
    console.log("Main...");
}

/* @@? 23:9 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 27:8 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 32:9 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 39:5 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 45:5 Error Syntax error ESY0163: return keyword should be used in function body. */
