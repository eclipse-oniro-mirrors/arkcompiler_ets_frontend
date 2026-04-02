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

// Test for multiple parameters with type hierarchy chain (5 arguments)
// Override rules (one-way hierarchy):
// - Any type can override Object type
// - Object type can override primitive types
// - Object CANNOT override Any
// - primitives CANNOT override each other

// Level 0: Base with all primitive types
class MultiParamBase {
    method(a: int, b: string, c: boolean, d: long, e: short): string { return "base" }
}

// Level 1: Override params 1,2,3 with Object
class MultiParamLevel1 extends MultiParamBase {
    method(a: Object, b: Object, c: Object, d: long, e: short): string { return "level1" }
}

// Level 2: Override params 1,2,3,4 with Object
class MultiParamLevel2 extends MultiParamLevel1 {
    method(a: Object, b: Object, c: Object, d: Object, e: short): string { return "level2" }
}

// Level 3: Override params 1,2,3,4 with Any
class MultiParamLevel3 extends MultiParamLevel2 {
    method(a: Any, b: Any, c: Any, d: Any, e: short): string { return "level3" }
}

// Level 4: Override all 5 params with Any
class MultiParamLevel4 extends MultiParamLevel3 {
    method(a: Any, b: Any, c: Any, d: Any, e: Any): string { return "level4" }
}

function main() {
    let ts = new arktest.ArkTestsuite("override_methods_multi_param")

    ts.addTest("MultiParamBase", () => {
        let obj = new MultiParamBase()
        let base: MultiParamBase = obj as MultiParamBase
        arktest.assertEQ(base.method(1, "test", true, 1, 1), "base")
    })

    ts.addTest("MultiParamLevel1", () => {
        let obj = new MultiParamLevel1()
        let base: MultiParamBase = obj as MultiParamBase
        arktest.assertEQ(base.method(1, "test", true, 1, 1), "level1")
    })

    ts.addTest("MultiParamLevel2", () => {
        let obj = new MultiParamLevel2()
        let base: MultiParamBase = obj as MultiParamBase
        arktest.assertEQ(base.method(1, "test", true, 1, 1), "level2")
    })

    ts.addTest("MultiParamLevel3", () => {
        let obj = new MultiParamLevel3()
        let base: MultiParamBase = obj as MultiParamBase
        arktest.assertEQ(base.method(1, "test", true, 1, 1), "level3")
    })

    ts.addTest("MultiParamLevel4", () => {
        let obj = new MultiParamLevel4()
        let base: MultiParamBase = obj as MultiParamBase
        arktest.assertEQ(base.method(1, "test", true, 1, 1), "level4")
    })

    return ts.run()
}
