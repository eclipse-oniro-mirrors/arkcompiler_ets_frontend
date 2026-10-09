/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

class SplitBase {}
class SplitDerived extends SplitBase {}

function accept<S extends SplitBase | undefined>(p: S): S {
    return p;
}

function split<T extends SplitDerived | undefined>(p: T): void {
    arktest.assertEQ(accept(p ? p : p), p);
}

let sawNonNull: boolean = false;

// Narrowing a union of a type parameter and 'undefined' must not add 'null' to the nullish part,
// otherwise the type merged after the check is no longer assignable to the declared 'T | undefined'.
function popAndTrack<T>(items: Array<T>): T | undefined {
    const result = items.pop();
    if (result) {
        sawNonNull = true;
    }
    return result;
}

function main(): void {
    split<SplitDerived | undefined>(new SplitDerived());
    split<SplitDerived | undefined>(undefined);

    const derived: SplitDerived = new SplitDerived();
    const items: Array<SplitDerived> = [derived];
    sawNonNull = false;
    arktest.assertEQ(popAndTrack(items), derived);
    arktest.assertTrue(sawNonNull);

    const onlyNullish: Array<SplitDerived | undefined> = [undefined];
    sawNonNull = false;
    const popped: SplitDerived | undefined = popAndTrack<SplitDerived | undefined>(onlyNullish);
    arktest.assertTrue(popped === undefined);
    arktest.assertFalse(sawNonNull);
}
