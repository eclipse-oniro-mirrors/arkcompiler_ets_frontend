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

const c: string = "const";
let l: string = "let";

enum Values: string {
    FIRST = "literal" as string,
    SECOND = c,
    THIRD = l,
}

function main(): void {
    let first = Values.FIRST;
    let second = Values.SECOND;
    let third = Values.THIRD;

    arktest.assertTrue(first instanceof Values);
    arktest.assertTrue(second instanceof Values);
    arktest.assertTrue(third instanceof Values);
    arktest.assertTrue(first.valueOf() == "literal");
    arktest.assertTrue(second.valueOf() == "const");
    arktest.assertTrue(third.valueOf() == "let");
}
