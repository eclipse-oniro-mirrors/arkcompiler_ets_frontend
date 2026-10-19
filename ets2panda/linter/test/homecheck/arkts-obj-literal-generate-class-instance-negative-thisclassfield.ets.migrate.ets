/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class A {
    name: string = ''
}

class Test {
    a1: A = {name: '1'}
    public a2: A = {name: '1'}
    private a3: A = {name: '1'}
    protected a4: A = {name: '1'}
    static a5: A = {name: '1'}

    foo() {
        let _ = this.a1 instanceof A
        _ = this.a2 instanceof A
        _ = this.a3 instanceof A
        _ = this.a4 instanceof A
        _ = this.a5 instanceof A
    }
}