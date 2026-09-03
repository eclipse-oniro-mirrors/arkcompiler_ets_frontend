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

// Regression test for a compiler crash in ETSChecker::HandleAwaitedUtilityType
// (utilityTypeHandlers.cpp): a recursive type alias whose target is a union
// with the alias itself ('T = Promise<T|int>') used to hit an unchecked
// AsETSObjectType() while computing Awaited<T|int> during generic
// instantiation of Promise members (ETSAwaitedType::Substitute).

// Self-referencing type alias: the awaited type of 'T' keeps the alias intact
// instead of infinitely unwrapping 'Promise<T|int>'.
type T = Promise<T|int>

function makePromise(): Promise<int> {
    return new Promise<int>((resolve) => {
        resolve(42)
    })
}

function main() {
    // Instantiating 'Promise<T>' methods triggers ETSAwaitedType::Substitute
    // on the union 'T|int' whose member 'T' is the recursive alias above.
    let p: Promise<T> = new Promise<T>((resolve) => {
        resolve(makePromise() as Object as T)
    })
    waitForCompletion(async () => {
        // 'Awaited<T>' unwraps one Promise layer of 'Promise<T|int>':
        // awaiting yields 'T|int', so the int result is assignable to it.
        let v: Awaited<T> = await p
        arktest.assertEQ(v as int, 42)
    })
}
