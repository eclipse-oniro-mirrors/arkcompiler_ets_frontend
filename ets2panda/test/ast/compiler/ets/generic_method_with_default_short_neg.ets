/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class Point {
    foo<X extends short>(i:X = 0 as X): X {
       return i;
    }
}

function main(): void {
    let p: Point = new Point();
    let result: short = p.foo<short>();
    arktest.assertEQ(result, 0 as short);       
}

/* @@? 17:32 Error TypeError: Cannot cast type 'Int' to 'X'  */
