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

class IntProviderA {
    public field: int = 11;

    getInt(x: int): int {
        return x + 1;
    }

    getBool(): boolean {
        return true;
    }

    getDouble(x: double): double {
        return x * 2.0;
    }

    getName(): String {
        return "A";
    }

    getIntOpt(x: int, y?: int): int {
        return x + (y == undefined ? 0 : y);
    }
}

class IntProviderB {
    public field: int = 22;

    getInt(x: int): int {
        return x + 2;
    }

    getBool(): boolean {
        return false;
    }

    getDouble(x: double): double {
        return x * 3.0;
    }

    getName(): String {
        return "B";
    }

    getIntOpt(x: int, y?: int): int {
        return x + (y == undefined ? 0 : y);
    }
}

type IntProvider = IntProviderA | IntProviderB;

function unionGetInt(p: IntProvider, x: int): int {
    return p.getInt(x);
}

function unionGetBool(p: IntProvider): boolean {
    return p.getBool();
}

function unionGetDouble(p: IntProvider, x: double): double {
    return p.getDouble(x);
}

function unionGetIntOpt(p: IntProvider, x: int): int {
    return p.getIntOpt(x);
}

function unionGetField(p: IntProvider): int {
    return p.field;
}

function genericGetInt<T extends IntProvider>(p: T, x: int): int {
    return p.getInt(x);
}

function genericGetBool<T extends IntProvider>(p: T): boolean {
    return p.getBool();
}

function genericGetDouble<T extends IntProvider>(p: T, x: double): double {
    return p.getDouble(x);
}

type DataArray = BigInt64Array | BigUint64Array;

function indexOfFirst<T extends DataArray>(a: T, fromIndex: int): int {
    return a.indexOf(0n, fromIndex);
}

function main() {
    arktest.assertEQ(unionGetInt(new IntProviderA(), 10), 11);
    arktest.assertEQ(unionGetInt(new IntProviderB(), 10), 12);

    arktest.assertTrue(unionGetBool(new IntProviderA()));
    arktest.assertFalse(unionGetBool(new IntProviderB()));

    arktest.assertEQ(unionGetDouble(new IntProviderA(), 1.5), 3.0);
    arktest.assertEQ(unionGetDouble(new IntProviderB(), 1.5), 4.5);

    arktest.assertEQ(unionGetIntOpt(new IntProviderA(), 5), 5);
    arktest.assertEQ(unionGetIntOpt(new IntProviderB(), 5), 5);

    arktest.assertEQ(unionGetField(new IntProviderA()), 11);
    arktest.assertEQ(unionGetField(new IntProviderB()), 22);

    arktest.assertEQ(genericGetInt(new IntProviderA(), 10), 11);
    arktest.assertEQ(genericGetInt(new IntProviderB(), 10), 12);

    arktest.assertTrue(genericGetBool(new IntProviderA()));
    arktest.assertFalse(genericGetBool(new IntProviderB()));

    arktest.assertEQ(genericGetDouble(new IntProviderA(), 1.5), 3.0);
    arktest.assertEQ(genericGetDouble(new IntProviderB(), 1.5), 4.5);

    arktest.assertEQ(indexOfFirst(new BigInt64Array(42), 42), -1);
    arktest.assertEQ(indexOfFirst(new BigUint64Array(42), 42), -1);
}
