/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

interface A {
    x(): boolean {
        return true;
    }
}

class B implements A {
    public x: boolean;
    public getX(): boolean {
       return this.x();
    }
}

function main(): void {
    let instance: B = new B();
    arktest.assertEQ(instance.getX(), true);
}

/* @@? 16:1 Error Semantic error ESE0215: Cannot inherit from interface A because method x is inherited with a different declaration type */
/* @@? 23:12 Error Semantic error ESE0136: Method x(<property>x: Boolean): void in B not overriding any method */
/* @@? 25:15 Error Semantic error ESE0002: Type 'Boolean' has no call signatures. */
