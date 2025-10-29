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

class A {
    private a: int = 1;
}

class B extends A {
    constructor() {
        super./* @@ label */a = 2;
    }
}


/* @@@ label Error Semantic error ESE0377: Class field 'a' defined by the parent class is not accessible in the child class via super. */
/* @@@ label Error Semantic error ESE0293: Property a is not visible here. */