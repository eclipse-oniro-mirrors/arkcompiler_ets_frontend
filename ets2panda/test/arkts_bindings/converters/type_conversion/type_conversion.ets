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

import { TestNativeModule } from '../native';

const BOOLEAN_TEST_DATA: boolean[] = [true, false, true, false, false, true, false, false];
const BYTE_TEST_DATA: byte[] = [-128, -127, -35, 0, 1, 27, 100, 127];
const SHORT_TEST_DATA: short[] = [-32768, -15325, -30, 0, 1, 12345, 32767];
const INT_TEST_DATA: int[] = [1, 0, -1, 82, -125, 123456, 205452356, -300000000, 1234567890, -2147483648, 2147483647];
const LONG_TEST_DATA: long[] = [0, -100, 200, 123456789012345, 1_000_000_000_000_000_000, 9_223_372_036_854_775_807, -9_223_372_036_854_775_808];
const FLOAT_TEST_DATA: float[] = [0f, 0.0f, 1f, -20.1f, 1.56f, 10/3f, 12345e-3f];
const DOUBLE_TEST_DATA: double[] = [-10.0, 0.0, 1.2, -300.6125, 2/3.0, 100/3.0];
const STRING_TEST_DATA: string[] = [
    '', '  ', '1', '200', 'a', 'b', 'z', 'qwerty123', '500600123456foobar',
    '   whitespaces at the beginning', 'whitespaces at the end   ', '   whitespaces     everywhere   ',
    '/home/project/src/dir1/dir2/dir3/file.ets'
];

const STRING_ARRAY_TEST_DATA: string[][] = [
    [],
    [""],
    ["hello world"],
    ["qwe", "rty", "hello"],
    ["1", "20", "300", "4000", "50000"]
];

abstract class BaseTypeConversionTest {
    protected testName: string = '';

    public get name() {
        return this.testName;
    }

    abstract run(): void;
}

type EtsToNativeF<T> = (val: T, idx: int) => void;
type NativeToEtsF<T> = (idx: int) => T;

class TypeConversionTest<T> extends BaseTypeConversionTest {
    private testData: T[];
    private etsToNativeFun: (EtsToNativeF<T>) | undefined;
    private nativeToEtsFun: NativeToEtsF<T> | undefined;

    constructor(
        name: string,
        testData: T[],
        etsToNativeFun: EtsToNativeF<T> | undefined,
        nativeToEtsFun: NativeToEtsF<T> | undefined
    ) {
        super();
        this.testName = name;
        this.testData = testData;
        this.etsToNativeFun = etsToNativeFun;
        this.nativeToEtsFun = nativeToEtsFun;
    }

    run(): void {
        for (let i = 0; i < this.testData.length; i++) {
            if (this.etsToNativeFun) {
                this.etsToNativeFun!(this.testData[i], i);
            }
            if (this.nativeToEtsFun) {
                arktest.assertEQ(this.nativeToEtsFun!(i), this.testData[i]);
            }
        }
    }
}

export function addTypeConversionTests(suite: arktest.ArkTestsuite): void {
    const nativeModule = TestNativeModule.nativeModule;

    let tests: BaseTypeConversionTest[] = [
        new TypeConversionTest<boolean>('Boolean type', BOOLEAN_TEST_DATA, nativeModule._EtsToNative_Boolean, nativeModule._NativeToEts_Boolean),
        new TypeConversionTest<byte>('Byte type', BYTE_TEST_DATA, nativeModule._EtsToNative_Byte, nativeModule._NativeToEts_Byte),
        new TypeConversionTest<short>('Short type', SHORT_TEST_DATA, nativeModule._EtsToNative_Short, nativeModule._NativeToEts_Short),
        new TypeConversionTest<int>('Int type', INT_TEST_DATA, nativeModule._EtsToNative_Int, nativeModule._NativeToEts_Int),
        new TypeConversionTest<long>('Long type', LONG_TEST_DATA, nativeModule._EtsToNative_Long, nativeModule._NativeToEts_Long),
        new TypeConversionTest<float>('Float type', FLOAT_TEST_DATA, nativeModule._EtsToNative_Float, nativeModule._NativeToEts_Float),
        new TypeConversionTest<double>('Double type', DOUBLE_TEST_DATA, nativeModule._EtsToNative_Double, nativeModule._NativeToEts_Double),
        new TypeConversionTest<string>('String type', STRING_TEST_DATA, nativeModule._EtsToNative_String, nativeModule._NativeToEts_String),
        new TypeConversionTest<string>('C-style string type', STRING_TEST_DATA, undefined, nativeModule._NativeToEts_CString),
        new TypeConversionTest<string[]>('String[] type', STRING_ARRAY_TEST_DATA, (val: string[], idx: int) => {
            nativeModule._EtsToNative_StringArray(val, val.length, idx)
        }, undefined)
    ];

    for (let test of tests) {
        suite.addTest(test.name, () => { test.run() });
    }
}