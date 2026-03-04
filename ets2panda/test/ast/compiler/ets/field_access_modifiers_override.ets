/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
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

class C {
    protected foo: number;
}

class D extends C {
    public foo: number;
}

class C1 {
    public foo: number;
}

class D1 extends C1 {
    private foo: number;
}

class C2 {
    private foo: number;
}

class D2 extends C2 {
    public foo: number;
}

class C3 {
    public foo: number;
}

class D3 extends C3 {
    protected foo: number;
}

/* @@? 21:12 Error Semantic error ESE1600001: Access modifier of field 'foo' in class 'D' and in the base class 'C' must be the same. */
/* @@? 29:13 Error Semantic error ESE1600001: Access modifier of field 'foo' in class 'D1' and in the base class 'C1' must be the same. */
/* @@? 45:15 Error Semantic error ESE1600001: Access modifier of field 'foo' in class 'D3' and in the base class 'C3' must be the same. */
