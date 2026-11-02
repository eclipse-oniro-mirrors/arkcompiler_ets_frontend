/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/modules/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Bytecode feature gated at target API 26.1.0: only the abrupt (exception)
// close of a SYNC iterator (for-of whose body throws, caught outside the
// loop) is emitted as the dedicated closeiterator opcode for 26.1.0 and
// later targets; every lower target keeps the inlined Iterator::Close
// sequence. The non-abrupt (break/continue/return) close stays inlined at
// EVERY target version - the break loop below pins that. return() is a
// plain data property here, so both emissions are observably identical
// and the printed output must not depend on the compile target.
function makeIterable(values) {
    return {
        [Symbol.iterator]() {
            let index = 0;
            return {
                next() {
                    return index < values.length
                        ? { done: false, value: values[index++] }
                        : { done: true };
                },
                return() {
                    print("closed");
                    return { done: true };
                }
            };
        }
    };
}

let total = 0;
// break: non-abrupt close - stays inlined for every target API version
for (const value of makeIterable([1, 2, -1, 4])) {
    if (value < 0) {
        break;
    }
    total += value;
}
// throw: abrupt close - folds into the closeiterator opcode from 26.1.0 on
try {
    for (const value of makeIterable([10, 20, -1, 40])) {
        if (value < 0) {
            throw new Error("negative value");
        }
        total += value;
    }
} catch (e) {
    print(e.message);
}
print(total);
