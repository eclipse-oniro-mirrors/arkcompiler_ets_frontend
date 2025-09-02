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
    foo<X extends short>(): Promise<X> {
       return Promise.resolve(1 as X);
    }
}

function main(): void {
    const p = new Point();
    p.foo<short>().then((value: short) => {
       arktest.assertEQ(value, 1 as short); 
    });   
}

/* @@? 18:15 Error TypeError: Type 'Promise<*ERROR_TYPE*>' is not compatible with the enclosing method's return type 'Promise<X>'  */
/* @@? 18:31 Error TypeError: Cannot cast type 'Int' to 'X'  */
