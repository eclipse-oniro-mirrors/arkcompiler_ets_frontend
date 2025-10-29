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

interface I {
    i:number
}
class A implements I{
    get i(): number { console.log("getter"); return this.i}
    set i(x: number) { console.log("setter"); this.i = x;}
}

function main() {
    let a = new A()
    a.i = 2
}


/* @@? 20:58 Warning Warning W0023: Reading the value of the property inside its getter may lead to an endless loop. */
/* @@? 21:52 Warning Warning W0024: Assigning new value to the property inside its setter may lead to an endless loop. */