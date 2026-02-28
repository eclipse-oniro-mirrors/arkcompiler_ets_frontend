/*
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and the
 * limitations under the License.
 */

class A {
    private constructor() {}
}

class B extends /* @@ label */A {
}

/* @@? 20:31 Error Semantic error ESE201875: Cannot extend the superclass 'A' as it has no accessible constructor which can be called without arguments. */
