/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export class A {
    public fld: number = -1
}

namespace OuterNamespace {
    export let test_object_inner: A = new A()
    
    export function getObj(): A {
        return test_object_inner
    }
    
    export namespace InnerNamespace {
        export let test_object_nested: A = new A()
        
        export function getObj(): A {
            return test_object_nested
        }
    }
}

function testNamespaceComplexExpression() {
    OuterNamespace.test_object_inner = new A()
    OuterNamespace.test_object_inner.fld = 10
    
    // Complex expression in namespace (needs temporary variable for getObj(), namespace itself doesn't)
    OuterNamespace.getObj().fld *= 5
    
    arktest.assertEQ(OuterNamespace.test_object_inner.fld, 50); // 10 * 5 = 50
}

function testNestedNamespaceComplexExpression() {
    OuterNamespace.InnerNamespace.test_object_nested = new A()
    OuterNamespace.InnerNamespace.test_object_nested.fld = 20
    
    // Complex expression in nested namespace (needs temporary variable for getObj(), namespace itself doesn't)
    OuterNamespace.InnerNamespace.getObj().fld += 5
    
    arktest.assertEQ(OuterNamespace.InnerNamespace.test_object_nested.fld, 25); // 20 + 5 = 25
}

function main() {
    testNamespaceComplexExpression();
    testNestedNamespaceComplexExpression();
}
