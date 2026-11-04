/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

function main(): void {
    let str: String = "hello\nworld\n";
    arktest.assertEQ(str.charAt(2), c'l')
    arktest.assertEQ(str.length, 12)
    arktest.assertEQ(str.charAt(Double.toInt(str.length) - 1), c'\n')

    let a: String = "abc";
    let b: String = "ace";
    let c: String = "abc";
    arktest.assertTrue(!a.equals(b))
    arktest.assertTrue(a.equals(c))
    arktest.assertTrue(!b.equals(c))

    let n2: int = 41;
    let n3: long = 42;
    let n4: float = 43.43f;
    let n5: double = 44.44;
    let n6: double = random();
    let n7: double = random() + 42;
    arktest.assertTrue(n6 >= 0.0 && n6 <= 1.0)
    arktest.assertTrue(n7 >= 42.0 && n7 <= 43.0)

    let s1: String = "This ";
    let s2: String = "is ";
    let s3: String = "another ";
    let s4: String = "string";
    let s5: String = "This is another string";
    arktest.assertTrue((s1 + s2 + s3 + s4).equals(s5))
}
