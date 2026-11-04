/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

type AliasPrimitive<T> = T;
type AliasAlias<T> = AliasPrimitive<T>;

function fn(p: Double)
{
    arktest.assertEQ(p, 42)
}

function main() {
    let v1 : double = new Int(42);  // widening
    fn(v1);

    let v2 : AliasPrimitive<double> = new Int(42);  // widening
    fn(v2);

    let v3 : AliasAlias<double> = new Int(42);   // widening
    fn(v3);
}
