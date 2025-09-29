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

type asyncLambda = () => Promise<void>

async function asyncFunc(): Promise<boolean> {
    return true;
}

async function callAsyncLambda() {
    let is_call_async_lambda: boolean = false;

    let async_lambda: asyncLambda = async ): Promise<void> => {
        await asyncFunc();
        is_call_async_lambda = true;
    }

    arktest.assertEQ(is_call_async_lambda, false);
    await async_lambda();
    arktest.assertEQ(is_call_async_lambda, true);
}

function main(): void {
    callAsyncLambda();
}

/* @@? 25:43 Error SyntaxError: Unexpected token, expected '('. */
/* @@? 25:43 Error TypeError: Type '((p1: Promise<void>) => Promise<void>)' cannot be assigned to type '(() => Promise<void>)' */
/* @@? 25:44 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 25:60 Error SyntaxError: Unexpected token, expected ',' or ')'. */
