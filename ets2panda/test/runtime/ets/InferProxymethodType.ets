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

 class A {
    met(s: string, q?: string){
        return true;
    }

    mat2(){
        return this.met("cat","a");
    }
}

function main() {
    let newA = new A();
    let a1 = new A().met('S');
    let a2 = new A().met('S',"a");
    let a3 = newA.met('S', 'a');
    let b = new A().mat2();

    assertTrue(a1)
    assertTrue(a2)
    assertTrue(a3)
    assertTrue(b)
}
