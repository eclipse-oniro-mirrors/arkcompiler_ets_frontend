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

export type EtsNativePointer = long;

const NATIVE_LIBRARY = 'arkts_bindings_tests_converters_native';

export class TestNativeModule {
    private constructor() {}

    native _EtsToNative_Boolean(value: boolean, idx: int): void;
    native _NativeToEts_Boolean(idx: int): boolean;

    native _EtsToNative_Byte(value: byte, idx: int): void;
    native _NativeToEts_Byte(idx: int): byte;

    native _EtsToNative_Short(value: short, idx: int): void;
    native _NativeToEts_Short(idx: int): short;

    native _EtsToNative_Int(value: int, idx: int): void;
    native _NativeToEts_Int(idx: int): int;

    native _EtsToNative_Long(value: long, idx: int): void;
    native _NativeToEts_Long(idx: int): long;

    native _EtsToNative_Float(value: float, idx: int): void;
    native _NativeToEts_Float(idx: int): float;

    native _EtsToNative_Double(value: double, idx: int): void;
    native _NativeToEts_Double(idx: int): double;

    native _EtsToNative_String(value: string, idx: int): void;
    native _NativeToEts_String(idx: int): string;

    native _NativeToEts_CString(idx: int): string;

    native _EtsToNative_StringArray(value: string[], len: int, idx: int): void;

    native _CreatePair(a: int, b: int): EtsNativePointer;
    native _ModifyPair(obj: EtsNativePointer, a: int, b: int): void;
    native _ValidatePair(obj: EtsNativePointer, idx: int): void;
    native _DestroyPair(obj: EtsNativePointer): void;
    

    native _Bridge_V1(p1: int): void;
    native _Bridge_V2(p1: int, p2: int): void;
    native _Bridge_V3(p1: int, p2: int, p3: int): void;
    native _Bridge_V4(p1: int, p2: int, p3: int, p4: int): void;
    native _Bridge_V5(p1: int, p2: int, p3: int, p4: int, p5: int): void;
    native _Bridge_V6(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int): void;
    native _Bridge_V7(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int): void;
    native _Bridge_V8(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int): void;
    native _Bridge_V9(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int): void;
    native _Bridge_V10(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int): void;
    native _Bridge_V11(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int, p11: int): void;
    native _Bridge_V12(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int, p11: int, p12: int): void;
    native _Bridge_V13(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int, p11: int, p12: int, p13: int): void;
    native _Bridge_V14(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int, p11: int, p12: int, p13: int, p14: int): void;
    native _Bridge_V15(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int, p11: int, p12: int, p13: int, p14: int, p15: int): void;

    native _Bridge_0(): int;
    native _Bridge_1(p1: int): int;
    native _Bridge_2(p1: int, p2: int): int;
    native _Bridge_3(p1: int, p2: int, p3: int): int;
    native _Bridge_4(p1: int, p2: int, p3: int, p4: int): int;
    native _Bridge_5(p1: int, p2: int, p3: int, p4: int, p5: int): int;
    native _Bridge_6(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int): int;
    native _Bridge_7(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int): int;
    native _Bridge_8(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int): int;
    native _Bridge_9(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int): int;
    native _Bridge_10(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int): int;
    native _Bridge_11(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int, p11: int): int;
    native _Bridge_12(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int, p11: int, p12: int): int;
    native _Bridge_13(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int, p11: int, p12: int, p13: int): int;
    native _Bridge_14(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int, p11: int, p12: int, p13: int, p14: int): int;
    native _Bridge_15(p1: int, p2: int, p3: int, p4: int, p5: int, p6: int, p7: int, p8: int, p9: int, p10: int, p11: int, p12: int, p13: int, p14: int, p15: int): int;

    private static _nativeModule: TestNativeModule | undefined = undefined;
    public static get nativeModule(): TestNativeModule {
        return TestNativeModule._nativeModule ?? TestNativeModule.initNativeModule()
    }

    private static initNativeModule(): TestNativeModule {
        loadLibrary(NATIVE_LIBRARY);
        return new TestNativeModule();
    }
}